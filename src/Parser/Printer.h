#pragma once

#include <iostream>

#include "AST/ASTVisitor.h"

class Printer : public ASTVisitor {
private:
    std::string show(TType aType) {
        switch (aType) {
            case TType::UNDEFINED:
                return "undef";
            case TType::BOOL:
                return "bool";
            case TType::INTEGER:
                return "int";
            case TType::FLOAT:
                return "float";
            default:
                return "unknown";
        }
    }

public:
    Printer() {}

    // [node]*
    void visit(VectorNode aNode) {
        for (Node *node: aNode.getNodes()) {
            node->accept((*this));
        }
    }

    void visit(DummyNode aNode) {
        std::cout << " [dummy]";
    }

    // [<type> <name>]
    void visit(VarNode aNode) {
        std::cout << " [" << show(aNode.getType())
                  << " " << aNode.getName() << "]";
    }

    // [<value>I]
    void visit(IntegerNode aNode) {
        std::cout << " [" << aNode.getValue() << "I]";
    }

    // [True] or [False]
    void visit(BooleanNode aNode) {
        if (aNode.getValue()) {
            std::cout << " [True]";
        } else {
            std::cout << "[False]";
        }
    }

    // [<value>F]
    void visit(FloatNode aNode) {
        std::cout << " [" << aNode.getValue() << "F]";
    }

    // (Assign <variable> <expression>)
    void visit(AssignmentNode aNode) {
        std::cout << " (Assign ";
        aNode.getLHS()->accept((*this));
        aNode.getRHS()->accept((*this));
        std::cout << ")";
    }

    // (<operator> <lhs> <rhs>)
    void visit(BinaryNode aNode) {
        std::cout << " (" << aNode.getOp() << " ";
        aNode.getLHS()->accept((*this));
        aNode.getRHS()->accept((*this));
        std::cout << ")";
    }

    // (<operator> <subexpr>)
    void visit(UnaryNode aNode) {
        std::cout << " (" << aNode.getOp() << " ";
        aNode.getSubexpr()->accept((*this));
        std::cout << ")";
    }

    // (Call <name> : ( [arg]* ) -> <type>)
    void visit(FuncallNode aNode) {
        std::cout << " (Call " << aNode.getName() << " : (";
        for (ExpressionNode *node: aNode.getArgs()) {
            node->accept((*this));
        }
        std::cout << " ) -> " << show(aNode.getType()) << ")";
    }

    // (Func <name> : ( [arg]* ) -> <type> <body>)
    void visit(FunctionDefNode aNode) {
        std::cout << " (Func " << aNode.getName() << " : (";
        for (VarNode *node: aNode.getArgs()) {
            node->accept((*this));
        }

        std::cout << " ) -> " << show(aNode.getType());
        aNode.getBody()->accept((*this));
        std::cout << ")";
    }

    // { [statement]* }
    void visit(BlockStatementNode aNode) {
        std::cout << " {";
        for (StatementNode *node: aNode.getStatements()) {
            node->accept((*this));
            std::cout << ";";
        }
        std::cout << " }";
    }

    // (If <expression> <statement> (Else <statement>)?
    void visit(IfStatementNode aNode) {
        std::cout << " (If ";
        aNode.getCond()->accept((*this));
        aNode.getTrueBranch()->accept((*this));
        StatementNode *falseBranch = aNode.getFalseBranch();
        if (falseBranch != NULL) {
            std::cout << " Else ";
            falseBranch->accept((*this));
        }
        std::cout << " )";
    }

    // (While <expression> <statement>)
    void visit(WhileStatementNode aNode) {
        std::cout << " (While ";
        aNode.getCond()->accept((*this));
        aNode.getBody()->accept((*this));
        std::cout << " )";
    }

    // (Do <statement> While <expression>)
    void visit(DoWhileStatementNode aNode) {
        std::cout << " (Do ";
        aNode.getBody()->accept((*this));
        std::cout << " While ";
        aNode.getCond()->accept((*this));
        std::cout << " )";
    }

    // <expression>
    void visit(ExpressionWrapperNode aNode) {
        aNode.getExpr()->accept((*this));
    }

    // (Print <expression>)
    void visit(IoPrintNode aNode) {
        std::cout << " (Print ";
        aNode.getSubexpr()->accept((*this));
        std::cout << ")";
    }

    // (Ret <expression>)
    void visit(ReturnNode aNode) {
        std::cout << " (Ret ";
        aNode.getSubexpr()->accept((*this));
        std::cout << ")";
    }
};