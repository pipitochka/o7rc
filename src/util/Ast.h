#pragma once

#include <codegen/IVisitor.h>

enum class NodeKind {

};

class Node {
public:
    virtual ~Node() = default;
    explicit Node(NodeKind kind);
    virtual void accept(IVisitor &v) = 0;
};
