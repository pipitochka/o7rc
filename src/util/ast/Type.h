#pragma once
#include <util/ast/IVisitor.h>
#include <util/ast/Node.h>


#include <string>
#include <vector>

struct NamedType final : TypeNode {
    std::string name;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct PointerType final : TypeNode {
    TypePtr baseType;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct ArrayType final : TypeNode {
    ExprPtr length; // nullptr => open array (если решишь)
    TypePtr elemType;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct FieldDecl final : Node {
    std::vector<std::string> names;
    TypePtr type;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct RecordType final : TypeNode {
    std::vector<Ptr<FieldDecl>> fields;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct ProcType final : TypeNode {
    std::vector<Ptr<ParamDecl>> params; // ParamDecl объявлен в AstFwd
    TypePtr returnType; // nullptr => no result
    void accept(IVisitor &v) override { v.visit(*this); }
};
