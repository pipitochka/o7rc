#pragma once
#include <memory>
#include <ostream>
#include <util/ast/AstFwd.h>

struct ICodeGen {
    virtual ~ICodeGen() = default;
    virtual void generate(Module& module, std::ostream& out) = 0;
};

using ICodeGenPtr = std::unique_ptr<ICodeGen>;
