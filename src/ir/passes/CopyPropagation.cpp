#include "CopyPropagation.h"
#include <unordered_map>

static void replaceValue(IRValue& v, const std::unordered_map<int, IRValue>& copies) {
    if (v.isTemp()) {
        auto it = copies.find(v.id);
        if (it != copies.end()) v = it->second;
    }
}

bool CopyPropagation::run(IRFunction& fn) {
    bool changed = false;

    std::unordered_map<int, IRValue> copies;

    for (auto& bb : fn.blocks) {
        for (auto& instr : bb->instrs) {
            if (instr.op == IROp::Copy && instr.dst.isTemp()) {
                copies[instr.dst.id] = instr.src1;
            }
        }
    }

    for (auto& bb : fn.blocks) {
        for (auto& instr : bb->instrs) {
            if (instr.readsSrc1()) {
                IRValue old = instr.src1;
                replaceValue(instr.src1, copies);
                if (instr.src1 != old) changed = true;
            }
            if (instr.readsSrc2()) {
                IRValue old = instr.src2;
                replaceValue(instr.src2, copies);
                if (instr.src2 != old) changed = true;
            }
            if (instr.op == IROp::Branch) {
                IRValue old = instr.src1;
                replaceValue(instr.src1, copies);
                if (instr.src1 != old) changed = true;
            }
            if (instr.op == IROp::Ret && !instr.src1.isVoid()) {
                IRValue old = instr.src1;
                replaceValue(instr.src1, copies);
                if (instr.src1 != old) changed = true;
            }
            for (auto& arg : instr.args) {
                IRValue old = arg;
                replaceValue(arg, copies);
                if (arg != old) changed = true;
            }
        }
    }

    return changed;
}
