#pragma once
#include "Function.h"
#include <util/ast/IVisitor.h>
#include <util/ast/Ast.h>

#include <string>
#include <unordered_map>
#include <cstring>

class IRBuilder : public IVisitor {
public:
    IRModule build(Module& module);

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
    IRModule module_;
    IRFunction* curFunc_ = nullptr;
    BasicBlock* curBlock_ = nullptr;
    IRValue lastVal_;
    bool inGlobalScope_ = false;
    std::string moduleName_;

    struct ConstInfo { int64_t value; };
    std::unordered_map<std::string, ConstInfo> constants_;

    struct VarInfo {
        IRValue addr;
        bool isGlobal = false;
        bool isVarParam = false;
        std::string globalLabel;
        int arrayLen = 0;
    };
    std::vector<std::unordered_map<std::string, VarInfo>> scopes_;

    void pushScope();
    void popScope();
    VarInfo* lookupVar(const std::string& name);

    void emit(IRInstr instr);
    IRValue emitExpr(Expr& e);
    void emitStatements(const std::vector<StmtPtr>& stmts);
    IRValue emitAddress(DesignatorExpr& des);
    IRValue emitLoad(DesignatorExpr& des);
    bool tryEmitBuiltin(DesignatorExpr& des);

    void setBlock(BasicBlock* bb);
    void finishBlock(IRInstr terminator);

    int typeSize(TypeNode* t);
    int arrayLength(TypeNode* t);
};
