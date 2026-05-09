#pragma once

#include <util/ast/Ast.h>

class RiscVCodeGen;

namespace o7rc::runtime {

bool riscvEmitStdlibCall(RiscVCodeGen& cg, DesignatorExpr& des, ArgsSelector* args);
bool riscvEmitStdlibStmt(RiscVCodeGen& cg, DesignatorExpr& des);

} // namespace o7rc::runtime
