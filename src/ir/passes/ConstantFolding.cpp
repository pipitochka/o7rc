#include "ConstantFolding.h"

bool ConstantFolding::run(IRFunction& fn) {
    bool changed = false;

    for (auto& bb : fn.blocks) {
        for (auto& instr : bb->instrs) {
            if (!instr.src1.isConst() || !instr.src2.isConst()) continue;

            int64_t a = instr.src1.constVal;
            int64_t b = instr.src2.constVal;
            int64_t result = 0;
            bool fold = true;

            switch (instr.op) {
                case IROp::Add: result = a + b; break;
                case IROp::Sub: result = a - b; break;
                case IROp::Mul: result = a * b; break;
                case IROp::Div: result = (b != 0) ? a / b : 0; break;
                case IROp::Mod: result = (b != 0) ? a % b : 0; break;
                case IROp::And: result = a & b; break;
                case IROp::Or:  result = a | b; break;
                case IROp::Eq:  result = (a == b) ? 1 : 0; break;
                case IROp::Neq: result = (a != b) ? 1 : 0; break;
                case IROp::Lt:  result = (a < b)  ? 1 : 0; break;
                case IROp::Le:  result = (a <= b) ? 1 : 0; break;
                case IROp::Gt:  result = (a > b)  ? 1 : 0; break;
                case IROp::Ge:  result = (a >= b) ? 1 : 0; break;
                default: fold = false; break;
            }

            if (fold) {
                IRValue dst = instr.dst;
                instr.op = IROp::Copy;
                instr.dst = dst;
                instr.src1 = IRValue::constant(result);
                instr.src2 = IRValue::voidVal();
                changed = true;
            }
        }

        for (auto& instr : bb->instrs) {
            if (instr.op == IROp::Neg && instr.src1.isConst()) {
                IRValue dst = instr.dst;
                instr.op = IROp::Copy;
                instr.dst = dst;
                instr.src1 = IRValue::constant(-instr.src1.constVal);
                changed = true;
            }
            if (instr.op == IROp::Not && instr.src1.isConst()) {
                IRValue dst = instr.dst;
                instr.op = IROp::Copy;
                instr.dst = dst;
                instr.src1 = IRValue::constant(instr.src1.constVal ? 0 : 1);
                changed = true;
            }
        }
    }

    return changed;
}
