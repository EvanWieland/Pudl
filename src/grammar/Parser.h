//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

#pragma once

#include <map>
#include <cctype>
#include <memory>
#include <iostream>
#include <vector>

#include "AST.h"
#include "Lexer.h"

namespace Pudl {
    class Parser {
    public:
        /// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
        /// token the parser is looking at.  getNextToken reads another token from the
        /// lexer and updates CurTok with its results.
        static int CurTok;

        /// BinopPrecedence - This holds the precedence for each binary operator that is
        /// defined.
        static std::map<char, int> BinopPrecedence;

        /// LogError* - These are little helper functions for error handling.
        static std::unique_ptr<Pudl::AST::ExprAST> LogError(const char *);

        /// definition ::= 'def' prototype expression
        static std::unique_ptr<Pudl::AST::FunctionAST> ParseDefinition();

        static int getNextToken();

        /// external ::= 'extern' prototype
        static std::unique_ptr<Pudl::AST::PrototypeAST> ParseExtern();

        /// toplevelexpr ::= expression
        static std::unique_ptr<Pudl::AST::FunctionAST> ParseTopLevelExpr();

    private:
        /// GetTokPrecedence - Get the precedence of the pending binary operator token.
        static int GetTokPrecedence();

        static std::unique_ptr<Pudl::AST::PrototypeAST> LogErrorP(const char *);

        /// numberexpr ::= number
        static std::unique_ptr<Pudl::AST::ExprAST> ParseNumberExpr();

        /// parenexpr ::= '(' expression ')'
        static std::unique_ptr<Pudl::AST::ExprAST> ParseParenExpr();

        /// identifierexpr
        ///   ::= identifier
        ///   ::= identifier '(' expression* ')'
        static std::unique_ptr<Pudl::AST::ExprAST> ParseIdentifierExpr();

        /// ifexpr ::= 'if' expression 'then' expression 'else' expression
        static std::unique_ptr<Pudl::AST::ExprAST> ParseIfExpr();

        /// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
        static std::unique_ptr<Pudl::AST::ExprAST> ParseForExpr();

        /// varexpr ::= 'var' identifier ('=' expression)?
        //                    (',' identifier ('=' expression)?)* 'in' expression
        static std::unique_ptr<Pudl::AST::ExprAST> ParseVarExpr();

        /// primary
        ///   ::= identifierexpr
        ///   ::= numberexpr
        ///   ::= parenexpr
        ///   ::= ifexpr
        ///   ::= forexpr
        ///   ::= varexpr
        static std::unique_ptr<Pudl::AST::ExprAST> ParsePrimary();

        /// unary
        ///   ::= primary
        ///   ::= '!' unary
        static std::unique_ptr<Pudl::AST::ExprAST> ParseUnary();

        /// binoprhs
        ///   ::= ('+' unary)*
        static std::unique_ptr<Pudl::AST::ExprAST> ParseBinOpRHS(int, std::unique_ptr<Pudl::AST::ExprAST>);

        /// expression
        ///   ::= unary binoprhs
        static std::unique_ptr<Pudl::AST::ExprAST> ParseExpression();

        /// prototype
        ///   ::= id '(' id* ')'
        ///   ::= binary LETTER number? (id, id)
        ///   ::= unary LETTER (id)
        static std::unique_ptr<Pudl::AST::PrototypeAST> ParsePrototype();
    };
} // namespace Pudl