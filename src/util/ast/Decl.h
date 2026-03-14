#pragma once
#include <util/ast/Node.h>
#include <util/ast/IVisitor.h>

#include <string>
#include <vector>

struct ConstDecl final : Decl {
    std::string name;
    ExprPtr value;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct TypeDecl final : Decl {
    std::string name;
    TypePtr type;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct VarDecl final : Decl {
    std::vector<std::string> names;
    TypePtr type;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct ParamDecl final : Decl {
    bool isVar = false;
    std::vector<std::string> names;
    TypePtr type;
    void accept(IVisitor& v) override { v.visit(*this); }
};

// ProcDecl зависит от Block => подключим Block.h в ProcDecl.h или ниже после Block.h
