#pragma once
#include "../util/Token.h"

class  ITokenizer {
public:
    virtual Token peek() = 0;
    virtual Token next() = 0;
    virtual ~ITokenizer() = default;
};
