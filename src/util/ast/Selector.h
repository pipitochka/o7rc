#pragma once
#include <util/ast/IVisitor.h>
#include <util/ast/Node.h>

#include <cstdint>
#include <string>
#include <vector>

struct FieldSelector final : Selector {
    std::string name;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct IndexSelector final : Selector {
    std::vector<ExprPtr> index;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct DerefSelector final : Selector {
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct TypeGuardSelector final : Selector {
    std::string typeName;  
    void accept(IVisitor &v) override { v.visit(*this); }
};