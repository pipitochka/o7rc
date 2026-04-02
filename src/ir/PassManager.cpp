#include "PassManager.h"
#include "IRPrinter.h"

PassManager& PassManager::addPass(std::unique_ptr<IPass> pass) {
    passes_.push_back(std::move(pass));
    return *this;
}

void PassManager::runOnFunction(IRFunction& fn, std::ostream* log) {
    IRPrinter printer;

    for (auto& pass : passes_) {
        bool changed = pass->run(fn);
        if (log) {
            *log << "; --- after " << pass->name()
                 << (changed ? " (changed)" : " (no change)")
                 << " on " << fn.name << " ---\n";
            printer.printFunction(fn, *log);
            *log << "\n";
        }
    }
}

void PassManager::run(IRModule& mod, std::ostream* log) {
    for (auto& fn : mod.functions)
        runOnFunction(fn, log);
    runOnFunction(mod.mainBody, log);
}
