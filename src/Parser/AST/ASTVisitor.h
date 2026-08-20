#pragma once

#include "AST.h"

class ASTVisitor {
public:
    // Nodes are taken by reference, not by value: they're arena-owned (see
    // Arena.h) and live for the whole compilation, so a visitor is free to
    // keep a pointer into one (e.g. Codegen.h's currentFunc) past the end
    // of a single visit() call -- taking nodes by value would silently
    // hand out a pointer to a stack-local copy instead.
    virtual void visit(VectorNode &aNode) = 0;

    virtual void visit(DummyNode &aNode) = 0;

    virtual void visit(VarNode &aNode) = 0;

    virtual void visit(FuncallNode &aNode) = 0;

    virtual void visit(BooleanNode &aNode) = 0;

    virtual void visit(IntegerNode &aNode) = 0;

    virtual void visit(FloatNode &aNode) = 0;

    virtual void visit(BinaryNode &aNode) = 0;

    virtual void visit(UnaryNode &aNode) = 0;

    virtual void visit(AssignmentNode &aNode) = 0;

    virtual void visit(FunctionDefNode &aNode) = 0;

    virtual void visit(BlockStatementNode &aNode) = 0;

    virtual void visit(IfStatementNode &aNode) = 0;

    virtual void visit(WhileStatementNode &aNode) = 0;

    virtual void visit(DoWhileStatementNode &aNode) = 0;

    virtual void visit(ExpressionWrapperNode &aNode) = 0;

    virtual void visit(IoPrintNode &aNode) = 0;

    virtual void visit(ReturnNode &aNode) = 0;
};