#pragma once
#include <util/ast/IVisitor.h>
#include <util/ast/Node.h>

#include <string>
#include <vector>

//used
struct ConstDecl final : Decl {
    std::string name;
    Ptr<Expr> value;
    void accept(IVisitor &v) override { v.visit(*this); }
};

//used
struct TypeDecl final : Decl {
    std::string name;
    TypePtr type;
    void accept(IVisitor &v) override { v.visit(*this); }
};

//used
struct VarDecl final : Decl {
    std::vector<std::string> names;
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
    TypePtr type;
    std::vector<DeclPtr> params;
    ExprPtr returnValue; 
    std::vector<StmtPtr> body;

    void accept(IVisitor &v) override { v.visit(*this); }
};