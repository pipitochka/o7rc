#pragma once
#include <tokenizer/ITokenizer.h>
#include <util/Ast.h>

class IParser {
public:
    virtual bool parse(ITokenizer &tz) = 0;
    virtual ~IParser() = default;
};
