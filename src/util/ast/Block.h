#pragma once
#include <util/ast/IVisitor.h>
#include <util/ast/Node.h>

#include <vector>

struct Block final : Node {
    std::vector<DeclPtr> decls;
    std::vector<StmtPtr> stmts;

    void accept(IVisitor &v) override { v.visit(*this); }
};
