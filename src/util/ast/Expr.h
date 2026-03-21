#pragma once
#include <util/ast/IVisitor.h>
#include <util/ast/Node.h>

#include <cstdint>
#include <string>
#include <vector>

struct LiteralExpr final : Expr {
    enum class Kind { Int, Real, String, Bool, Nil } kind = Kind::Int;

    std::int64_t intValue = 0;
    double realValue = 0.0;
    std::string strValue;
    bool boolValue = false;

    static Ptr<LiteralExpr> makeInt(std::int64_t v) {
        auto p = std::make_unique<LiteralExpr>();
        p->kind = Kind::Int;
        p->intValue = v;
        return p;
    }

    void accept(IVisitor &v) override { v.visit(*this); }
};

struct DesignatorExpr final : Expr {
    std::string baseName;
    std::vector<SelectorPtr> selectors;

    void accept(IVisitor &v) override { v.visit(*this); }
};

struct CallExpr final : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;

    void accept(IVisitor &v) override { v.visit(*this); }
};

//used
struct UnaryExpr final : Expr {
    enum class Op { Neg, Not} op = Op::Neg;
    ExprPtr rhs;

    void accept(IVisitor &v) override { v.visit(*this); }
};

struct SetElement {
    ExprPtr low = nullptr;
    ExprPtr high = nullptr;
};

struct SetExpr final : Expr {
    std::vector<SetElement> elements;

    void accept(IVisitor &v) override { v.visit(*this); }
};


//used
struct BinaryExpr final : Expr {
    enum class Op { Or, And, Eq, Neq, Lt, Le, Gt, Ge, Add, Sub, Mul, RDiv, IDiv, Mod, In, Is, None } op = Op::None;

    ExprPtr lhs;
    ExprPtr rhs;

    void accept(IVisitor &v) override { v.visit(*this); }
};
