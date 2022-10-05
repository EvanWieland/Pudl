#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include "Token.h"

class Lexer {
private:
    FILE *file;
    int line, col;
    Token saved;

    void whitespace(char aCh);

    void skipComment();

    Token identifierOrKeyword(char aBegin);

    std::string identifier(char aBegin);

    Token number(char aBegin);

public:
    explicit Lexer(FILE *file) : saved(EOF_TOKEN, 0, 0) {
        this->file = file;
        line = 1;
        col = 1;
    }

    ~Lexer();

    Token lex();

    void unlex(Token aToken);
};