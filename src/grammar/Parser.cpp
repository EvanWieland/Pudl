#include "Parser.h"

int Pudl::Parser::CurTok;
std::map<char, int> Pudl::Parser::BinopPrecedence;

int Pudl::Parser::getNextToken() { return Pudl::Parser::CurTok = Pudl::Lexer::getTok(); }

int Pudl::Parser::GetTokPrecedence() {
    if (!isascii(Pudl::Parser::CurTok))
        return -1;

    // Make sure it's a declared binop.
    int TokPrec = Pudl::Parser::BinopPrecedence[Pudl::Parser::CurTok];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::unique_ptr<Pudl::AST::PrototypeAST> Pudl::Parser::LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::ParseNumberExpr() {
    auto Result = std::make_unique<Pudl::AST::NumberExprAST>(Pudl::Lexer::NumVal);
    getNextToken(); // consume the number
    return std::move(Result);
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::ParseParenExpr() {
    getNextToken(); // eat (.
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (Pudl::Parser::CurTok != ')')
        return LogError("expected ')'");
    getNextToken(); // eat ).
    return V;
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::ParseIdentifierExpr() {
    std::string IdName = Pudl::Lexer::IdentifierStr;

    getNextToken(); // eat identifier.

    if (Pudl::Parser::CurTok != '(') // Simple variable ref.
        return std::make_unique<Pudl::AST::VariableExprAST>(IdName);

    // Call.
    getNextToken(); // eat (
    std::vector<std::unique_ptr<Pudl::AST::ExprAST>> Args;
    if (Pudl::Parser::CurTok != ')') {
        while (true) {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if (Pudl::Parser::CurTok == ')')
                break;

            if (Pudl::Parser::CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    // Eat the ')'.
    getNextToken();

    return std::make_unique<Pudl::AST::CallExprAST>(IdName, std::move(Args));
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::ParseIfExpr() {
    getNextToken(); // eat the if.

    // condition.
    auto Cond = ParseExpression();
    if (!Cond)
        return nullptr;

    if (Pudl::Parser::CurTok != Pudl::Lexer::tok_then)
        return LogError("expected then");
    getNextToken(); // eat the then

    auto Then = ParseExpression();
    if (!Then)
        return nullptr;

    if (Pudl::Parser::CurTok != Pudl::Lexer::tok_else)
        return LogError("expected else");

    getNextToken();

    auto Else = ParseExpression();
    if (!Else)
        return nullptr;

    return std::make_unique<Pudl::AST::IfExprAST>(std::move(Cond), std::move(Then),
                                                  std::move(Else));
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::ParseForExpr() {
    getNextToken(); // eat the for.

    if (Pudl::Parser::CurTok != Pudl::Lexer::tok_identifier)
        return LogError("expected identifier after for");

    std::string IdName = Pudl::Lexer::IdentifierStr;
    getNextToken(); // eat identifier.

    if (Pudl::Parser::CurTok != '=')
        return LogError("expected '=' after for");
    getNextToken(); // eat '='.

    auto Start = ParseExpression();
    if (!Start)
        return nullptr;
    if (Pudl::Parser::CurTok != ',')
        return LogError("expected ',' after for start value");
    getNextToken();

    auto End = ParseExpression();
    if (!End)
        return nullptr;

    // The step value is optional.
    std::unique_ptr<Pudl::AST::ExprAST> Step;
    if (Pudl::Parser::CurTok == ',') {
        getNextToken();
        Step = ParseExpression();
        if (!Step)
            return nullptr;
    }

    if (Pudl::Parser::CurTok != Pudl::Lexer::tok_in)
        return LogError("expected 'in' after for");
    getNextToken(); // eat 'in'.

    auto Body = ParseExpression();
    if (!Body)
        return nullptr;

    return std::make_unique<Pudl::AST::ForExprAST>(IdName, std::move(Start), std::move(End),
                                                   std::move(Step), std::move(Body));
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::ParseVarExpr() {
    getNextToken(); // eat the var.

    // Data type or auto is required.
    if (Pudl::Parser::CurTok != Pudl::Lexer::tok_type)
        return LogError("Expected data type after var");

    std::string Type = Pudl::Lexer::IdentifierStr;

    getNextToken(); // eat the type.

    std::vector<std::tuple<std::string, std::string, std::unique_ptr<Pudl::AST::ExprAST>>> VarNames;

    // At least one variable name is required.
    if (Pudl::Parser::CurTok != Pudl::Lexer::tok_identifier)
        return LogError("Expected identifier after var");

    while (true) {
        std::string Name = Pudl::Lexer::IdentifierStr;
        getNextToken(); // eat identifier.

        // Read the optional initializer.
        std::unique_ptr<Pudl::AST::ExprAST> Init = nullptr;
        if (Pudl::Parser::CurTok == '=') {
            getNextToken(); // eat the '='.

            Init = ParseExpression();
            if (!Init)
                return nullptr;
        }

        VarNames.push_back(std::make_tuple(Name, Type, std::move(Init)));

        // End of var list, exit loop.
        if (Pudl::Parser::CurTok != ',')
            break;
        getNextToken(); // eat the ','.

        if (Pudl::Parser::CurTok != Pudl::Lexer::tok_identifier)
            return LogError("expected identifier list after var");
    }

    // At this point, we have to have 'in'.
    if (Pudl::Parser::CurTok != Pudl::Lexer::tok_in)
        return LogError("expected 'in' keyword after 'var'");
    getNextToken(); // eat 'in'.

    auto Body = ParseExpression();
    if (!Body)
        return nullptr;

    return std::make_unique<Pudl::AST::VarExprAST>(std::move(VarNames), std::move(Body));
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::ParsePrimary() {
    switch (Pudl::Parser::CurTok) {
        default:
            return LogError("unknown token when expecting an expression");
        case Pudl::Lexer::tok_identifier:
            return ParseIdentifierExpr();
        case Pudl::Lexer::tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
        case Pudl::Lexer::tok_if:
            return ParseIfExpr();
        case Pudl::Lexer::tok_for:
            return ParseForExpr();
        case Pudl::Lexer::tok_var:
            return ParseVarExpr();
    }
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::ParseUnary() {
    // If the current token is not an operator, it must be a primary expr.
    if (!isascii(Pudl::Parser::CurTok) || Pudl::Parser::CurTok == '(' || Pudl::Parser::CurTok == ',')
        return ParsePrimary();

    // If this is a unary operator, read it.
    int Opc = Pudl::Parser::CurTok;
    getNextToken();
    if (auto Operand = ParseUnary())
        return std::make_unique<Pudl::AST::UnaryExprAST>(Opc, std::move(Operand));
    return nullptr;
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::ParseBinOpRHS(int ExprPrec,
                                                                std::unique_ptr<Pudl::AST::ExprAST> LHS) {
    // If this is a binop, find its precedence.
    while (true) {
        int TokPrec = GetTokPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (TokPrec < ExprPrec)
            return LHS;

        // Okay, we know this is a binop.
        int BinOp = Pudl::Parser::CurTok;
        getNextToken(); // eat binop

        // Parse the unary expression after the binary operator.
        auto RHS = ParseUnary();
        if (!RHS)
            return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        // Merge LHS/RHS.
        LHS = std::make_unique<Pudl::AST::BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

std::unique_ptr<Pudl::AST::ExprAST> Pudl::Parser::ParseExpression() {
    auto LHS = ParseUnary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<Pudl::AST::PrototypeAST> Pudl::Parser::ParsePrototype() {
    std::string FnName;

    unsigned Kind = 0; // 0 = identifier, 1 = unary, 2 = binary.
    unsigned BinaryPrecedence = 30;

    switch (Pudl::Parser::CurTok) {
        default:
            return LogErrorP("Expected function name in prototype");
        case Pudl::Lexer::tok_identifier:
            FnName = Pudl::Lexer::IdentifierStr;
            Kind = 0;
            getNextToken();
            break;
        case Pudl::Lexer::tok_unary:
            getNextToken();
            if (!isascii(Pudl::Parser::CurTok))
                return LogErrorP("Expected unary operator");
            FnName = "unary";
            FnName += (char) Pudl::Parser::CurTok;
            Kind = 1;
            getNextToken();
            break;
        case Pudl::Lexer::tok_binary:
            getNextToken();
            if (!isascii(Pudl::Parser::CurTok))
                return LogErrorP("Expected binary operator");
            FnName = "binary";
            FnName += (char) Pudl::Parser::CurTok;
            Kind = 2;
            getNextToken();

            // Read the precedence if present.
            if (Pudl::Parser::CurTok == Pudl::Lexer::tok_number) {
                if (Pudl::Lexer::NumVal < 1 || Pudl::Lexer::NumVal > 100)
                    return LogErrorP("Invalid precedence: must be 1..100");
                BinaryPrecedence = (unsigned) Pudl::Lexer::NumVal;
                getNextToken();
            }
            break;
    }

    if (Pudl::Parser::CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> ArgNames;
    while (getNextToken() == Pudl::Lexer::tok_identifier)
        ArgNames.push_back(Pudl::Lexer::IdentifierStr);
    if (Pudl::Parser::CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

    // success.
    getNextToken(); // eat ')'.

    // Verify right number of names for operator.
    if (Kind && ArgNames.size() != Kind)
        return LogErrorP("Invalid number of operands for operator");

    return std::make_unique<Pudl::AST::PrototypeAST>(FnName, ArgNames, Kind != 0,
                                                     BinaryPrecedence);
}

std::unique_ptr<Pudl::AST::FunctionAST> Pudl::Parser::ParseDefinition() {
    getNextToken(); // eat def.
    auto Proto = ParsePrototype();
    if (!Proto)
        return nullptr;

    if (auto E = ParseExpression())
        return std::make_unique<Pudl::AST::FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

std::unique_ptr<Pudl::AST::FunctionAST> Pudl::Parser::ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        // Make an anonymous proto.
        auto Proto = std::make_unique<Pudl::AST::PrototypeAST>("__anon_expr",
                                                               std::vector<std::string>());
        return std::make_unique<Pudl::AST::FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

std::unique_ptr<Pudl::AST::PrototypeAST> Pudl::Parser::ParseExtern() {
    getNextToken(); // eat extern.
    return ParsePrototype();
}