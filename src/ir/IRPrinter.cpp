#include "IRPrinter.h"

std::string IRPrinter::opName(IROp op) {
    switch (op) {
        case IROp::Nop:         return "nop";
        case IROp::LoadGlobal:  return "load_global";
        case IROp::StoreGlobal: return "store_global";
        case IROp::LoadLocal:   return "load_local";
        case IROp::StoreLocal:  return "store_local";
        case IROp::Alloca:      return "alloca";
        case IROp::AddrLocal:   return "addr_local";
        case IROp::AddrGlobal:  return "addr_global";
        case IROp::Add:  return "add";
        case IROp::Sub:  return "sub";
        case IROp::Mul:  return "mul";
        case IROp::Div:  return "div";
        case IROp::Mod:  return "mod";
        case IROp::Neg:  return "neg";
        case IROp::Itof: return "itof";
        case IROp::FAdd: return "fadd";
        case IROp::FSub: return "fsub";
        case IROp::FMul: return "fmul";
        case IROp::FDiv: return "fdiv";
        case IROp::FNeg: return "fneg";
        case IROp::FEq:  return "feq";
        case IROp::FLt:  return "flt";
        case IROp::FLe:  return "fle";
        case IROp::Index1: return "index1";
        case IROp::Load8: return "load8";
        case IROp::Store8: return "store8";
        case IROp::Eq:   return "eq";
        case IROp::Neq:  return "neq";
        case IROp::Lt:   return "lt";
        case IROp::Le:   return "le";
        case IROp::Gt:   return "gt";
        case IROp::Ge:   return "ge";
        case IROp::And:  return "and";
        case IROp::Or:   return "or";
        case IROp::Not:  return "not";
        case IROp::Load:  return "load";
        case IROp::Store: return "store";
        case IROp::Index: return "index";
        case IROp::Branch: return "br";
        case IROp::Jump:   return "jmp";
        case IROp::Ret:    return "ret";
        case IROp::Call:    return "call";
        case IROp::Syscall: return "syscall";
        case IROp::Copy:    return "copy";
    }
    return "???";
}

std::string IRPrinter::formatInstr(const IRInstr& instr) {
    std::string s;

    switch (instr.op) {
        case IROp::Alloca:
            s = instr.dst.str() + " = alloca \"" + instr.name + "\" (" + instr.src1.str() + " bytes)";
            break;

        case IROp::AddrGlobal:
            s = instr.dst.str() + " = addr_global \"" + instr.name + "\"";
            break;

        case IROp::AddrLocal:
            s = instr.dst.str() + " = addr_local \"" + instr.name + "\"";
            break;

        case IROp::LoadGlobal:
            s = instr.dst.str() + " = load_global \"" + instr.name + "\"";
            break;

        case IROp::StoreGlobal:
            s = "store_global \"" + instr.name + "\", " + instr.src1.str();
            break;

        case IROp::Load:
            s = instr.dst.str() + " = load [" + instr.src1.str() + "]";
            break;

        case IROp::Store:
            s = "store [" + instr.src1.str() + "], " + instr.src2.str();
            break;

        case IROp::LoadLocal:
            s = instr.dst.str() + " = load_local [" + instr.src1.str() + "]";
            break;

        case IROp::StoreLocal:
            s = "store_local [" + instr.src1.str() + "], " + instr.src2.str();
            break;

        case IROp::Index:
            s = instr.dst.str() + " = index " + instr.src1.str() + ", " + instr.src2.str();
            break;

        case IROp::Add: case IROp::Sub: case IROp::Mul: case IROp::Div: case IROp::Mod:
        case IROp::Eq: case IROp::Neq: case IROp::Lt: case IROp::Le: case IROp::Gt: case IROp::Ge:
        case IROp::And: case IROp::Or:
        case IROp::FAdd: case IROp::FSub: case IROp::FMul: case IROp::FDiv:
        case IROp::FEq: case IROp::FLt: case IROp::FLe:
        case IROp::Index1:
            s = instr.dst.str() + " = " + opName(instr.op) + " " + instr.src1.str() + ", " + instr.src2.str();
            break;

        case IROp::Load8:
            s = instr.dst.str() + " = load8 [" + instr.src1.str() + "]";
            break;

        case IROp::Store8:
            s = "store8 [" + instr.src1.str() + "], " + instr.src2.str();
            break;

        case IROp::Neg: case IROp::Not: case IROp::Itof: case IROp::FNeg:
            s = instr.dst.str() + " = " + opName(instr.op) + " " + instr.src1.str();
            break;

        case IROp::Copy:
            s = instr.dst.str() + " = copy " + instr.src1.str();
            break;

        case IROp::Branch:
            s = "br " + instr.src1.str() + ", bb" + std::to_string(instr.targetBlock)
                + ", bb" + std::to_string(instr.falseBlock);
            break;

        case IROp::Jump:
            s = "jmp bb" + std::to_string(instr.targetBlock);
            break;

        case IROp::Ret:
            s = "ret " + instr.src1.str();
            break;

        case IROp::Call: {
            s = instr.dst.str() + " = call " + instr.name + "(";
            for (size_t i = 0; i < instr.args.size(); ++i) {
                if (i > 0) s += ", ";
                s += instr.args[i].str();
            }
            s += ")";
            break;
        }

        case IROp::Syscall: {
            if (!instr.dst.isVoid())
                s = instr.dst.str() + " = ";
            s += "syscall " + std::to_string(instr.syscallNum) + "(";
            for (size_t i = 0; i < instr.args.size(); ++i) {
                if (i > 0) s += ", ";
                s += instr.args[i].str();
            }
            s += ")";
            break;
        }

        default:
            s = opName(instr.op);
            break;
    }

    return s;
}

void IRPrinter::printBlock(const BasicBlock& bb, std::ostream& out) {
    out << "  " << bb.label << ":";
    if (!bb.predecessors.empty()) {
        out << "                              ; preds:";
        for (int p : bb.predecessors) out << " bb" << p;
    }
    out << "\n";
    for (auto& instr : bb.instrs) {
        out << "    " << formatInstr(instr) << "\n";
    }
}

void IRPrinter::printFunction(const IRFunction& fn, std::ostream& out) {
    out << "function " << fn.name << "(";
    for (size_t i = 0; i < fn.params.size(); ++i) {
        if (i > 0) out << ", ";
        if (i < fn.varParams.size() && fn.varParams[i]) out << "VAR ";
        out << fn.params[i];
    }
    out << ")";
    if (fn.hasReturn) out << " -> i32";
    out << " {\n";

    for (auto& bb : fn.blocks) {
        if (bb->instrs.empty()) continue;
        printBlock(*bb, out);
    }

    out << "}\n";
}

void IRPrinter::print(const IRModule& mod, std::ostream& out) {
    out << "; === IR Module: " << mod.name << " ===\n\n";

    if (!mod.globals.empty()) {
        out << "; --- globals ---\n";
        for (auto& g : mod.globals) {
            if (g.isString) {
                out << "  " << g.label << " = string \"" << g.stringLiteral << "\"\n";
            } else if (g.isArray) {
                out << "  " << g.label << " = array [" << g.size << " bytes]\n";
            } else {
                out << "  " << g.label << " = global i32\n";
            }
        }
        out << "\n";
    }

    for (auto& fn : mod.functions) {
        printFunction(fn, out);
        out << "\n";
    }

    out << "; --- main body ---\n";
    printFunction(mod.mainBody, out);
}
