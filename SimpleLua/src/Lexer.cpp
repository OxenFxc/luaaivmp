#include "Lexer.h"
#include <cctype>

Lexer::Lexer(const std::string& source) : source(source), pos(0), line(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos < (int)source.length()) {
        skipWhitespace();
        if (pos >= (int)source.length()) break;

        char c = peek();
        if (isdigit(c)) {
            tokens.push_back(number());
        } else if (isalpha(c) || c == '_') {
            tokens.push_back(identifier());
        } else if (c == '"') {
            tokens.push_back(string());
        } else {
            tokens.push_back(scanToken());
        }
    }
    tokens.push_back({TokenType::END_OF_FILE, "", line});
    return tokens;
}

void Lexer::skipWhitespace() {
    while (pos < (int)source.length()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            line++;
            advance();
        } else {
            break;
        }
    }
}

char Lexer::peek() {
    if (pos >= (int)source.length()) return '\0';
    return source[pos];
}

char Lexer::advance() {
    if (pos >= (int)source.length()) return '\0';
    return source[pos++];
}

bool Lexer::match(char expected) {
    if (peek() == expected) {
        advance();
        return true;
    }
    return false;
}

Token Lexer::scanToken() {
    char c = advance();
    switch (c) {
        case '=':
            if (match('=')) return {TokenType::EQ, "==", line};
            return {TokenType::ASSIGN, "=", line};
        case '~':
            if (match('=')) return {TokenType::NE, "~=", line};
            return {TokenType::UNKNOWN, "~", line};
        case '<':
            if (match('=')) return {TokenType::LE, "<=", line};
            return {TokenType::LT, "<", line};
        case '>':
            if (match('=')) return {TokenType::GE, ">=", line};
            return {TokenType::GT, ">", line};
        case '+': return {TokenType::PLUS, "+", line};
        case '-': return {TokenType::MINUS, "-", line};
        case '*': return {TokenType::MUL, "*", line};
        case '/': return {TokenType::DIV, "/", line};
        case '(': return {TokenType::LPAREN, "(", line};
        case ')': return {TokenType::RPAREN, ")", line};
        case '{': return {TokenType::LBRACE, "{", line};
        case '}': return {TokenType::RBRACE, "}", line};
        case '[': return {TokenType::LBRACKET, "[", line};
        case ']': return {TokenType::RBRACKET, "]", line};
        case ',': return {TokenType::COMMA, ",", line};
        case '.': return {TokenType::DOT, ".", line};
        case ':': return {TokenType::COLON, ":", line};
        case ';': return {TokenType::SEMICOLON, ";", line};
        default:
            return {TokenType::UNKNOWN, std::string(1, c), line};
    }
}

Token Lexer::identifier() {
    std::string text;
    while (pos < (int)source.length() && (isalnum(peek()) || peek() == '_')) {
        text += advance();
    }

    if (text == "local") return {TokenType::LOCAL, text, line};
    if (text == "print") return {TokenType::PRINT, text, line};
    if (text == "if") return {TokenType::IF, text, line};
    if (text == "then") return {TokenType::THEN, text, line};
    if (text == "else") return {TokenType::ELSE, text, line};
    if (text == "end") return {TokenType::END, text, line};
    if (text == "while") return {TokenType::WHILE, text, line};
    if (text == "do") return {TokenType::DO, text, line};
    if (text == "function") return {TokenType::FUNCTION, text, line};
    if (text == "return") return {TokenType::RETURN, text, line};

    return {TokenType::ID, text, line};
}

Token Lexer::number() {
    std::string text;
    while (isdigit(peek())) {
        text += advance();
    }
    if (peek() == '.') {
        text += advance();
        while (isdigit(peek())) {
            text += advance();
        }
    }
    return {TokenType::NUMBER, text, line};
}

Token Lexer::string() {
    advance(); // Skip opening "
    std::string text;
    while (peek() != '"' && peek() != '\0') {
        text += advance();
    }
    if (peek() == '"') advance(); // Skip closing "
    return {TokenType::STRING, text, line};
}
