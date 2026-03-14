#pragma once
#include <tokenizer/ITokenizer.h>
#include <util/ast/AstFwd.h>

#include <memory>
#include <string>

struct Module;

struct ParserContext {
    ITokenizerPtr tz = nullptr;
    ModulePtr module;
    std::string lastError;
};
