#pragma once
#include "Instruction.h"
#include <string>
#include <vector>
#include <algorithm>

struct BasicBlock {
    int id;
    std::string label;
    std::vector<IRInstr> instrs;

    std::vector<int> predecessors;
    std::vector<int> successors;

    explicit BasicBlock(int id)
        : id(id), label("bb" + std::to_string(id)) {}

    BasicBlock(int id, std::string lbl)
        : id(id), label(std::move(lbl)) {}

    bool hasTerminator() const {
        return !instrs.empty() && instrs.back().isTerminator();
    }

    void addSuccessor(int bbId) {
        if (std::find(successors.begin(), successors.end(), bbId) == successors.end())
            successors.push_back(bbId);
    }

    void addPredecessor(int bbId) {
        if (std::find(predecessors.begin(), predecessors.end(), bbId) == predecessors.end())
            predecessors.push_back(bbId);
    }
};
