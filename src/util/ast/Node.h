#pragma once

#include <cstdint>
#include "AstFwd.h"

struct SourcePos {
    std::uint32_t line = 1;
    std::uint32_t col  = 1;
};

struct SourceRange {
    SourcePos begin;
    SourcePos end;
};

struct Node {
    SourceRange range{};
    virtual ~Node() = default;
    virtual void accept(IVisitor& v) = 0;
};

struct Expr : Node {};
struct Stmt : Node {};
struct Decl : Node {};
struct TypeNode : Node {};
struct Selector : Node {};
