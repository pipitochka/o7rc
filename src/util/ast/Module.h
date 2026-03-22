#pragma once
#include <util/ast/IVisitor.h>
#include <util/ast/Node.h>

#include <optional>
#include <string>
#include <vector>

struct Import final : Node {
    std::string name;
    std::optional<std::string> alias;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct Module final : Node {
    std::string name;
    std::string endName;

    std::vector<Import> imports;
    std::vector<DeclPtr> decls;
    std::vector<StmtPtr> block;


    void accept(IVisitor &v) override { v.visit(*this); }
};
