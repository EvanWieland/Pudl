#include "ASTVisitor.h"

OperatorKind binaryOperatorFromLexeme(const std::string &aLexeme) {
    if (aLexeme == "+") return OperatorKind::Add;
    if (aLexeme == "-") return OperatorKind::Sub;
    if (aLexeme == "*") return OperatorKind::Mul;
    if (aLexeme == "/") return OperatorKind::Div;
    if (aLexeme == "==") return OperatorKind::Eq;
    if (aLexeme == "!=") return OperatorKind::Ne;
    if (aLexeme == ">") return OperatorKind::Gt;
    if (aLexeme == "<") return OperatorKind::Lt;
    if (aLexeme == ">=") return OperatorKind::Ge;
    if (aLexeme == "<=") return OperatorKind::Le;
    if (aLexeme == "&&") return OperatorKind::And;
    if (aLexeme == "||") return OperatorKind::Or;
    return OperatorKind::Add;
}

OperatorKind unaryOperatorFromLexeme(const std::string &aLexeme) {
    if (aLexeme == "-") return OperatorKind::Neg;
    if (aLexeme == "+") return OperatorKind::Pos;
    if (aLexeme == "!") return OperatorKind::Not;
    return OperatorKind::Pos;
}

std::string showOperator(OperatorKind aOp) {
    switch (aOp) {
        case OperatorKind::Add:
            return "+";
        case OperatorKind::Sub:
            return "-";
        case OperatorKind::Mul:
            return "*";
        case OperatorKind::Div:
            return "/";
        case OperatorKind::Eq:
            return "==";
        case OperatorKind::Ne:
            return "!=";
        case OperatorKind::Gt:
            return ">";
        case OperatorKind::Lt:
            return "<";
        case OperatorKind::Ge:
            return ">=";
        case OperatorKind::Le:
            return "<=";
        case OperatorKind::And:
            return "&&";
        case OperatorKind::Or:
            return "||";
        case OperatorKind::Pos:
            return "+";
        case OperatorKind::Neg:
            return "-";
        case OperatorKind::Not:
            return "!";
    }
    return "?";
}

void VectorNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void DummyNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void VarNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void FuncallNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void BooleanNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void IntegerNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void FloatNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void AssignmentNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void BinaryNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void UnaryNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void FunctionDefNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void BlockStatementNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void IfStatementNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void WhileStatementNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void DoWhileStatementNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void ExpressionWrapperNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void IoPrintNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}

void ReturnNode::accept(ASTVisitor &aVisitor) {
    aVisitor.visit((*this));
}