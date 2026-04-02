#pragma once
#include "Value.h"
#include <string>
#include <vector>

enum class IROp {
    Nop,

    // Load/Store
    LoadGlobal,     // dst = load_global "name"
    StoreGlobal,    // store_global "name", src
    LoadLocal,      // dst = load [src]        (src = address on stack)
    StoreLocal,     // store [dst_addr], src
    Alloca,         // dst = alloca            (reserves stack slot)
    AddrLocal,      // dst = addr_local "name" (address of local var)
    AddrGlobal,     // dst = addr_global "name"

    // Arithmetic
    Add, Sub, Mul, Div, Mod,
    Neg,

    // Comparison
    Eq, Neq, Lt, Le, Gt, Ge,

    // Logic (bitwise, Oberon semantics)
    And, Or, Not,

    // Memory (pointer/array)
    Load,           // dst = load [src]
    Store,          // store [dst_addr], src
    Index,          // dst = index base, offset (base + offset*4)

    // Control flow (terminators)
    Branch,         // br cond, trueBlock, falseBlock
    Jump,           // jmp targetBlock
    Ret,            // ret src  (or ret void)

    // Calls
    Call,           // dst = call "name"(args...)
    Syscall,        // syscall number, args...

    // Copy
    Copy,           // dst = src
};

struct IRInstr {
    IROp op = IROp::Nop;
    IRValue dst;
    IRValue src1;
    IRValue src2;

    std::string name;
    std::vector<IRValue> args;

    int targetBlock = -1;
    int falseBlock = -1;
    int syscallNum = 0;

    bool isTerminator() const {
        return op == IROp::Branch || op == IROp::Jump || op == IROp::Ret;
    }

    bool hasDst() const {
        switch (op) {
            case IROp::StoreGlobal: case IROp::StoreLocal: case IROp::Store:
            case IROp::Branch: case IROp::Jump: case IROp::Ret:
            case IROp::Syscall: case IROp::Nop:
                return false;
            default:
                return !dst.isVoid();
        }
    }

    bool readsSrc1() const {
        switch (op) {
            case IROp::Nop: case IROp::Alloca: case IROp::AddrLocal:
            case IROp::AddrGlobal: case IROp::LoadGlobal: case IROp::Jump:
                return false;
            default:
                return true;
        }
    }

    bool readsSrc2() const {
        switch (op) {
            case IROp::Add: case IROp::Sub: case IROp::Mul: case IROp::Div:
            case IROp::Mod: case IROp::Eq: case IROp::Neq: case IROp::Lt:
            case IROp::Le: case IROp::Gt: case IROp::Ge: case IROp::And:
            case IROp::Or: case IROp::Index: case IROp::StoreLocal:
            case IROp::Store:
                return true;
            default:
                return false;
        }
    }
};
