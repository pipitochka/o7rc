#pragma once
#include <util/Token.h>

#include <memory>


class ITokenizer {
public:
    virtual Token peek() = 0;
    virtual Token next() = 0;

    virtual ~ITokenizer() = default;
};

using ITokenizerPtr = std::shared_ptr<ITokenizer>;
