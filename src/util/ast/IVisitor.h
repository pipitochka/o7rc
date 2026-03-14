#pragma once
#include <util/ast/AstFwd.h>

struct IVisitor {
    virtual ~IVisitor() = default;

    virtual void visit(Module&) = 0;
    virtual void visit(Import&) = 0;
    virtual void visit(Block&)  = 0;

    virtual void visit(ConstDecl&) = 0;
    virtual void visit(TypeDecl&)  = 0;
    virtual void visit(VarDecl&)   = 0;
    virtual void visit(ProcDecl&)  = 0;
    virtual void visit(ParamDecl&) = 0;

    virtual void visit(NamedType&)   = 0;
    virtual void visit(ArrayType&)   = 0;
    virtual void visit(RecordType&)  = 0;
    virtual void visit(PointerType&) = 0;
    virtual void visit(ProcType&)    = 0;
    virtual void visit(FieldDecl&)   = 0;

    virtual void visit(AssignStmt&) = 0;
    virtual void visit(CallStmt&)   = 0;
    virtual void visit(IfStmt&)     = 0;
    virtual void visit(WhileStmt&)  = 0;
    virtual void visit(RepeatStmt&) = 0;
    virtual void visit(ForStmt&)    = 0;
    virtual void visit(ReturnStmt&) = 0;
    virtual void visit(CaseStmt&)   = 0;

    virtual void visit(LiteralExpr&)    = 0;
    virtual void visit(UnaryExpr&)      = 0;
    virtual void visit(BinaryExpr&)     = 0;
    virtual void visit(CallExpr&)       = 0;
    virtual void visit(DesignatorExpr&) = 0;
    virtual void visit(IsExpr&)         = 0;
    virtual void visit(InExpr&)         = 0;

    virtual void visit(FieldSelector&) = 0;
    virtual void visit(IndexSelector&) = 0;
    virtual void visit(DerefSelector&) = 0;

    virtual void visit(CaseAlternative&) = 0;
    virtual void visit(CaseLabel&) = 0;
};
