#pragma once
#include "IPass.h"

class ConstantFolding : public IPass {
public:
    std::string name() const override { return "ConstantFolding"; }
    bool run(IRFunction& fn) override;
};
