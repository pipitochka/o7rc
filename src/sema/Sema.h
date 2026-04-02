#pragma once
#include "SemaError.h"
#include <util/ast/IVisitor.h>
#include <util/ast/Ast.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Sema : public IVisitor {
public:
    SemaErrors analyze(Module& module);

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
    enum class SymKind { Const, Var, Type, Proc, Param, Import };

    struct SymInfo {
        SymKind kind;
        std::string name;
        int paramCount = -1;
        bool hasReturn = false;
    };

    std::vector<std::unordered_map<std::string, SymInfo>> scopes_;
    SemaErrors errors_;
    std::string moduleName_;
    bool insideFunction_ = false;
    bool currentFuncHasReturn_ = false;

    static const std::unordered_set<std::string> builtinProcs_;
    static const std::unordered_set<std::string> builtinTypes_;

    void pushScope();
    void popScope();
    bool declare(const std::string& name, SymInfo info, SourcePos pos);
    SymInfo* lookup(const std::string& name);
    void error(SemaError::Kind kind, const std::string& msg, SourcePos pos);
    void checkExpr(Expr* e);
    void checkStatements(const std::vector<StmtPtr>& stmts);
    std::string resolveBaseName(const std::string& name);
};
