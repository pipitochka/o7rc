#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include <parser/impl/bison/BisonParser.h>
#include <tokenizer/impl/flex/FlexTokenizer.h>
#include <util/ast/Ast.h>

namespace {

class BisonParserExprTest : public ::testing::Test {
protected:
    std::unique_ptr<Module> parseModule(const std::string& body) {
        std::stringstream ss("MODULE Test; VAR x: INTEGER; BEGIN " + body + " END Test.");
        auto tokenizer = std::make_unique<FlexTokenizer>(ss);
        BisonParser parser;
        return parser.parse(std::move(tokenizer));
    }

    Expr* getFirstAssignmentExpr(Module* mod) {
        if (!mod || mod->block.empty()) return nullptr;
        auto* assign = dynamic_cast<AssignStmt*>(mod->block[0].get());
        if (!assign) return nullptr;
        return assign->rhs.get();
    }
};

TEST_F(BisonParserExprTest, CorrectlyParsesIntegerLiteral) {
    auto mod = parseModule("x := 123");
    auto* expr = getFirstAssignmentExpr(mod.get());

    auto* lit = dynamic_cast<LiteralExpr*>(expr);
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::Int);
    EXPECT_EQ(lit->intValue, 123);
}

TEST_F(BisonParserExprTest, RespectsPrecedenceMulOverAdd) {
    auto mod = parseModule("x := 2 + 3 * 4");
    auto* root = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->op, BinaryExpr::Op::Add);

    auto* lhs = dynamic_cast<LiteralExpr*>(root->lhs.get());
    auto* rhs = dynamic_cast<BinaryExpr*>(root->rhs.get());

    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->intValue, 2);

    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(rhs->op, BinaryExpr::Op::Mul);

    auto* r_lhs = dynamic_cast<LiteralExpr*>(rhs->lhs.get());
    auto* r_rhs = dynamic_cast<LiteralExpr*>(rhs->rhs.get());
    EXPECT_EQ(r_lhs->intValue, 3);
    EXPECT_EQ(r_rhs->intValue, 4);
}

TEST_F(BisonParserExprTest, RespectsParentheses) {
    auto mod = parseModule("x := (2 + 3) * 4");
    auto* root = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->op, BinaryExpr::Op::Mul);

    auto* lhs = dynamic_cast<BinaryExpr*>(root->lhs.get());
    auto* rhs = dynamic_cast<LiteralExpr*>(root->rhs.get());

    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->op, BinaryExpr::Op::Add);
    EXPECT_EQ(rhs->intValue, 4);
}

TEST_F(BisonParserExprTest, ParsesDesignatorChain) {
    auto mod = parseModule("x := a.b[10]^.c");

    auto* des = dynamic_cast<DesignatorExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(des, nullptr);
    EXPECT_EQ(des->baseName, "a");

    ASSERT_EQ(des->selectors.size(), 4);

    auto* s1 = dynamic_cast<FieldSelector*>(des->selectors[0].get());
    ASSERT_NE(s1, nullptr);
    EXPECT_EQ(s1->name, "b");

    auto* s2 = dynamic_cast<IndexSelector*>(des->selectors[1].get());
    ASSERT_NE(s2, nullptr);
    ASSERT_EQ(s2->index.size(), 1);
    auto* idxVal = dynamic_cast<LiteralExpr*>(s2->index[0].get());
    EXPECT_EQ(idxVal->intValue, 10);

    auto* s3 = dynamic_cast<DerefSelector*>(des->selectors[2].get());
    ASSERT_NE(s3, nullptr);

    auto* s4 = dynamic_cast<FieldSelector*>(des->selectors[3].get());
    ASSERT_NE(s4, nullptr);
    EXPECT_EQ(s4->name, "c");
}

TEST_F(BisonParserExprTest, ParsesFunctionCall) {
    auto mod = parseModule("x := Math.Sin(3.14)");

    auto* call = dynamic_cast<CallExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(call, nullptr);

    auto* callee = dynamic_cast<DesignatorExpr*>(call->callee.get());
    EXPECT_EQ(callee->baseName, "Math.Sin");

    ASSERT_EQ(call->args.size(), 1);
    auto* arg = dynamic_cast<LiteralExpr*>(call->args[0].get());
    EXPECT_DOUBLE_EQ(arg->realValue, 3.14);
}

} // namespace
