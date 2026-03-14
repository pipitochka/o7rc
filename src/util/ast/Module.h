#pragma once
#include <util/ast/Node.h>
#include <util/ast/IVisitor.h>

#include <optional>
#include <string>
#include <vector>

struct Import final : Node {
    std::string name;
    std::optional<std::string> alias;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct ProcDecl final : Decl {
    std::string name;

    std::vector<Ptr<ParamDecl>> params;
    TypePtr returnType;   // nullptr => no result
    Ptr<Block> body;

    void accept(IVisitor& v) override { v.visit(*this); }
};

struct Module final : Node {
    std::string name;
    std::string endName;

    std::vector<Import> imports;
    Ptr<Block> block;

    void accept(IVisitor& v) override { v.visit(*this); }
};
