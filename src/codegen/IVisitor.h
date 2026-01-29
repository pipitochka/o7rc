#pragma once

#include <util/Ast.h>

class Node;

class IVisitor {
public:
    virtual ~IVisitor() = default;

    virtual void visit(Node &node) = 0;
};
