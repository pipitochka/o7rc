#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <util/ast/AstFwd.h>

using ParserFactory = std::function<ModulePtr(const std::string& path)>;

struct ModuleInfo {
    std::string name;
    std::string path;
    std::unique_ptr<Module> ast;
};

class ModuleLoader {
public:
    void addSearchPath(const std::string& dir);
    void setParserFactory(ParserFactory factory);

    static const std::unordered_set<std::string>& builtinModules();

    Module* load(const std::string& moduleName);

    bool loadImports(Module& root);

    const std::vector<ModuleInfo>& loaded() const { return loaded_; }

private:
    std::string findModule(const std::string& name) const;

    std::vector<std::string> searchPaths_;
    ParserFactory parserFactory_;
    std::unordered_map<std::string, size_t> cache_;
    std::vector<ModuleInfo> loaded_;
};
