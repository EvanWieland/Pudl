#include "Parser.h"

#include <stdexcept>

/**
Converts string representation of type to TType
\param aType String type
\return AST Type
*/
TType Parser::fromString(std::string aType) {
    if (aType == "int") {
        return TType::INTEGER;
    } else if (aType == "float") {
        return TType::FLOAT;
    } else if (aType == "bool") {
        return TType::BOOL;
    }
    return TType::UNDEFINED;
}

/**
Equals token types
\param aToken Token
\param aExpectedType Expected token type
\param aSuppress Suppress error if types are not equal
\return aToken.type == aExpectedType
*/
bool Parser::is(Token aToken, TokenType aExpectedType, bool aSuppress) {
    if (aToken.getType() != aExpectedType) {
        if (!aSuppress) {
            std::cout << "error: expected " << Token::showType(aExpectedType)
                      << " at (" << aToken.getLine() << ":" << aToken.getColumn() << ")"
                      << " but given " << Token::showType(aToken.getType()) << std::endl;
        }
        return false;
    }
    return true;
}

/**
\return AST root
*/
Node *Parser::parse(FILE *aFile) {
    lexer = std::make_unique<Lexer>(aFile);

    next();
    VectorNode *root = arena.construct<VectorNode>(std::vector<Node *>());
    while (1) {
        info("first! >> ");
        Token t = current;
        lexinfo(t.getLexeme(), t.getType());

        switch (t.getType()) {
            case ERROR_TOKEN:
                return arena.construct<DummyNode>();
            case EOF_TOKEN:
                return root;
            case FUNC: {
                FunctionDefNode *def = functionDef();
                if (def == NULL) { return arena.construct<DummyNode>(); }
                root->insert(def);
                break;
            }
            default:
                // next() is essential here, not cosmetic: without it,
                // `current` never advances past the token that just failed
                // to match any case above, so the next iteration of this
                // while(1) sees the exact same token, hits `default:`
                // again, and repeats forever -- an infinite loop calling
                // error() (found by fuzzing: see fuzz/fuzz_pipeline.cpp)
                // rather than a crash, which is why it never showed up as
                // a golden-test failure.
                error(t.getLine(), "unexpected token `" + t.getLexeme() + "`");
                next();
                break;
        }
    }

    return arena.construct<DummyNode>();
}

// function-definition
//  := func <name> [( <function-args> )]? : <type> <statement>
FunctionDefNode *Parser::functionDef() {
    infoln("debug?: parsing <function-definition>");
    scope.clear();
    Token t = next();

    if (!is(t, SYMBOL)) {
        return NULL;
    }
    std::string name = t.getLexeme();

    infoln("debug?: defining function '" + name + "'");

    t = next();
    std::vector<VarNode *> args;
    if (is(t, PL, true)) {
        next();
        args = functionArgs();
        for (
                std::vector<VarNode *>::iterator it = args.begin();
                it != args.end(); ++it
                ) {
            scope.insert(std::pair<std::string, VarNode *>((*it)->getName(), (*it)));
        }
    }
    if (!is(current, COLON)) { return NULL; }
    if (!is(t = next(), TYPE)) { return NULL; }

    TType type = fromString(t.getLexeme());
    if (type == TType::UNDEFINED) {
        std::cout << "WARN: undefined type" << std::endl;
    }
    next();

    // Registered (name/args/type known, body still nullptr) before the
    // body is parsed, so a call to this function from inside its own
    // body -- direct recursion -- can already resolve via funcs[name].
    // funcs.insert() used to happen only after the body was fully
    // parsed, so funcall() saw funcs[name] == NULL for any such
    // self-call and reported "function `name` is undefined". Mutual
    // recursion (A calls B, B calls A) still doesn't work -- that needs
    // a full pre-pass over every top-level function signature before
    // any body is parsed, which this single-pass parser doesn't do.
    FunctionDefNode *func = arena.construct<FunctionDefNode>(name, args, nullptr, type);
    funcs.insert(std::pair<std::string, FunctionDefNode *>(name, func));

    StatementNode *body = statement();
    if (body == NULL) { return NULL; }
    func->setBody(body);

    return func;
}

// function-args := [<type> <variable>,]* [<type> <variable>]?
std::vector<VarNode *> Parser::functionArgs() {
    infoln("debug?: parsing <function-args>");
    std::vector<VarNode *> args;
    while (1) {
        if (is(current, PR, true)) { break; }
        if (!is(current, TYPE)) { return args; }
        Token ty = current;
        if (!is(next(), SYMBOL)) { return args; }
        Token var = current;
        VarNode *arg = arena.construct<VarNode>(var.getLexeme(), fromString(ty.getLexeme()));
        args.push_back(arg);

        next();
        if (is(current, COMMA, true)) {
            next();
            continue;
        }
        if (is(current, PR)) { break; }
        else { return args; }
    }

    next();
    return args;
}

// statement := <block> | <expression> | <compound-statement>
// @implicit nullable
StatementNode *Parser::statement() {
    infoln("debug?: parsing <statement>");
    lexinfo(current.getLexeme(), current.getType());
    switch (current.getType()) {
        case EOF_TOKEN: {
            error(current.getLine(), "unexpected End-Of-File");
            return NULL;
        }
        case BL: {
            return blockStatement();
        }
        case IF: {
            return ifStatement();
        }
        case WHILE: {
            return whileStmt();
        }
        case DO: {
            return doWhileStmt();
        }
        case TYPE: {
            // <declaration>
            StatementNode *stmt = declaration();
            if (is(current, SEMICOLON, true)) { next(); }
            return stmt;
        }
        case SYMBOL: {
            // <funcall> | <assignment>
            Token tmp = current;
            Token lookup = next();
            if (is(lookup, PL, true)) {
                unlex(lookup);
                current = tmp;
                ExpressionWrapperNode *node = _funcall();
                if (is(current, SEMICOLON, true)) { next(); }
                return node;
            }
            unlex(lookup);
            current = tmp;
            AssignmentNode *node = assignment();
            if (is(current, SEMICOLON, true)) { next(); }
            return node;
        }
        case IO_PRINT: {
            IoPrintNode *iop = ioPrint();
            if (is(current, SEMICOLON, true)) { next(); }
            return iop;
        }
        case RETURN: {
            ReturnNode *retOp = ret();
            if (is(current, SEMICOLON, true)) { next(); }
            return retOp;
        }
    }
    return NULL;
}

// ret ::= Return <expression>
ReturnNode *Parser::ret() {
    infoln("debug?: parsing <ret>");
    Token tmp = current;
    ExpressionNode *expr = expression();
    if (expr == NULL) {
        error(tmp.getLine(), "expression expected after `return`");
        return NULL;
    }
    return arena.construct<ReturnNode>(expr);
}

// io-print ::= Print <expression>
IoPrintNode *Parser::ioPrint() {
    infoln("debug?: parsing <io-print>");
    Token tmp = current;
    ExpressionNode *expr = expression();
    if (expr == NULL) {
        error(tmp.getLine(), "expression expected after `print`");
        return NULL;
    }
    return arena.construct<IoPrintNode>(expr);
}

// if-stmt := If \( <expression : bool> \) <statement>
//   ( Else <statement> )?
IfStatementNode *Parser::ifStatement() {
    infoln("debug?: parsing <if-stmt>");
    Token t = current;
    // if (!is(next(), PL)) { return NULL; }
    ExpressionNode *cond = expression();
    if (cond == NULL) { return NULL; }
    if (cond->getType() != TType::BOOL) {
        error(t.getLine(), "expected boolean expression");
        return NULL;
    }

    infoln("debug?: parsing <if-stmt.true>");
    StatementNode *trueBranch = statement();
    if (trueBranch == NULL) { return NULL; }
    if (is(current, ELSE, true)) {
        infoln("debug?: parsing <if-stmt.false>");
        next();
        StatementNode *falseBranch = statement();
        if (falseBranch == NULL) { return NULL; }
        return arena.construct<IfStatementNode>(cond, trueBranch, falseBranch);
    }
    return arena.construct<IfStatementNode>(cond, trueBranch);
}

// while-stmt := While <expression : bool> <statement>
WhileStatementNode *Parser::whileStmt() {
    infoln("debug?: parsing <while-stmt>");
    Token t = current;
    ExpressionNode *cond = expression();
    if (cond == nullptr) { return nullptr; }
    if (cond->getType() != TType::BOOL) {
        error(t.getLine(), "expected boolean expression");
        return nullptr;
    }
    infoln("debug?: parsing <while-stmt.body>");
    StatementNode *body = statement();
    if (body == nullptr) { return nullptr; }
    return arena.construct<WhileStatementNode>(cond, body);
}

DoWhileStatementNode *Parser::doWhileStmt() {
    infoln("debug?: parsing <do-while-stmt>");
    Token t = current;
    next();
    StatementNode *body = statement();
    if (body == nullptr) { return nullptr; }

    infoln("debug?: parsing <do-while-stmt.cond>");
    if (!is(current, WHILE)) { return nullptr; }
    Token condTok = current;
    ExpressionNode *cond = expression();
    if (cond == nullptr) { return nullptr; }
    if (cond->getType() != TType::BOOL) {
        error(condTok.getLine(), "expected boolean expression");
        return nullptr;
    }
    return arena.construct<DoWhileStatementNode>(cond, body);
}

// block := { <statement>* }
BlockStatementNode *Parser::blockStatement() {
    infoln("debug?: parsing <block>");
    next();
    std::vector<StatementNode *> statements;
    while (!is(current, BR, true)) {
        lexinfo(current.getLexeme());
        if (current.getType() == EOF_TOKEN) {
            is(current, BR);
            return NULL;
        }

        StatementNode *node = statement();
        if (node == NULL) { return NULL; }
        statements.push_back(node);
    }
    next(); // skip `}`
    return arena.construct<BlockStatementNode>(
            statements
    );
}

// assignment := <variable> = <expression>
AssignmentNode *Parser::assignment() {
    infoln("debug?: parsing <assignment>");

    Token t = current;
    std::string name = t.getLexeme();

    VarNode *lhs = scope[name];
    if (lhs == NULL) {
        error(t.getLine(), "assignment to undeclared variable " + name);
        return NULL;
    }

    if (!is(next(), ASSIGN)) { return NULL; }

    Token op = current;
    ExpressionNode *rhs = expression();

    if (rhs == NULL) {
        error(op.getLine(), "expression expected after `=`");
        return NULL;
    }
    if (lhs->getType() == TType::BOOL && lhs->getType() != rhs->getType()) {
        error(t.getLine(), "expected boolean but given number");
        return nullptr;
    }
    if (lhs->getType() != TType::BOOL && rhs->getType() == TType::BOOL) {
        error(t.getLine(), "expected number but given boolean");
        return nullptr;
    }

    lexinfo(current);
    infoln("debug!: parsed <assignment>");
    return arena.construct<AssignmentNode>(lhs, rhs);
}

// declaration := Type <variable> = <expression>
AssignmentNode *Parser::declaration() {
    infoln("debug?: parsing <declaration>");

    Token t = current;

    TType type = fromString(current.getLexeme());
    if (!is(next(), SYMBOL)) { return NULL; }

    std::string name = current.getLexeme();
    // scope.insert() below silently no-ops if `name` is already a key
    // (std::map::insert() never overwrites an existing entry) -- without
    // this check, redeclaring a name compiled without error, but which
    // declaration later references actually resolved to was undefined
    // behavior from the language's perspective (whichever one happened
    // to already be in the map).
    if (scope.find(name) != scope.end()) {
        error(t.getLine(), "variable `" + name + "` is already declared");
        return nullptr;
    }
    VarNode *lhs = arena.construct<VarNode>(name, type);
    if (!is(next(), ASSIGN)) { return NULL; }

    Token op = current;
    ExpressionNode *rhs = expression();

    if (rhs == NULL) {
        error(op.getLine(), "expression expected after `=`");
        return NULL;
    }
    if (type == TType::BOOL && type != rhs->getType()) {
        error(t.getLine(), "expected boolean but given number");
        return nullptr;
    }
    if (type != TType::BOOL && rhs->getType() == TType::BOOL) {
        error(t.getLine(), "expected number but given boolean");
        return nullptr;
    }

    scope.insert(std::pair<std::string, VarNode *>(name, lhs));

    lexinfo(current);
    infoln("debug!: parsed <assignment>");
    return arena.construct<AssignmentNode>(lhs, rhs);
}

// expression := <lor>
ExpressionNode *Parser::expression() {
    infoln("debug?: parsing <expression>");
    Token t = current;
    switch (next().getType()) {
        case EOF_TOKEN:
            error(t.getLine(), "unexpected End-Of-File");
            return NULL;
    }
    ExpressionNode *node = lor();
    return node;
}

// lor := <land> (LOr <lor : int>)?
ExpressionNode *Parser::lor() {
    ExpressionNode *lhs = land();
    // A NULL lhs here means a deeper call already reported its own error
    // (e.g. unary()'s "expression expected after `+`") -- propagate it
    // rather than dereferencing it below. Found by fuzzing: an unchecked
    // lhs->getType() on a NULL lhs is a null-pointer dereference (see
    // fuzz/fuzz_pipeline.cpp), and every sibling precedence level
    // (land/cmpeq/cmp/additive/multiplicative) had the same gap.
    if (lhs == NULL) { return NULL; }
    // lexinfo(current.getLexeme(), current.getType());
    if (is(current, LOR, true)) {
        infoln("debug?: parsing <lor>");

        Token t = current;
        std::string op = t.getLexeme();

        next();
        ExpressionNode *rhs = lor();
        if (rhs == NULL) {
            error(t.getLine(), "expression expected after `" + op + "`");
            return NULL;
        }
        if (lhs->getType() != TType::BOOL || rhs->getType() != TType::BOOL) {
            error(t.getLine(), "expected boolean but given number");
            return nullptr;
        }

        return arena.construct<BinaryNode>(
                TType::BOOL, binaryOperatorFromLexeme(op), lhs, rhs
        );
    }
    return lhs;
}

// land := <cmpeq> (LAnd <land : int>)?
ExpressionNode *Parser::land() {
    ExpressionNode *lhs = cmpeq();
    if (lhs == NULL) { return NULL; }
    if (is(current, LAND, true)) {
        infoln("debug?: parsing <land>");

        Token t = current;
        std::string op = t.getLexeme();

        next();
        ExpressionNode *rhs = land();
        if (rhs == NULL) {
            error(t.getLine(), "expression expected after `" + op + "`");
            return NULL;
        }
        if (lhs->getType() != TType::BOOL || rhs->getType() != TType::BOOL) {
            error(t.getLine(), "expected boolean but given number");
            return nullptr;
        }

        return arena.construct<BinaryNode>(
                TType::BOOL, binaryOperatorFromLexeme(op), lhs, rhs
        );
    }
    return lhs;
}

// cmpeq := <cmp> (Eq <cmpeq>)?
ExpressionNode *Parser::cmpeq() {
    ExpressionNode *lhs = cmp();
    if (lhs == NULL) { return NULL; }
    // lexinfo(current.getLexeme(), current.getType());
    if (is(current, CMP_EQ, true)) {
        infoln("debug?: parsing <cmpeq>");

        Token t = current;
        std::string op = t.getLexeme();

        next();
        ExpressionNode *rhs = cmpeq();
        if (rhs == NULL) {
            error(t.getLine(), "expression expected after `" + op + "`");
            return NULL;
        }

        return arena.construct<BinaryNode>(
                TType::BOOL, binaryOperatorFromLexeme(op), lhs, rhs
        );
    }
    return lhs;
}

// cmp := <add> (Cmp <cmp>)?
ExpressionNode *Parser::cmp() {
    ExpressionNode *lhs = additive();
    if (lhs == NULL) { return NULL; }
    // lexinfo(current.getLexeme(), current.getType());
    if (is(current, CMP, true)) {
        infoln("debug?: parsing <cmp>");

        Token t = current;
        std::string op = t.getLexeme();

        next();
        ExpressionNode *rhs = cmp();
        if (rhs == NULL) {
            error(t.getLine(), "expression expected after `" + op + "`");
            return NULL;
        }
        if (lhs->getType() == TType::BOOL || rhs->getType() == TType::BOOL) {
            error(t.getLine(), "expected number but given boolean");
            return nullptr;
        }

        return arena.construct<BinaryNode>(
                TType::BOOL, binaryOperatorFromLexeme(op), lhs, rhs
        );
    }
    return lhs;
}

// add := <mul> (Add <add>)?
ExpressionNode *Parser::additive() {
    ExpressionNode *lhs = multiplicative();
    if (lhs == NULL) { return NULL; }
    // lexinfo(current.getLexeme(), current.getType());
    if (is(current, ADD, true)) {
        infoln("debug?: parsing <add>");

        Token t = current;
        std::string op = t.getLexeme();

        next();
        ExpressionNode *rhs = additive();
        if (rhs == NULL) {
            error(t.getLine(), "expression expected after `" + op + "`");
            return NULL;
        }
        if (lhs->getType() == TType::BOOL || rhs->getType() == TType::BOOL) {
            error(t.getLine(), "expected number but given boolean");
            return nullptr;
        }

        TType type = TType::INTEGER;
        if (lhs->getType() == TType::FLOAT || rhs->getType() == TType::FLOAT) {
            type = TType::FLOAT;
        }

        return arena.construct<BinaryNode>(
                type, binaryOperatorFromLexeme(op), lhs, rhs
        );
    }
    return lhs;
}

// mul := <unary> (Mul <mul>)?
ExpressionNode *Parser::multiplicative() {
    ExpressionNode *lhs = unary();
    if (lhs == NULL) { return NULL; }
    // lexinfo(current.getLexeme(), current.getType());
    if (is(current, MUL, true)) {
        infoln("debug?: parsing <mul>");

        Token t = current;
        std::string op = t.getLexeme();

        next();
        ExpressionNode *rhs = multiplicative();
        if (rhs == NULL) {
            error(t.getLine(), "expression expected after `" + op + "`");
            return NULL;
        }
        if (lhs->getType() == TType::BOOL || rhs->getType() == TType::BOOL) {
            error(t.getLine(), "expected number but given boolean");
            return NULL;
        }

        TType type = TType::INTEGER;
        if (lhs->getType() == TType::FLOAT || rhs->getType() == TType::FLOAT) {
            type = TType::FLOAT;
        }

        return arena.construct<BinaryNode>(
                type, binaryOperatorFromLexeme(op), lhs, rhs
        );
    }
    return lhs;
}

// unary := UnaryOperator? <factor>
ExpressionNode *Parser::unary() {
    if (is(current, ADD, true) || is(current, NOT, true)) {
        infoln("debug?: parsing <unary>");

        Token t = current;
        std::string op = t.getLexeme();

        next();
        ExpressionNode *exp = unary();
        if (exp == NULL) {
            error(t.getLine(), "expression expected after `" + op + "`");
            return NULL;
        }

        // t is the operator token itself, captured before next()/unary()
        // advanced past it and the operand -- checking current here (as
        // this used to) tests whatever token follows the whole unary
        // expression instead, making these guards non-functional.
        if (t.getType() == NOT && exp->getType() != TType::BOOL) {
            error(t.getLine(), "expected boolean but given number");
            return NULL;
        }
        if (t.getType() == ADD && exp->getType() == TType::BOOL) {
            error(t.getLine(), "expected number but given boolean");
            return NULL;
        }

        return arena.construct<UnaryNode>(unaryOperatorFromLexeme(op), exp);
    }
    ExpressionNode *node = factor();
    return node;
}

// factor := <constant> | \( <expression> \)
ExpressionNode *Parser::factor() {
    infoln("debug?: parsing <factor>");
    if (is(current, PL, true)) {
        infoln("lex!: PL");
        next();
        ExpressionNode *expr = lor();
        if (!is(current, PR)) { return NULL; }
        next();
        return expr;
    } else if (is(current, SYMBOL, true)) {
        Token tmp = current;
        Token lookup = next();
        if (is(lookup, PL, true)) {
            unlex(lookup);
            current = tmp;
            return funcall();
        }
        unlex(lookup);
        current = tmp;
        return var();
    }
    return constant();
}

// constant := <float> | <integer> | <var>
ExpressionNode *Parser::constant() {
    infoln("debug?: parsing <constant>");
    switch (current.getType()) {
        case BOOL:
            return boolean();
        case INTEGER:
            return intgr();
        case FLOAT:
            return flt();
    }
    return NULL;
}

// var := Symbol | Symbol \( <funcall-args> \)
VarNode *Parser::var() {
    infoln("debug?: parsing <var>");

    Token t = current;
    std::string name = current.getLexeme();
    next();

    VarNode *var = scope[name];
    if (var != NULL) { return var; }

    error(t.getLine(), "variable " + name + " is not initilized");
    return arena.construct<VarNode>(name, TType::UNDEFINED);
}

ExpressionWrapperNode *Parser::_funcall() {
    FuncallNode *node = funcall();
    if (node == NULL) { return NULL; }
    return arena.construct<ExpressionWrapperNode>(node);
}

// funcall := Symbol \( <funcall-args> \)
FuncallNode *Parser::funcall() {
    infoln("debug?: parsing <funcall> ");

    Token begin = current;
    std::string name = current.getLexeme();

    FunctionDefNode *func = funcs[name];
    if (func == NULL) {
        error(begin.getLine(), "function `" + name + "` is undefined");
        return NULL;
    }

    std::vector<ExpressionNode *> args = funcallArgs();
    next();
    return arena.construct<FuncallNode>(name, args, func->getType());
}

// funcall-args := <expression>*
std::vector<ExpressionNode *> Parser::funcallArgs() {
    infoln("debug?: parsing <funcall-args>");

    Token tmp = current;
    std::vector<ExpressionNode *> args;
    next();

    // Peek at the token right after '(' to detect a zero-argument call
    // (`name()`) without disturbing it for the "one or more args" path
    // below, which -- like every other <expr> callsite in this parser --
    // relies on expression()'s own leading next() to consume the
    // preceding separator token ('(' for the first argument, ',' for
    // each one after). The loop's own `is(current, PR, true)` check
    // can't see this: at this point `current` IS '(', not whatever
    // follows it, so `name()` always fell through to trying (and
    // failing) to parse ')' as an expression instead of recognizing
    // zero arguments -- "expected expression" for every no-arg call.
    Token afterParen = current;
    Token peek = next();
    unlex(peek);
    current = afterParen;

    if (is(peek, PR, true)) {
        // Consume the buffered ')' so this returns with `current`
        // positioned at ')', matching what the non-empty path below
        // leaves for funcall()'s own trailing next() to consume.
        next();
        return args;
    }

    while (1) {
        if (is(current, PR, true)) { break; }

        ExpressionNode *arg = expression();
        if (arg == NULL) {
            error(tmp.getLine(), "expected expression");
            return args; // TODO: skip top \)
        }
        lexinfo(current);
        args.push_back(arg);

        if (is(current, COMMA, true)) {
            // next();
            continue;
        }
        if (is(current, PR)) { break; }
        else { return args; }
    }
    return args;
}

// boolean := Bool
BooleanNode *Parser::boolean() {
    infoln("debug?: parsing <integer>");
    std::string value = current.getLexeme();
    next();

    return value == "True"
           ? arena.construct<BooleanNode>(true)
           : arena.construct<BooleanNode>(false);
}

// integer := Integer
IntegerNode *Parser::intgr() {
    infoln("debug?: parsing <integer>");
    Token t = current;
    std::string value = current.getLexeme();
    next();
    // std::stoi throws std::out_of_range for a literal too large/small to
    // fit an int (found by fuzzing: an uncaught exception here unwound
    // straight out of main(), aborting the whole process instead of
    // reporting a parse error) -- every other error path in this class
    // reports via error() and returns nullptr, which the (now
    // null-checked, see lor()/land()/etc.) precedence chain already knows
    // how to propagate cleanly.
    try {
        return arena.construct<IntegerNode>(std::stoi(value));
    } catch (const std::exception &) {
        error(t.getLine(), "integer literal `" + value + "` is out of range");
        return nullptr;
    }
}

// float := Float
FloatNode *Parser::flt() {
    infoln("debug?: parsing <float>");
    Token t = current;
    std::string value = current.getLexeme();
    lexinfo(value);
    next();
    try {
        return arena.construct<FloatNode>(std::stof(value));
    } catch (const std::exception &) {
        error(t.getLine(), "float literal `" + value + "` is out of range");
        return nullptr;
    }
}