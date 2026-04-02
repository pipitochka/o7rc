#pragma once
#include "IPass.h"

class CopyPropagation : public IPass {
public:
    std::string name() const override { return "CopyPropagation"; }
    bool run(IRFunction& fn) override;
};
