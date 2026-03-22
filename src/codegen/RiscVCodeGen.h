#pragma once
#include "ICodeGen.h"
#include "Emitter.h"
#include "SymbolTable.h"
#include "TypeInfo.h"
#include <util/ast/IVisitor.h>
#include <util/ast/Ast.h>

/// Кодогенератор Oberon-7 → RISC-V ассемблер (RV32IM, совместим с RARS).
///
/// Стратегия: стековая модель вычисления выражений.
/// Результат каждого выражения помещается в регистр a0.
/// Адрес designator'а — тоже в a0 (через emitAddress).
class RiscVCodeGen : public ICodeGen, public IVisitor {
public:
    void generate(Module& module, std::ostream& out) override;

    // ---- IVisitor ----
    void visit(Module&) override;
    void visit(Import&) override;
    void visit(Block&) override;

    void visit(ConstDecl&) override;
    void visit(TypeDecl&) override;
    void visit(VarDecl&) override;
    void visit(ProcDecl&) override;
    void visit(ParamDecl&) override;

    void visit(NamedType&) override;
    void visit(ArrayType&) override;
    void visit(RecordType&) override;
    void visit(PointerType&) override;
    void visit(ProcParams&) override;
    void visit(ProcType&) override;
    void visit(FieldDecl&) override;

    void visit(AssignStmt&) override;
    void visit(CallStmt&) override;
    void visit(IfStmt&) override;
    void visit(WhileStmt&) override;
    void visit(RepeatStmt&) override;
    void visit(ForStmt&) override;
    void visit(ReturnStmt&) override;
    void visit(CaseStmt&) override;

    void visit(LiteralExpr&) override;
    void visit(UnaryExpr&) override;
    void visit(BinaryExpr&) override;
    void visit(CallExpr&) override;
    void visit(DesignatorExpr&) override;
    void visit(SetExpr&) override;

    void visit(FieldSelector&) override;
    void visit(IndexSelector&) override;
    void visit(DerefSelector&) override;
    void visit(TypeGuardSelector&) override;
    void visit(ArgsSelector&) override;

    void visit(CaseAlternative&) override;
    void visit(CaseLabel&) override;

private:
    Emitter emit_;
    SymbolTable sym_;
    TypeRegistry types_;

    bool inGlobalScope_ = false;

    /// Разрешает TypeNode из AST в TypeInfo для кодогенерации.
    TypeInfo* resolveType(TypeNode* t);

    /// Генерирует код, помещающий *адрес* designator'а в a0.
    void emitAddress(DesignatorExpr& des);

    /// Генерирует загрузку значения designator'а в a0.
    void emitLoad(DesignatorExpr& des);

    /// Проверяет, является ли designator вызовом встроенной процедуры,
    /// и генерирует код для неё. Возвращает true если обработано.
    bool tryEmitBuiltinCall(DesignatorExpr& des);

    /// Генерирует код для всех операторов в последовательности.
    void emitStatements(const std::vector<StmtPtr>& stmts);

    /// Имя метки глобальной переменной: _ModName_varName
    std::string globalVarLabel(const std::string& name);

    std::string moduleName_;

    /// Метка эпилога текущей процедуры (для RETURN).
    std::string currentEpilogueLabel_;
};
