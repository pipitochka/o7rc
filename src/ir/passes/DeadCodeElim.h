#pragma once
#include "IPass.h"

class DeadCodeElim : public IPass {
public:
    std::string name() const override { return "DeadCodeElim"; }
    bool run(IRFunction& fn) override;
};
