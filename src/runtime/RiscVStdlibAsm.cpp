#include "RiscVStdlibAsm.h"
#include "StdlibProc.h"

#include <codegen/RiscVCodeGen.h>

namespace o7rc::runtime {

bool riscvEmitStdlibCall(RiscVCodeGen& cg, DesignatorExpr& des, ArgsSelector* args) {
    if (!args) return false;

    std::string name = des.baseName;
    auto dotPos = name.find('.');
    if (dotPos == std::string::npos) return false;

    std::string module = name.substr(0, dotPos);
    std::string proc = name.substr(dotPos + 1);

    switch (classifyStdlibProc(module, proc)) {
    case StdlibProcKind::None:
        return false;

    case StdlibProcKind::Out_Int:
        if (!args->args.empty()) {
            args->args[0]->accept(cg);
            cg.emit_.text("li a7, 1");
            cg.emit_.text("ecall");
        }
        return true;

    case StdlibProcKind::Out_Ln:
        cg.emit_.text("li a0, 10");
        cg.emit_.text("li a7, 11");
        cg.emit_.text("ecall");
        return true;

    case StdlibProcKind::Out_String:
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                if (d->selectors.empty()) {
                    TypeInfo* ty = cg.typeAfterDesignator(*d);
                    if (ty && ty->kind == TypeInfo::TArray) {
                        cg.emitAddress(*d);
                        cg.emit_.text("li a7, 4");
                        cg.emit_.text("ecall");
                        return true;
                    }
                }
            }
            args->args[0]->accept(cg);
            cg.emit_.text("li a7, 4");
            cg.emit_.text("ecall");
        }
        return true;

    case StdlibProcKind::Out_Char:
        if (!args->args.empty()) {
            args->args[0]->accept(cg);
            cg.emit_.text("li a7, 11");
            cg.emit_.text("ecall");
        }
        return true;

    case StdlibProcKind::Out_Real:
        if (!args->args.empty()) {
            args->args[0]->accept(cg);
            cg.emit_.text("fmv.w.x fa0, a0");
            cg.emit_.text("li a7, 2");
            cg.emit_.text("ecall");
        }
        return true;

    case StdlibProcKind::In_Open:
        return true;

    case StdlibProcKind::In_Int:
        cg.emit_.text("li a7, 5");
        cg.emit_.text("ecall");
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                cg.emit_.text("mv t0, a0");
                cg.emitAddress(*d);
                cg.emit_.text("sw t0, 0(a0)");
            }
        }
        return true;

    case StdlibProcKind::In_Char:
        cg.emit_.text("li a7, 12");
        cg.emit_.text("ecall");
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                cg.emit_.text("mv t0, a0");
                cg.emitAddress(*d);
                cg.emit_.text("sw t0, 0(a0)");
            }
        }
        return true;

    case StdlibProcKind::In_Line:
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                cg.emitAddress(*d);
                cg.emit_.text("mv t0, a0");
                int maxLen = 256;
                TypeInfo* ty = cg.typeAfterDesignator(*d);
                if (ty && ty->kind == TypeInfo::TArray)
                    maxLen = ty->arrayLength;
                cg.emit_.text("mv a0, t0");
                cg.emit_.text("li a1, " + std::to_string(maxLen));
                cg.emit_.text("li a7, 8");
                cg.emit_.text("ecall");
            }
        }
        return true;

    default:
        return false;
    }
}

bool riscvEmitStdlibStmt(RiscVCodeGen& cg, DesignatorExpr& des) {
    std::string name = des.baseName;
    auto dotPos = name.find('.');
    if (dotPos == std::string::npos) return false;

    std::string module = name.substr(0, dotPos);
    std::string proc = name.substr(dotPos + 1);

    switch (classifyStdlibProc(module, proc)) {
    case StdlibProcKind::Out_Ln:
        cg.emit_.text("li a0, 10");
        cg.emit_.text("li a7, 11");
        cg.emit_.text("ecall");
        return true;
    case StdlibProcKind::In_Open:
        return true;
    default:
        return false;
    }
}

} // namespace o7rc::runtime
