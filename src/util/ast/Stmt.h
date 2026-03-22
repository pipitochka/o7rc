#pragma once
#include <util/ast/IVisitor.h>
#include <util/ast/Node.h>

#include <string>
#include <vector>

struct Branch {
    ExprPtr cond;
    std::vector<StmtPtr> body;
};

struct AssignStmt final : Stmt {
    Ptr<DesignatorExpr> lhs;
    ExprPtr rhs;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct CallStmt final : Stmt {
    Ptr<CallExpr> call;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct ReturnStmt final : Stmt {
    ExprPtr value; // nullptr => RETURN;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct IfStmt final : Stmt {
    std::vector<Branch> branches;
    std::vector<StmtPtr> elseBody;

    void accept(IVisitor &v) override { v.visit(*this); }
};

struct WhileStmt final : Stmt {
    std::vector<Branch> branches;
    ExprPtr cond;
    std::vector<StmtPtr> body;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct RepeatStmt final : Stmt {
    std::vector<StmtPtr> body;
    ExprPtr untilCond;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct ForStmt final : Stmt {
    std::string varName;
    ExprPtr from;
    ExprPtr to;
    ExprPtr by; // nullptr => default 1
    std::vector<StmtPtr> body;

    void accept(IVisitor &v) override { v.visit(*this); }
};

// CASE helpers
struct CaseLabel final : Node {
    ExprPtr from = nullptr;
    ExprPtr to = nullptr; // nullptr => single label
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct CaseAlternative final : Node {
    std::vector<Ptr<CaseLabel>> labels;
    std::vector<Ptr<Stmt>> body;
    void accept(IVisitor &v) override { v.visit(*this); }
};

struct CaseStmt final : Stmt {
    ExprPtr expr;
    std::vector<Ptr<CaseAlternative>> alts;
    void accept(IVisitor &v) override { v.visit(*this); }
};
