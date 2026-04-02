#pragma once
#include "Function.h"
#include <codegen/Emitter.h>
#include <ostream>
#include <string>
#include <unordered_map>

class RiscVIRCodeGen {
public:
    void generate(const IRModule& mod, std::ostream& out);

private:
    Emitter emit_;
    std::string moduleName_;

    struct SlotInfo {
        int offset;
    };
    std::unordered_map<int, SlotInfo> slots_;
    int frameSize_ = 0;
    int nextSlot_ = 0;

    void emitFunction(const IRFunction& fn, const std::string& label);
    void emitBlock(const BasicBlock& bb);
    void emitInstr(const IRInstr& instr);

    void allocateSlots(const IRFunction& fn);
    int slotOf(int tempId);
    std::string blockLabel(const IRFunction& fn, int bbId);

    void loadValue(const IRValue& v, const std::string& reg);
    void storeToSlot(int tempId, const std::string& reg);

    const IRFunction* curFunc_ = nullptr;
};
