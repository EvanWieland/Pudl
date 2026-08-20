#pragma once

#include <string>
#include <iostream>
#include <map>
#include <memory>

#include "../Lexer/Lexer.h"
#include "AST/ASTVisitor.h"
#include "AST/Arena.h"

class Parser {
private:
    // unique_ptr, not a raw owning pointer: Lexer's destructor closes the
    // FILE* it was given, and this was previously leaked (`new Lexer`,
    // never `delete`d) along with that open file handle. Tying it to the
    // Parser's own lifetime fixes both without needing to hook every
    // return point in parse().
    std::unique_ptr<Lexer> lexer;
    Token current;
    bool isDebugMode;
    bool isSuccess;
    bool isError;

    // Owns every AST node this Parser produces -- they live as long as
    // this Parser does (see Arena.h). Declared before any parsing method
    // runs and destroyed only when the Parser itself is (main() keeps the
    // Parser alive through printing/codegen), so pointers into it -- like
    // Codegen.h's currentFunc -- stay valid for the whole compilation.
    Arena arena;

    std::map<std::string, VarNode *> scope;
    std::map<std::string, FunctionDefNode *> funcs;

    std::string show(const TType aType) {
        switch (aType) {
            case TType::INTEGER:
                return "Int";
            case TType::FLOAT:
                return "Float";
            default:
                return "Undefined";
        }
    }

    Token next() { return current = lexer->lex(); }

    void unlex(Token aToken) { lexer->unlex(aToken); }

    bool is(Token aToken, TokenType aExpectedType, bool aSuppress = false);

    TType fromString(std::string aType);

    void info(std::string aMsg) {
        if (isDebugMode) {
            std::cout << aMsg;
        }
    }

    void infoln(std::string aMsg = "") {
        if (isDebugMode) {
            std::cout << aMsg << std::endl;
        }
    }

    void lexinfo(std::string aLexeme) {
        if (isDebugMode) {
            std::cout << "lex!: " << aLexeme << std::endl;
        }
    }

    void lexinfo(std::string aLexeme, TokenType aType) {
        if (isDebugMode) {
            std::cout << "lex!: " << aLexeme << " of "
                      << Token::showType(aType) << std::endl;
        }
    }

    void lexinfo(Token aToken) {
        if (isDebugMode) {
            std::cout << "lex!: " << aToken.getLexeme() << " of "
                      << Token::showType(aToken.getType()) << " at ("
                      << aToken.getLine() << ":" << aToken.getColumn() << ")"
                      << std::endl;
        }
    }

    void error(int aLine, std::string aMessage) {
        std::cout << "ERROR: " << aMessage
                  << " at " << aLine << " line"
                  << std::endl;
        isSuccess = false;
        isError = true;
    }

    void errorty(int aLine, TType aSrc, TType aDst) {
        std::cout << "ERROR: can't cast from " << show(aSrc)
                  << " to " << show(aDst)
                  << " at " << aLine << " line" << std::endl;
        isError = true;
    }

    FunctionDefNode *functionDef();

    std::vector<VarNode *> functionArgs();

    ExpressionWrapperNode *_funcall();

    FuncallNode *funcall();

    std::vector<ExpressionNode *> funcallArgs();

    StatementNode *statement();

    BlockStatementNode *blockStatement();

    IfStatementNode *ifStatement();

    WhileStatementNode *whileStmt();

    DoWhileStatementNode *doWhileStmt();

    AssignmentNode *declaration();

    AssignmentNode *assignment();

    IoPrintNode *ioPrint();

    ReturnNode *ret();

    ExpressionNode *expression();

    ExpressionNode *lor();

    ExpressionNode *land();

    ExpressionNode *cmpeq();

    ExpressionNode *cmp();

    ExpressionNode *additive();

    ExpressionNode *multiplicative();

    ExpressionNode *unary();

    ExpressionNode *factor();

    ExpressionNode *constant();

    BooleanNode *boolean();

    IntegerNode *intgr();

    FloatNode *flt();

    VarNode *var();

public:
    Parser(bool debug = false) : current(EOF_TOKEN, 0, 0) {
        isDebugMode = debug;
        isSuccess = true;
        isError = false;
    }

    bool isFailed() { return isError; }

    Node *parse(FILE *aFile);
};