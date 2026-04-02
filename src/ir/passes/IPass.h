#pragma once
#include <ir/Function.h>
#include <string>

class IPass {
public:
    virtual ~IPass() = default;
    virtual std::string name() const = 0;
    virtual bool run(IRFunction& fn) = 0;
};
