#include "RiscVIRCodeGen.h"
#include <stdexcept>

void RiscVIRCodeGen::generate(const IRModule& mod, std::ostream& out) {
    moduleName_ = mod.name;

    for (auto& g : mod.globals) {
        if (g.isString) {
            emit_.data(g.label + ": .asciz \"" + g.stringLiteral + "\"");
        } else if (g.isArray) {
            emit_.data(g.label + ": .space " + std::to_string(g.size));
        } else {
            emit_.data(g.label + ": .word 0");
        }
    }

    for (auto& fn : mod.functions) {
        std::string label = "proc_" + moduleName_ + "_" + fn.name;
        emitFunction(fn, label);
    }

    emitFunction(mod.mainBody, "main");

    emit_.writeTo(out);
}

std::string RiscVIRCodeGen::blockLabel(const IRFunction& fn, int bbId) {
    return "L_" + fn.name + "_bb" + std::to_string(bbId);
}

void RiscVIRCodeGen::allocateSlots(const IRFunction& fn) {
    slots_.clear();
    allocaDataOffset_.clear();
    nextSlot_ = 0;

    // Pass 1: allocate slots for all temporaries (pointer/value storage)
    for (auto& bb : fn.blocks) {
        for (auto& instr : bb->instrs) {
            auto ensure = [&](const IRValue& v) {
                if (v.isTemp() && slots_.find(v.id) == slots_.end()) {
                    slots_[v.id] = {nextSlot_};
                    nextSlot_ += 4;
                }
            };
            if (instr.hasDst()) ensure(instr.dst);
            if (instr.src1.isTemp()) ensure(instr.src1);
            if (instr.src2.isTemp()) ensure(instr.src2);
            for (auto& a : instr.args)
                if (a.isTemp()) ensure(a);
        }
    }

    for (int i = 0; i < static_cast<int>(fn.params.size()); ++i) {
        if (slots_.find(-1000 - i) == slots_.end()) {
            slots_[-1000 - i] = {nextSlot_};
            nextSlot_ += 4;
        }
    }

    // Pass 2: allocate separate data areas for alloca instructions
    for (auto& bb : fn.blocks) {
        for (auto& instr : bb->instrs) {
            if (instr.op == IROp::Alloca && instr.dst.isTemp()) {
                int size = 4;
                if (instr.src1.isConst() && instr.src1.constVal > 0)
                    size = static_cast<int>(instr.src1.constVal);
                size = (size + 3) & ~3;
                allocaDataOffset_[instr.dst.id] = nextSlot_;
                nextSlot_ += size;
            }
        }
    }

    frameSize_ = nextSlot_ + 8;
    frameSize_ = (frameSize_ + 15) & ~15;
}

int RiscVIRCodeGen::slotOf(int tempId) {
    auto it = slots_.find(tempId);
    if (it != slots_.end()) return it->second.offset;
    slots_[tempId] = {nextSlot_};
    nextSlot_ += 4;
    return slots_[tempId].offset;
}

void RiscVIRCodeGen::loadValue(const IRValue& v, const std::string& reg) {
    if (v.isConst()) {
        emit_.text("li " + reg + ", " + std::to_string(v.constVal));
    } else if (v.isTemp()) {
        int off = slotOf(v.id);
        emit_.text("lw " + reg + ", " + std::to_string(off) + "(sp)");
    } else if (v.isParam()) {
        int pslot = slots_.count(-1000 - v.id) ? slots_[-1000 - v.id].offset : 0;
        emit_.text("lw " + reg + ", " + std::to_string(pslot) + "(sp)");
    }
}

void RiscVIRCodeGen::storeToSlot(int tempId, const std::string& reg) {
    int off = slotOf(tempId);
    emit_.text("sw " + reg + ", " + std::to_string(off) + "(sp)");
}

void RiscVIRCodeGen::emitFunction(const IRFunction& fn, const std::string& label) {
    curFunc_ = &fn;
    allocateSlots(fn);

    emit_.blank();
    emit_.label(label);

    // Prologue
    emit_.text("addi sp, sp, -" + std::to_string(frameSize_));
    emit_.text("sw ra, " + std::to_string(frameSize_ - 4) + "(sp)");
    emit_.text("sw s0, " + std::to_string(frameSize_ - 8) + "(sp)");
    emit_.text("addi s0, sp, " + std::to_string(frameSize_));

    // Store params from registers to stack slots
    for (int i = 0; i < static_cast<int>(fn.params.size()) && i < 8; ++i) {
        int pslot = slots_[-1000 - i].offset;
        emit_.text("sw a" + std::to_string(i) + ", " + std::to_string(pslot) + "(sp)");
    }

    for (auto& bb : fn.blocks) {
        if (bb->instrs.empty()) continue;
        emitBlock(*bb);
    }

    std::string epilogue = "L_" + fn.name + "_epilogue";
    emit_.label(epilogue);
    emit_.text("lw ra, " + std::to_string(frameSize_ - 4) + "(sp)");
    emit_.text("lw s0, " + std::to_string(frameSize_ - 8) + "(sp)");
    emit_.text("addi sp, sp, " + std::to_string(frameSize_));

    if (label == "main") {
        emit_.text("li a7, 10");
        emit_.text("ecall");
    } else {
        emit_.text("jr ra");
    }

    curFunc_ = nullptr;
}

void RiscVIRCodeGen::emitBlock(const BasicBlock& bb) {
    emit_.label(blockLabel(*curFunc_, bb.id));
    for (auto& instr : bb.instrs)
        emitInstr(instr);
}

void RiscVIRCodeGen::emitInstr(const IRInstr& instr) {
    switch (instr.op) {
        case IROp::Alloca: {
            int dataOff = allocaDataOffset_[instr.dst.id];
            emit_.text("addi a0, sp, " + std::to_string(dataOff));
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::AddrGlobal: {
            emit_.text("la a0, " + instr.name);
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::AddrLocal: {
            int off = slotOf(instr.dst.id);
            emit_.text("addi a0, sp, " + std::to_string(off));
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Load: {
            loadValue(instr.src1, "t0");
            emit_.text("lw a0, 0(t0)");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Store: {
            loadValue(instr.src2, "t0");
            loadValue(instr.src1, "t1");
            emit_.text("sw t0, 0(t1)");
            break;
        }

        case IROp::LoadLocal: {
            loadValue(instr.src1, "t0");
            emit_.text("lw a0, 0(t0)");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::StoreLocal: {
            loadValue(instr.src2, "t0");
            loadValue(instr.src1, "t1");
            emit_.text("sw t0, 0(t1)");
            break;
        }

        case IROp::LoadGlobal: {
            emit_.text("la t0, " + instr.name);
            emit_.text("lw a0, 0(t0)");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::StoreGlobal: {
            loadValue(instr.src1, "t0");
            emit_.text("la t1, " + instr.name);
            emit_.text("sw t0, 0(t1)");
            break;
        }

        case IROp::Copy: {
            loadValue(instr.src1, "a0");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Add: case IROp::Sub: case IROp::Mul: case IROp::Div: case IROp::Mod:
        case IROp::And: case IROp::Or: {
            loadValue(instr.src1, "t0");
            loadValue(instr.src2, "t1");
            switch (instr.op) {
                case IROp::Add: emit_.text("add a0, t0, t1"); break;
                case IROp::Sub: emit_.text("sub a0, t0, t1"); break;
                case IROp::Mul: emit_.text("mul a0, t0, t1"); break;
                case IROp::Div: emit_.text("div a0, t0, t1"); break;
                case IROp::Mod: emit_.text("rem a0, t0, t1"); break;
                case IROp::And: emit_.text("and a0, t0, t1"); break;
                case IROp::Or:  emit_.text("or a0, t0, t1"); break;
                default: break;
            }
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Neg: {
            loadValue(instr.src1, "t0");
            emit_.text("neg a0, t0");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Not: {
            loadValue(instr.src1, "t0");
            emit_.text("seqz a0, t0");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Eq: {
            loadValue(instr.src1, "t0");
            loadValue(instr.src2, "t1");
            emit_.text("sub a0, t0, t1");
            emit_.text("seqz a0, a0");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Neq: {
            loadValue(instr.src1, "t0");
            loadValue(instr.src2, "t1");
            emit_.text("sub a0, t0, t1");
            emit_.text("snez a0, a0");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Lt: {
            loadValue(instr.src1, "t0");
            loadValue(instr.src2, "t1");
            emit_.text("slt a0, t0, t1");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Ge: {
            loadValue(instr.src1, "t0");
            loadValue(instr.src2, "t1");
            emit_.text("slt a0, t0, t1");
            emit_.text("xori a0, a0, 1");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Gt: {
            loadValue(instr.src1, "t0");
            loadValue(instr.src2, "t1");
            emit_.text("slt a0, t1, t0");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Le: {
            loadValue(instr.src1, "t0");
            loadValue(instr.src2, "t1");
            emit_.text("slt a0, t1, t0");
            emit_.text("xori a0, a0, 1");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Index: {
            loadValue(instr.src1, "t0");
            loadValue(instr.src2, "t1");
            emit_.text("slli t1, t1, 2");
            emit_.text("add a0, t0, t1");
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Branch: {
            loadValue(instr.src1, "t0");
            emit_.text("bnez t0, " + blockLabel(*curFunc_, instr.targetBlock));
            emit_.text("j " + blockLabel(*curFunc_, instr.falseBlock));
            break;
        }

        case IROp::Jump: {
            emit_.text("j " + blockLabel(*curFunc_, instr.targetBlock));
            break;
        }

        case IROp::Ret: {
            if (!instr.src1.isVoid()) {
                loadValue(instr.src1, "a0");
            }
            emit_.text("j L_" + curFunc_->name + "_epilogue");
            break;
        }

        case IROp::Call: {
            for (int i = 0; i < static_cast<int>(instr.args.size()) && i < 8; ++i) {
                loadValue(instr.args[i], "a" + std::to_string(i));
            }

            std::string procLabel = "proc_" + moduleName_ + "_" + instr.name;
            emit_.text("jal ra, " + procLabel);
            storeToSlot(instr.dst.id, "a0");
            break;
        }

        case IROp::Syscall: {
            if (!instr.args.empty()) {
                loadValue(instr.args[0], "a0");
            }
            emit_.text("li a7, " + std::to_string(instr.syscallNum));
            emit_.text("ecall");
            if (!instr.dst.isVoid() && instr.dst.isTemp()) {
                storeToSlot(instr.dst.id, "a0");
            }
            break;
        }

        case IROp::Nop:
            break;

        default:
            emit_.comment("unhandled IR op");
            break;
    }
}
