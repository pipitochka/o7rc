#pragma once
#include <tokenizer/ITokenizer.h>
#include <istream>
#include <string>
#include <unordered_map>

class HandTokenizer final : public ITokenizer {
public:
    explicit HandTokenizer(std::istream& in);

    Token peek() override;
    Token next() override;

    ~HandTokenizer() override = default;

private:
    Token readOne();
    char peekChar();
    char getChar();
    void skipWhitespace();
    void skipComment();
    Token readIdent();
    Token readNumber();
    Token readString();

    std::istream& in_;
    std::uint32_t line_ = 1;
    std::uint32_t col_ = 1;
    bool hasLA_ = false;
    Token la_{};

    static const std::unordered_map<std::string, TokenType> keywords_;
};
