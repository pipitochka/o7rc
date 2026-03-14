#pragma once
#include <tokenizer/ITokenizer.h>

#include <memory>
#include <vector>


class BufferedTokenizer : public ITokenizer {
public:
    explicit BufferedTokenizer(ITokenizerPtr tokenizer);
    Token next() override;
    Token peek() override;
    void check();

    void print();

private:
    std::vector<Token> tokens;
    std::size_t pos = 0;
};
