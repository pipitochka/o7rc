#pragma once
#include <util/ast/Node.h>
#include <util/ast/IVisitor.h>

#include <cstdint>
#include <string>
#include <vector>

struct FieldSelector final : Selector {
    std::string name;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct IndexSelector final : Selector {
    ExprPtr index; 
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct DerefSelector final : Selector {
    void accept(IVisitor& v) override { v.visit(*this); }
};