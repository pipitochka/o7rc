#include "BufferedTokenizer.h"

#include <iostream>
#include <stdio.h>

BufferedTokenizer::BufferedTokenizer(ITokenizerPtr tokenizer) {
    while (true) {
        Token t = tokenizer->next();
        tokens.push_back(t);
        if (t.type == TokenType::Eof)
            break;
    }
}

Token BufferedTokenizer::peek() {
    if (pos < tokens.size()) {
        return tokens[pos];
    }
    if (!tokens.empty()) {
        return tokens.back();
    }
    return Token{TokenType::Eof, "", 0, 0};
}

Token BufferedTokenizer::next() {
    Token t = peek();
    if (pos < tokens.size()) {
        ++pos;
    }
    return t;
}

void BufferedTokenizer::check() {
    for (auto &token: tokens) {
        if (token.type == TokenType::Unknown) {
            std::string msg = "Lexical error at line " + std::to_string(token.line) + ", col " +
                              std::to_string(token.col) + ": Unknown token '" + token.text + "'";
            throw std::runtime_error(msg);
        }
    }
}

void BufferedTokenizer::print() {
    for (auto &token: tokens) {
        std::cout << token;
    }
}
