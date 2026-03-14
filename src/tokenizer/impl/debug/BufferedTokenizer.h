#pragma once

#include <vector>
#include <memory>
#include <tokenizer/ITokenizer.h>


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
