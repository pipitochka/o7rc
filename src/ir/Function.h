#pragma once
#include "BasicBlock.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

struct IRGlobal {
    std::string name;
    std::string label;
    int size = 4;
    bool isArray = false;
    std::string stringLiteral;
    bool isString = false;
};

struct IRFunction {
    std::string name;
    std::string moduleName;
    std::vector<std::string> params;
    std::vector<bool> varParams;
    bool hasReturn = false;

    std::vector<std::unique_ptr<BasicBlock>> blocks;
    int nextBlockId = 0;
    int nextTempId = 0;

    struct LocalInfo {
        int tempId;
        int size;
        bool isVarParam;
    };
    std::unordered_map<std::string, LocalInfo> locals;

    BasicBlock* createBlock() {
        auto bb = std::make_unique<BasicBlock>(nextBlockId++);
        auto* ptr = bb.get();
        blocks.push_back(std::move(bb));
        return ptr;
    }

    BasicBlock* createBlock(const std::string& label) {
        auto bb = std::make_unique<BasicBlock>(nextBlockId++, label);
        auto* ptr = bb.get();
        blocks.push_back(std::move(bb));
        return ptr;
    }

    IRValue freshTemp() {
        return IRValue::temp(nextTempId++);
    }

    BasicBlock* entry() {
        return blocks.empty() ? nullptr : blocks.front().get();
    }

    BasicBlock* blockById(int id) {
        for (auto& b : blocks)
            if (b->id == id) return b.get();
        return nullptr;
    }

    void linkBlocks(int from, int to) {
        if (auto* f = blockById(from)) f->addSuccessor(to);
        if (auto* t = blockById(to))   t->addPredecessor(from);
    }
};

struct IRModule {
    std::string name;
    std::vector<IRGlobal> globals;
    std::vector<IRFunction> functions;
    IRFunction mainBody;
};
