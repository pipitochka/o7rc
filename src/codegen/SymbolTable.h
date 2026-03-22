#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

struct TypeInfo;

/// Символ: переменная, константа, процедура или параметр.
struct Symbol {
    enum Kind { Var, Const, Proc, Param };

    Kind kind = Var;
    std::string name;
    TypeInfo* type = nullptr;

    bool isGlobal = false;
    int stackOffset = 0;       // смещение от fp (для локальных/параметров)
    std::string globalLabel;   // метка в .data (для глобальных)
    std::string procLabel;     // метка в .text (для процедур)

    int64_t constValue = 0;    // значение (если Const)
    bool isVarParam = false;   // VAR-параметр (передача по ссылке)
};

/// Таблица символов с поддержкой вложенных областей видимости.
class SymbolTable {
public:
    void enterScope();
    void leaveScope();

    void define(const Symbol& sym);
    Symbol* lookup(const std::string& name);

    /// Выделяет место на стеке в текущей области и возвращает смещение от fp.
    int allocLocal(int size);

    /// Размер текущего стекового кадра (сколько байт занято локальными).
    int currentFrameSize() const;

    int depth() const { return static_cast<int>(scopes_.size()); }

private:
    struct Scope {
        std::unordered_map<std::string, Symbol> symbols;
        int frameSize = 0;
    };
    std::vector<Scope> scopes_;
};
