#include "BufferedTokenizer.h"

BufferedTokenizer::BufferedTokenizer(std::unique_ptr<ITokenizer> tokenizer) {
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
    return Token{TokenType::Eof, "", 0, 0};
}

Token BufferedTokenizer::next() {
    Token t = peek();
    if (pos < tokens.size()) {
        ++pos;
    }
    return t;
}
