#include "Expr.h"

void UnaryExpr::accept(IVisitor& v) {
    v.visit(*this);
}

void BinaryExpr::accept(IVisitor& v) {
    v.visit(*this);
}