#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include <parser/impl/bison/BisonParser.h>
#include <tokenizer/impl/flex/FlexTokenizer.h>
#include <util/ast/Ast.h>

namespace {

class BisonParserStmtTest : public ::testing::Test {
protected:
    std::unique_ptr<Module> parseBlock(const std::string& code) {
        std::stringstream ss("MODULE Test; VAR x: INTEGER; BEGIN " + code + " END Test.");
        auto tokenizer = std::make_unique<FlexTokenizer>(ss);
        BisonParser parser;
        return parser.parse(std::move(tokenizer));
    }
};

TEST_F(BisonParserStmtTest, ParsesAssignStatement) {
    auto mod = parseBlock("x := 10");
    ASSERT_EQ(mod->block.size(), 1);

    auto* stmt = dynamic_cast<AssignStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    EXPECT_EQ(stmt->lhs->baseName, "x");
    auto* lit = dynamic_cast<LiteralExpr*>(stmt->rhs.get());
    EXPECT_EQ(lit->intValue, 10);
}

TEST_F(BisonParserStmtTest, ParsesIfElseStatement) {
    auto mod = parseBlock("IF x > 0 THEN x := 1 ELSE x := 2 END");

    auto* stmt = dynamic_cast<IfStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    ASSERT_EQ(stmt->branches.size(), 1);
    auto* cond = dynamic_cast<BinaryExpr*>(stmt->branches[0].cond.get());
    EXPECT_EQ(cond->op, BinaryExpr::Op::Gt);

    ASSERT_EQ(stmt->branches[0].body.size(), 1); // x := 1 inside

    ASSERT_EQ(stmt->elseBody.size(), 1); // x := 2 inside
    auto* elseStmt = dynamic_cast<AssignStmt*>(stmt->elseBody[0].get());
    auto* val = dynamic_cast<LiteralExpr*>(elseStmt->rhs.get());
    EXPECT_EQ(val->intValue, 2);
}

TEST_F(BisonParserStmtTest, ParsesWhileLoop) {
    auto mod = parseBlock("WHILE x < 10 DO x := x + 1 END");

    auto* stmt = dynamic_cast<WhileStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    auto* cond = dynamic_cast<BinaryExpr*>(stmt->cond.get());
    EXPECT_EQ(cond->op, BinaryExpr::Op::Lt);

    ASSERT_EQ(stmt->body.size(), 1);
}

TEST_F(BisonParserStmtTest, ParsesForLoop) {
    auto mod = parseBlock("FOR x := 1 TO 10 BY 2 DO x := 0 END");

    auto* stmt = dynamic_cast<ForStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    EXPECT_EQ(stmt->varName, "x");

    auto* from = dynamic_cast<LiteralExpr*>(stmt->from.get());
    EXPECT_EQ(from->intValue, 1);

    auto* to = dynamic_cast<LiteralExpr*>(stmt->to.get());
    EXPECT_EQ(to->intValue, 10);

    auto* by = dynamic_cast<LiteralExpr*>(stmt->by.get());
    ASSERT_NE(by, nullptr);
    EXPECT_EQ(by->intValue, 2);
}

} // namespace
