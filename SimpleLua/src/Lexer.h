#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <iostream>

enum class TokenType {
    LOCAL,
    PRINT,
    ID,
    NUMBER,
    STRING,
    ASSIGN,     // =
    PLUS,       // +
    MINUS,      // -
    MUL,        // *
    DIV,        // /
    LPAREN,     // (
    RPAREN,     // )
    SEMICOLON,  // ;

    // Keywords
    IF, THEN, ELSE, END, WHILE, DO, FUNCTION, RETURN,

    // Symbols
    LBRACE,     // {
    RBRACE,     // }
    LBRACKET,   // [
    RBRACKET,   // ]
    COMMA,      // ,
    DOT,        // .
    COLON,      // :

    // Comparison
    EQ, // ==
    NE, // ~=
    LT, // <
    GT, // >
    LE, // <=
    GE, // >=

    UNKNOWN,    // Error/Unknown char
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source;
    int pos;
    int line;

    char peek();
    char advance();
    bool match(char expected);
    void skipWhitespace();
    Token scanToken();
    Token identifier();
    Token number();
    Token string();
};

#endif
