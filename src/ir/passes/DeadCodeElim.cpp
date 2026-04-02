#include "DeadCodeElim.h"
#include <unordered_set>
#include <algorithm>

bool DeadCodeElim::run(IRFunction& fn) {
    bool changed = false;

    std::unordered_set<int> usedTemps;

    auto markUsed = [&](const IRValue& v) {
        if (v.isTemp()) usedTemps.insert(v.id);
    };

    for (auto& bb : fn.blocks) {
        for (auto& instr : bb->instrs) {
            if (instr.readsSrc1()) markUsed(instr.src1);
            if (instr.readsSrc2()) markUsed(instr.src2);
            if (instr.op == IROp::Branch || instr.op == IROp::Ret)
                markUsed(instr.src1);
            if (instr.op == IROp::Store || instr.op == IROp::StoreLocal) {
                markUsed(instr.src1);
                markUsed(instr.src2);
            }
            for (auto& arg : instr.args) markUsed(arg);
        }
    }

    for (auto& bb : fn.blocks) {
        auto& instrs = bb->instrs;
        auto newEnd = std::remove_if(instrs.begin(), instrs.end(),
            [&](const IRInstr& instr) {
                if (instr.isTerminator()) return false;
                if (instr.op == IROp::Store || instr.op == IROp::StoreLocal ||
                    instr.op == IROp::StoreGlobal || instr.op == IROp::Syscall ||
                    instr.op == IROp::Call)
                    return false;
                if (instr.op == IROp::Alloca) return false;
                if (!instr.hasDst()) return false;
                if (!instr.dst.isTemp()) return false;
                return usedTemps.find(instr.dst.id) == usedTemps.end();
            });

        if (newEnd != instrs.end()) {
            instrs.erase(newEnd, instrs.end());
            changed = true;
        }
    }

    // Remove empty non-entry blocks (unreachable)
    for (auto it = fn.blocks.begin(); it != fn.blocks.end();) {
        if ((*it)->instrs.empty() && (*it)->id != 0) {
            it = fn.blocks.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    return changed;
}
