#include "ModuleLoader.h"
#include <util/ast/Ast.h>

#include <fstream>
#include <stdexcept>

static const std::unordered_set<std::string> kBuiltins = {
    "SYSTEM",
};

const std::unordered_set<std::string>& ModuleLoader::builtinModules() {
    return kBuiltins;
}

void ModuleLoader::addSearchPath(const std::string& dir) {
    searchPaths_.push_back(dir);
}

void ModuleLoader::setParserFactory(ParserFactory factory) {
    parserFactory_ = std::move(factory);
}

std::string ModuleLoader::findModule(const std::string& name) const {
    for (auto& dir : searchPaths_) {
        std::string path = dir + "/" + name + ".obr";
        std::ifstream f(path);
        if (f.good()) return path;

        path = dir + "/" + name + ".obn";
        f = std::ifstream(path);
        if (f.good()) return path;
    }
    return {};
}

Module* ModuleLoader::load(const std::string& moduleName) {
    if (builtinModules().count(moduleName))
        return nullptr;

    auto it = cache_.find(moduleName);
    if (it != cache_.end())
        return loaded_[it->second].ast.get();

    std::string path = findModule(moduleName);
    if (path.empty())
        return nullptr;

    if (!parserFactory_)
        return nullptr;

    auto mod = parserFactory_(path);
    if (!mod)
        return nullptr;

    if (mod->name != moduleName)
        return nullptr;

    Module* ptr = mod.get();
    size_t idx = loaded_.size();
    loaded_.push_back({moduleName, path, std::move(mod)});
    cache_[moduleName] = idx;

    loadImports(*ptr);

    return ptr;
}

bool ModuleLoader::loadImports(Module& root) {
    for (auto& imp : root.imports) {
        const std::string& name = imp.name;
        if (builtinModules().count(name))
            continue;
        if (!load(name))
            return false;
    }
    return true;
}
