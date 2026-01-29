#pragma once
#include <FlexLexer.h>
#include <tokenizer/ITokenizer.h>

class FlexTokenizer final : public ITokenizer {
public:
    explicit FlexTokenizer(std::istream &in);

    Token peek() override;
    Token next() override;

    ~FlexTokenizer() override = default;

private:
    Token readOne();

    yyFlexLexer lexer;
    bool hasLA = false;
    Token la{};
};
