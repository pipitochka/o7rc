#pragma once


class BinaryExpr;
class UnaryExpr;


class IVisitor {
public:
    virtual ~IVisitor() = default;

    virtual void visit(BinaryExpr &node) = 0;
    virtual void visit(UnaryExpr& expr) = 0;
};
