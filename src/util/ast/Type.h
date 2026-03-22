#pragma once
#include <util/ast/IVisitor.h>
#include <util/ast/Node.h>


#include <string>
#include <vector>

// used
struct NamedType final : TypeNode {
    std::string name;
    void accept(IVisitor &v) override { v.visit(*this); }
};

// used
struct PointerType final : TypeNode {
    TypePtr baseType;
    void accept(IVisitor &v) override { v.visit(*this); }
};

// used
struct ArrayType final : TypeNode {
    std::vector<ExprPtr> length;
    TypePtr elemType;
    void accept(IVisitor &v) override { v.visit(*this); }
};

// used
struct FieldDecl final : Node {
    std::vector<std::string> names;
    TypePtr type;
    void accept(IVisitor &v) override { v.visit(*this); }
};

// used
struct RecordType final : TypeNode {
    TypePtr baseType;
    std::vector<Ptr<FieldDecl>> fields;
    void accept(IVisitor &v) override { v.visit(*this); }
};

// used
struct ProcParams final : Node {
    bool isVar = false;
    std::vector<std::string> names;
    TypePtr type;
    void accept(IVisitor &v) override { v.visit(*this); }
};

// used
struct ProcType final : TypeNode {
    Ptr<NamedType> type;
    std::vector<Ptr<ProcParams>> params;
    void accept(IVisitor &v) override { v.visit(*this); }
};
