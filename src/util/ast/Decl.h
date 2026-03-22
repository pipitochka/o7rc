#pragma once
#include <util/ast/IVisitor.h>
#include <util/ast/Node.h>

#include <string>
#include <vector>

// used
struct ConstDecl final : Decl {
    std::string name;
    bool exported = false;
    Ptr<Expr> value;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct TypeDecl final : Decl {
    std::string name;
    bool exported = false;
    TypePtr type;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct VarDecl final : Decl {
    std::vector<std::string> names;
    std::vector<bool> exportedFlags;
    TypePtr type;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct ParamDecl final : Decl {
    bool isVar = false;
    std::vector<std::string> names;
    TypePtr type;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct ProcDecl final : Decl {
    std::string name;
    bool exported = false;
    TypePtr type;
    std::vector<DeclPtr> decls;
    ExprPtr returnValue;
    std::vector<StmtPtr> body;

    void accept(IVisitor &v) override { v.visit(*this); }
};
