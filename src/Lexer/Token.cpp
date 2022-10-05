#include "Token.h"

Token::Token(TokenType type, const std::string& lexeme, int line, int col) {
    this->type = type;
    this->lexeme = lexeme;
    this->line = line;
    this->col = col;
}

Token::Token(TokenType type, int line, int col) {
    this->type = type;
    this->lexeme = "";
    this->line = line;
    this->col = col;
}

TokenType Token::getType() { return type; }

std::string Token::getLexeme() { return lexeme; }

int Token::getLine() const { return line; }

int Token::getColumn() const { return col; }

std::string Token::showType(TokenType aType) {
    switch (aType) {
        case EOF_TOKEN:
            return "EOF";
        case SYMBOL:
            return "SYMBOL";
        case INTEGER:
            return "INT";
        case FLOAT:
            return "FLOAT";
        case BOOL:
            return "BOOL";
        case LAND:
            return "And";
        case LOR:
            return "Or";
        case LXOR:
            return "Xor";
        case ADD:
            return "Add";
        case MUL:
            return "Mul";
        case ASSIGN:
            return "Assign";
        case CMP:
            return "Relation";
        case CMP_EQ:
            return "Equality Relation";
        case FUNC:
            return "Func";
        case IF:
            return "If";
        case ELSE:
            return "Else";
        case DO:
            return "Do";
        case WHILE:
            return "While";
        case RETURN:
            return "Return";
        case IO_PRINT:
            return "Print";
        case IO_READ:
            return "Read";
        case PL:
            return "Open Parenthesis";
        case PR:
            return "Close Parenthesis";
        case BL:
            return "Open Brace";
        case BR:
            return "Close Brace";
        case COMMA:
            return "Comma";
        case SEMICOLON:
            return "Semicolon";
        case COLON:
            return "Colon";
        case TYPE:
            return "Type";
        default:
            return "ERR";
    }
}