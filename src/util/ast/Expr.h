#pragma once

#include <memory>
#include "Ast.h"

class Expr : public Node {
public:
    virtual ~Expr() = default;
};

using ExprPtr = std::unique_ptr<Expr>;

enum class UnOp { Neg, Not };

class UnaryExpr final: public Expr {
public:
    UnOp op;
    ExprPtr operand;

    UnaryExpr(const UnOp op, ExprPtr operand) : op(op), operand(std::move(operand)) {}

    void accept(IVisitor &v) override;
};

enum class BinOp {
    Add, Sub, Mul,
    RealDiv, IntDiv, Mod,
    And, Or,
    Eq, Neq, Lt, Le, Gt, Ge,
    In,
    Is
};

class BinaryExpr final: public Expr {
public:
    BinOp op;
    ExprPtr lhs;
    ExprPtr rhs;

    BinaryExpr(const BinOp op, ExprPtr lhs, ExprPtr rhs) : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    void accept(IVisitor &v) override;
};