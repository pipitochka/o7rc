#pragma once

#include <vector>

#include <tokenizer/ITokenizer.h>


class BufferedTokenizer : public ITokenizer {
public:
    explicit BufferedTokenizer(std::unique_ptr<ITokenizer> tokenizer);
    Token next() override;
    Token peek() override;

private:
    std::vector<Token> tokens;
    std::size_t pos = 0;
};
