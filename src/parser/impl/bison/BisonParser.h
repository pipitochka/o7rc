#pragma once
#include <parser/IParser.h>

class BisonParser final: public IParser {
public:  
  bool parse(ITokenizer& tz) override;
  ~BisonParser() override = default;
};