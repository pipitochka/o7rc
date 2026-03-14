#pragma once

#include "AstFwd.h"

#include <memory>
#include <string>
#include <tokenizer/ITokenizer.h>

struct Module; 

struct ParserContext {
    ITokenizerPtr tz = nullptr;
    ModulePtr module;
    std::string lastError;
};