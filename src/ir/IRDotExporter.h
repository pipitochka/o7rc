#pragma once
#include "Function.h"
#include "IRPrinter.h"
#include <ostream>

class IRDotExporter {
public:
    void exportModule(const IRModule& mod, std::ostream& out);
    void exportFunction(const IRFunction& fn, std::ostream& out);

private:
    IRPrinter printer_;
    static std::string escape(const std::string& s);
};
