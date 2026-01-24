#pragma once
#include <parser/IParser.h>

class BisonParser final: public IParser {
public:
  BisonParser() = default;
  bool parse(ITokenizer& tz) override;
  ~BisonParser() override = default;
};