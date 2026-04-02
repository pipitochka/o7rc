#pragma once
#include "Function.h"
#include <ostream>
#include <string>

class IRPrinter {
public:
    void print(const IRModule& mod, std::ostream& out);
    void printFunction(const IRFunction& fn, std::ostream& out);
    void printBlock(const BasicBlock& bb, std::ostream& out);
    std::string formatInstr(const IRInstr& instr);

private:
    std::string opName(IROp op);
};
