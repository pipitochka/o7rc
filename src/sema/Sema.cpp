#include "Sema.h"
#include <algorithm>

#include <runtime/StdlibProc.h>

const std::unordered_set<std::string> Sema::builtinProcs_ = [] {
    std::unordered_set<std::string> s = {
        "INC", "DEC", "NEW", "ABS", "ODD", "ORD", "CHR", "LEN",
        "ASSERT", "PACK", "UNPK", "FLOOR", "FLT", "LSL", "ASR", "ROR",
    };
    for (const auto& q : o7rc::runtime::stdlibQualifiedProcNames())
        s.insert(q);
    return s;
}();

const std::unordered_set<std::string> Sema::builtinTypes_ = {
    "INTEGER", "BOOLEAN", "REAL", "CHAR", "SET", "BYTE", "LONGREAL",
};

SemaErrors Sema::analyze(Module& module) {
    errors_.clear();
    scopes_.clear();
    module.accept(*this);
    return errors_;
}

void Sema::pushScope() {
    scopes_.emplace_back();
}

void Sema::popScope() {
    if (!scopes_.empty()) scopes_.pop_back();
}

bool Sema::declare(const std::string& name, SymInfo info, SourcePos pos) {
    if (scopes_.empty()) return false;
    auto& top = scopes_.back();
    if (top.count(name)) {
        error(SemaError::DuplicateSymbol,
              "duplicate declaration of '" + name + "'", pos);
        return false;
    }
    top[name] = std::move(info);
    return true;
}

Sema::SymInfo* Sema::lookup(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

void Sema::error(SemaError::Kind kind, const std::string& msg, SourcePos pos) {
    errors_.push_back({kind, msg, pos});
}

std::string Sema::resolveBaseName(const std::string& name) {
    auto dot = name.find('.');
    if (dot != std::string::npos)
        return name.substr(dot + 1);
    return name;
}

void Sema::checkExpr(Expr* e) {
    if (e) e->accept(*this);
}

void Sema::checkStatements(const std::vector<StmtPtr>& stmts) {
    for (auto& s : stmts)
        if (s) s->accept(*this);
}

// --- Module ---

void Sema::visit(Module& mod) {
    moduleName_ = mod.name;

    if (mod.name != mod.endName) {
        error(SemaError::ModuleNameMismatch,
              "MODULE name '" + mod.name + "' does not match END name '" + mod.endName + "'",
              mod.range.begin);
    }

    pushScope();

    for (auto& imp : mod.imports)
        imp.accept(*this);

    for (auto& d : mod.decls)
        d->accept(*this);

    checkStatements(mod.block);

    popScope();
}

void Sema::visit(Import& imp) {
    std::string alias = imp.alias.value_or(imp.name);
    declare(alias, {SymKind::Import, imp.name}, imp.range.begin);
}

void Sema::visit(Block&) {}

// --- Declarations ---

void Sema::visit(ConstDecl& d) {
    declare(d.name, {SymKind::Const, d.name}, d.range.begin);
    checkExpr(d.value.get());
}

void Sema::visit(TypeDecl& d) {
    declare(d.name, {SymKind::Type, d.name}, d.range.begin);
    if (d.type) d.type->accept(*this);
}

void Sema::visit(VarDecl& d) {
    for (auto& name : d.names)
        declare(name, {SymKind::Var, name}, d.range.begin);
    if (d.type) d.type->accept(*this);
}

void Sema::visit(ProcDecl& proc) {
    auto* sig = dynamic_cast<ProcType*>(proc.type.get());
    int paramCount = 0;
    bool hasReturn = false;
    if (sig) {
        for (auto& sec : sig->params)
            paramCount += static_cast<int>(sec->names.size());
        hasReturn = (sig->type != nullptr && !sig->type->name.empty());
    }

    declare(proc.name,
            {SymKind::Proc, proc.name, paramCount, hasReturn},
            proc.range.begin);

    pushScope();
    bool savedInsideFunc = insideFunction_;
    bool savedFuncReturn = currentFuncHasReturn_;
    insideFunction_ = hasReturn;
    currentFuncHasReturn_ = false;

    if (sig) {
        for (auto& sec : sig->params)
            for (auto& pname : sec->names)
                declare(pname, {SymKind::Param, pname}, proc.range.begin);
    }

    for (auto& d : proc.decls)
        d->accept(*this);

    checkStatements(proc.body);

    if (proc.returnValue) {
        checkExpr(proc.returnValue.get());
        currentFuncHasReturn_ = true;
    }

    if (hasReturn && !currentFuncHasReturn_) {
        error(SemaError::MissingReturn,
              "function '" + proc.name + "' missing RETURN",
              proc.range.begin);
    }

    insideFunction_ = savedInsideFunc;
    currentFuncHasReturn_ = savedFuncReturn;
    popScope();
}

void Sema::visit(ParamDecl&) {}

// --- Types ---

void Sema::visit(NamedType& t) {
    if (builtinTypes_.count(t.name)) return;
    if (!lookup(t.name)) {
        error(SemaError::UndefinedSymbol,
              "undefined type '" + t.name + "'",
              t.range.begin);
    }
}

void Sema::visit(ArrayType& t) {
    for (auto& len : t.length)
        checkExpr(len.get());
    if (t.elemType) t.elemType->accept(*this);
}

void Sema::visit(RecordType& t) {
    if (t.baseType) t.baseType->accept(*this);
    for (auto& f : t.fields)
        if (f) f->accept(*this);
}

void Sema::visit(PointerType& t) {
    if (t.baseType) t.baseType->accept(*this);
}

void Sema::visit(ProcParams& p) {
    if (p.type) p.type->accept(*this);
}

void Sema::visit(ProcType& t) {
    for (auto& p : t.params)
        if (p) p->accept(*this);
    if (t.type) t.type->accept(*this);
}

void Sema::visit(FieldDecl& f) {
    if (f.type) f.type->accept(*this);
}

// --- Expressions ---

void Sema::visit(LiteralExpr&) {}

void Sema::visit(UnaryExpr& e) {
    checkExpr(e.rhs.get());
}

void Sema::visit(BinaryExpr& e) {
    checkExpr(e.lhs.get());
    checkExpr(e.rhs.get());
}

void Sema::visit(CallExpr& e) {
    checkExpr(e.callee.get());
    for (auto& a : e.args)
        checkExpr(a.get());
}

void Sema::visit(DesignatorExpr& des) {
    std::string baseName = des.baseName;
    std::string lookupName = resolveBaseName(baseName);

    auto dot = baseName.find('.');
    if (dot != std::string::npos) {
        std::string modName = baseName.substr(0, dot);
        std::string qualName = modName + "." + lookupName;
        // SYSTEM не загружается с диска; остальные квалифицированные имена требуют IMPORT.
        if (modName != "SYSTEM" && !lookup(modName)) {
            error(SemaError::UndefinedSymbol,
                  "undefined module '" + modName + "'",
                  des.range.begin);
            return;
        }
        if (builtinProcs_.count(qualName))
            goto selectors;
        goto selectors;
    }

    if (!builtinProcs_.count(lookupName)) {
        if (!lookup(lookupName)) {
            error(SemaError::UndefinedSymbol,
                  "undefined symbol '" + lookupName + "'",
                  des.range.begin);
            return;
        }
    }

selectors:
    for (auto& sel : des.selectors) {
        if (auto* args = dynamic_cast<ArgsSelector*>(sel.get())) {
            for (auto& a : args->args)
                checkExpr(a.get());

            std::string procName;
            if (dot != std::string::npos) {
                procName = baseName;
            } else {
                procName = lookupName;
            }

            if (!builtinProcs_.count(procName)) {
                auto* sym = lookup(lookupName);
                if (sym && sym->kind == SymKind::Proc && sym->paramCount >= 0) {
                    int actual = static_cast<int>(args->args.size());
                    if (actual != sym->paramCount) {
                        error(SemaError::ArityMismatch,
                              "'" + lookupName + "' expects " +
                              std::to_string(sym->paramCount) +
                              " arguments, got " + std::to_string(actual),
                              des.range.begin);
                    }
                }
            }
        } else if (auto* idx = dynamic_cast<IndexSelector*>(sel.get())) {
            for (auto& i : idx->index)
                checkExpr(i.get());
        }
    }
}

void Sema::visit(SetExpr& e) {
    for (auto& elem : e.elements) {
        checkExpr(elem.low.get());
        checkExpr(elem.high.get());
    }
}

// --- Selectors (handled in DesignatorExpr) ---

void Sema::visit(FieldSelector&) {}
void Sema::visit(IndexSelector&) {}
void Sema::visit(DerefSelector&) {}
void Sema::visit(TypeGuardSelector&) {}
void Sema::visit(ArgsSelector&) {}

// --- Statements ---

void Sema::visit(AssignStmt& s) {
    checkExpr(s.rhs.get());

    if (s.lhs) {
        std::string name = resolveBaseName(s.lhs->baseName);
        auto* sym = lookup(name);
        if (sym && sym->kind == SymKind::Const) {
            error(SemaError::ConstAssign,
                  "cannot assign to constant '" + name + "'",
                  s.range.begin);
        }
        s.lhs->accept(*this);
    }
}

void Sema::visit(CallStmt& s) {
    if (s.designator) s.designator->accept(*this);
}

void Sema::visit(IfStmt& s) {
    for (auto& br : s.branches) {
        checkExpr(br.cond.get());
        checkStatements(br.body);
    }
    checkStatements(s.elseBody);
}

void Sema::visit(WhileStmt& s) {
    checkExpr(s.cond.get());
    checkStatements(s.body);
}

void Sema::visit(RepeatStmt& s) {
    checkStatements(s.body);
    checkExpr(s.untilCond.get());
}

void Sema::visit(ForStmt& s) {
    if (!lookup(s.varName) && !s.varName.empty()) {
        error(SemaError::UndefinedSymbol,
              "undefined loop variable '" + s.varName + "'",
              s.range.begin);
    }
    checkExpr(s.from.get());
    checkExpr(s.to.get());
    if (s.by) checkExpr(s.by.get());
    checkStatements(s.body);
}

void Sema::visit(ReturnStmt& s) {
    currentFuncHasReturn_ = true;
    checkExpr(s.value.get());
}

void Sema::visit(CaseStmt& s) {
    checkExpr(s.expr.get());
    for (auto& alt : s.alts)
        if (alt) alt->accept(*this);
}

void Sema::visit(CaseAlternative& alt) {
    for (auto& lbl : alt.labels)
        if (lbl) lbl->accept(*this);
    for (auto& s : alt.body)
        if (s) s->accept(*this);
}

void Sema::visit(CaseLabel& lbl) {
    checkExpr(lbl.from.get());
    checkExpr(lbl.to.get());
}
