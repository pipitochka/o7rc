#include "IRStdlib.h"
#include "StdlibProc.h"

#include <ir/IRBuilder.h>

namespace o7rc::runtime {

bool irEmitStdlibCall(IRBuilder& b, DesignatorExpr& des, ArgsSelector* args) {
    if (!args) return false;

    std::string name = des.baseName;
    auto dotPos = name.find('.');
    if (dotPos == std::string::npos) return false;

    std::string mod = name.substr(0, dotPos);
    std::string proc = name.substr(dotPos + 1);

    const StdlibProcKind kind = classifyStdlibProc(mod, proc);
    if (kind == StdlibProcKind::None) return false;

    switch (kind) {
    case StdlibProcKind::Out_Int:
        if (!args->args.empty()) {
            IRValue v = b.emitExpr(*args->args[0]);
            IRInstr sc;
            sc.op = IROp::Syscall;
            sc.syscallNum = 1;
            sc.args = {v};
            b.emit(sc);
        }
        b.lastVal_ = IRValue::voidVal();
        return true;

    case StdlibProcKind::Out_Ln: {
        IRInstr sc;
        sc.op = IROp::Syscall;
        sc.syscallNum = 11;
        sc.args = {IRValue::constant(10)};
        b.emit(sc);
        b.lastVal_ = IRValue::voidVal();
        return true;
    }

    case StdlibProcKind::Out_String:
        if (!args->args.empty()) {
            IRValue v;
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                std::string bn = d->baseName;
                auto dotp = bn.find('.');
                std::string lk = (dotp != std::string::npos) ? bn.substr(dotp + 1) : bn;
                auto* vi = b.lookupVar(lk);
                if (!vi) vi = b.lookupVar(bn);
                if (vi && vi->arrayLen > 0 && d->selectors.empty())
                    v = b.emitAddress(*d);
                else
                    v = b.emitExpr(*args->args[0]);
            } else {
                v = b.emitExpr(*args->args[0]);
            }
            IRInstr sc;
            sc.op = IROp::Syscall;
            sc.syscallNum = 4;
            sc.args = {v};
            b.emit(sc);
        }
        b.lastVal_ = IRValue::voidVal();
        return true;

    case StdlibProcKind::Out_Char:
        if (!args->args.empty()) {
            IRValue v = b.emitExpr(*args->args[0]);
            IRInstr sc;
            sc.op = IROp::Syscall;
            sc.syscallNum = 11;
            sc.args = {v};
            b.emit(sc);
        }
        b.lastVal_ = IRValue::voidVal();
        return true;

    case StdlibProcKind::Out_Real:
        if (!args->args.empty()) {
            IRValue v = b.emitExpr(*args->args[0]);
            IRInstr sc;
            sc.op = IROp::Syscall;
            sc.syscallNum = 2;
            sc.args = {v};
            b.emit(sc);
        }
        b.lastVal_ = IRValue::voidVal();
        return true;

    case StdlibProcKind::In_Open:
        b.lastVal_ = IRValue::voidVal();
        return true;

    case StdlibProcKind::In_Int: {
        IRValue dst = b.curFunc_->freshTemp();
        IRInstr sc;
        sc.op = IROp::Syscall;
        sc.syscallNum = 5;
        sc.dst = dst;
        b.emit(sc);
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                IRValue addr = b.emitAddress(*d);
                IRInstr store;
                store.op = IROp::Store;
                store.src1 = addr;
                store.src2 = dst;
                b.emit(store);
            }
        }
        b.lastVal_ = dst;
        return true;
    }

    case StdlibProcKind::In_Char: {
        IRValue dst = b.curFunc_->freshTemp();
        IRInstr sc;
        sc.op = IROp::Syscall;
        sc.syscallNum = 12;
        sc.dst = dst;
        b.emit(sc);
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                IRValue addr = b.emitAddress(*d);
                IRInstr store;
                store.op = IROp::Store;
                store.src1 = addr;
                store.src2 = dst;
                b.emit(store);
            }
        }
        b.lastVal_ = dst;
        return true;
    }

    case StdlibProcKind::In_Line:
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                IRValue addr = b.emitAddress(*d);
                std::string bn = d->baseName;
                auto dotp = bn.find('.');
                if (dotp != std::string::npos) bn = bn.substr(dotp + 1);
                auto* vi = b.lookupVar(bn);
                int maxLen = 256;
                if (vi && vi->arrayLen > 0) maxLen = vi->arrayLen;
                IRInstr sc;
                sc.op = IROp::Syscall;
                sc.syscallNum = 8;
                sc.args = {addr, IRValue::constant(maxLen)};
                b.emit(sc);
            }
        }
        b.lastVal_ = IRValue::voidVal();
        return true;

    default:
        return false;
    }
}

bool irEmitStdlibStmt(IRBuilder& b, DesignatorExpr& des) {
    std::string name = des.baseName;
    auto dotPos = name.find('.');
    if (dotPos == std::string::npos) return false;

    std::string mod = name.substr(0, dotPos);
    std::string proc = name.substr(dotPos + 1);

    switch (classifyStdlibProc(mod, proc)) {
    case StdlibProcKind::Out_Ln: {
        IRInstr sc;
        sc.op = IROp::Syscall;
        sc.syscallNum = 11;
        sc.args = {IRValue::constant(10)};
        b.emit(sc);
        return true;
    }
    case StdlibProcKind::In_Open:
        return true;
    default:
        return false;
    }
}

} // namespace o7rc::runtime
