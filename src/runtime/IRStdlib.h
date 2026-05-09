#pragma once

#include <util/ast/Ast.h>

class IRBuilder;

namespace o7rc::runtime {

bool irEmitStdlibCall(IRBuilder& b, DesignatorExpr& des, ArgsSelector* args);
bool irEmitStdlibStmt(IRBuilder& b, DesignatorExpr& des);

} // namespace o7rc::runtime
