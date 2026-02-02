#pragma once

#include <memory>
#include <util/IVisitor.h>

class Node {
public:
    virtual ~Node() = default;
    virtual void accept(IVisitor &v) = 0;
};

using NodePtr = std::unique_ptr<Node>;