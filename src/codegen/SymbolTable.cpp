#include "SymbolTable.h"

void SymbolTable::enterScope() {
    scopes_.push_back({});
}

void SymbolTable::leaveScope() {
    if (!scopes_.empty())
        scopes_.pop_back();
}

void SymbolTable::define(const Symbol& sym) {
    if (!scopes_.empty())
        scopes_.back().symbols[sym.name] = sym;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
        auto it = scopes_[i].symbols.find(name);
        if (it != scopes_[i].symbols.end())
            return &it->second;
    }
    return nullptr;
}

int SymbolTable::allocLocal(int size) {
    if (scopes_.empty()) return 0;
    auto& scope = scopes_.back();
    scope.frameSize += size;
    return -scope.frameSize;
}

int SymbolTable::currentFrameSize() const {
    if (scopes_.empty()) return 0;
    return scopes_.back().frameSize;
}
