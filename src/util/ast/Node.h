#pragma once

#include <cstdint>
#include <memory>
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

template<class T>
using Ptr = std::unique_ptr<T>;

using NodePtr = Ptr<Node>;
using ExprPtr = Ptr<Expr>;
using StmtPtr = Ptr<Stmt>;
using DeclPtr = Ptr<Decl>;
using TypePtr = Ptr<TypeNode>;
using SelectorPtr = Ptr<Selector>;
