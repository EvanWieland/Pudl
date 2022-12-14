#pragma once

#include <string>

enum TokenType {
    EOF_TOKEN,
    ERROR_TOKEN,

    SYMBOL,
    INTEGER, FLOAT, BOOL,

    ADD, MUL,
    LAND, LOR, LXOR, NOT,
    ASSIGN,
    CMP, CMP_EQ,

    FUNC,
    IF, ELSE,
    DO, WHILE,
    RETURN,
    IO_PRINT, IO_READ,

    PL, PR, BL, BR,
    COMMA, SEMICOLON, COLON,

    TYPE
};

class Token {
private:
    TokenType type;
    std::string lexeme;
    int line, col;

public:
    Token(TokenType aType, const std::string &aLexeme, int line, int col);

    Token(TokenType aType, int line, int col);

    TokenType getType();

    std::string getLexeme();

    static std::string showType(TokenType aType);

    int getLine() const;

    int getColumn() const;
};