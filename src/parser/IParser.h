#pragma once
#include <tokenizer/ITokenizer.h>
#include <util/ast/AstFwd.h>

class IParser {
public:
    virtual ModulePtr parse(ITokenizerPtr tz) = 0;
    virtual ~IParser() = default;
};
