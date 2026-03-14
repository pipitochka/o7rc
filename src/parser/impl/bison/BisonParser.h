#pragma once
#include <parser/IParser.h>

class BisonParser final : public IParser {
public:
    BisonParser() = default;
    ModulePtr parse(ITokenizerPtr tz) override;
    ~BisonParser() override = default;
};
