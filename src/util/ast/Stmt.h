#pragma once
#include <util/ast/Node.h>
#include <util/ast/IVisitor.h>

#include <string>
#include <vector>

struct AssignStmt final : Stmt {
    Ptr<DesignatorExpr> lhs;
    ExprPtr rhs;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct CallStmt final : Stmt {
    Ptr<CallExpr> call;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct ReturnStmt final : Stmt {
    ExprPtr value; // nullptr => RETURN;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct IfStmt final : Stmt {
    struct Branch {
        ExprPtr cond;
        Ptr<Block> body;
    };
    std::vector<Branch> branches;
    Ptr<Block> elseBody; // nullptr if absent

    void accept(IVisitor& v) override { v.visit(*this); }
};

struct WhileStmt final : Stmt {
    ExprPtr cond;
    Ptr<Block> body;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct RepeatStmt final : Stmt {
    Ptr<Block> body;
    ExprPtr untilCond;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct ForStmt final : Stmt {
    std::string varName;
    ExprPtr from;
    ExprPtr to;
    ExprPtr by;      // nullptr => default 1
    Ptr<Block> body;

    void accept(IVisitor& v) override { v.visit(*this); }
};

// CASE helpers
struct CaseLabel final : Node {
    ExprPtr from;
    ExprPtr to; // nullptr => single label
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct CaseAlternative final : Node {
    std::vector<Ptr<CaseLabel>> labels;
    Ptr<Block> body;
    void accept(IVisitor& v) override { v.visit(*this); }
};

struct CaseStmt final : Stmt {
    ExprPtr expr;
    std::vector<Ptr<CaseAlternative>> alts;
    Ptr<Block> elseBody;
    void accept(IVisitor& v) override { v.visit(*this); }
};
