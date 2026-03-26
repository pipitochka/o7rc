#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include "test_factory.h"

namespace {

struct FrontendConfig {
    TokenizerKind tok;
    ParserKind par;
};

class ParserStmtTest : public ::testing::TestWithParam<FrontendConfig> {
protected:
    std::unique_ptr<Module> parseBlock(const std::string& code) {
        auto ss = std::make_unique<std::stringstream>(
            "MODULE Test; VAR x: INTEGER; BEGIN " + code + " END Test.");
        auto tokenizer = makeTokenizer(GetParam().tok, *ss);
        auto parser = makeParser(GetParam().par);
        streams_.push_back(std::move(ss));
        return parser->parse(std::move(tokenizer));
    }

    std::unique_ptr<Module> parseFull(const std::string& code) {
        auto ss = std::make_unique<std::stringstream>(code);
        auto tokenizer = makeTokenizer(GetParam().tok, *ss);
        auto parser = makeParser(GetParam().par);
        streams_.push_back(std::move(ss));
        return parser->parse(std::move(tokenizer));
    }
    std::vector<std::unique_ptr<std::stringstream>> streams_;
};

static std::vector<FrontendConfig> allConfigs() {
    std::vector<FrontendConfig> v;
    for (auto t : availableTokenizers())
        for (auto p : availableParsers())
            v.push_back({t, p});
    return v;
}

static std::string configName(const ::testing::TestParamInfo<FrontendConfig>& info) {
    return std::string(tokenizerName(info.param.tok)) + "_" +
           parserName(info.param.par);
}

TEST_P(ParserStmtTest, ParsesAssignStatement) {
    auto mod = parseBlock("x := 10");
    ASSERT_EQ(mod->block.size(), 1u);
    auto* stmt = dynamic_cast<AssignStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->lhs->baseName, "x");
    auto* lit = dynamic_cast<LiteralExpr*>(stmt->rhs.get());
    EXPECT_EQ(lit->intValue, 10);
}

TEST_P(ParserStmtTest, ParsesIfElseStatement) {
    auto mod = parseBlock("IF x > 0 THEN x := 1 ELSE x := 2 END");
    auto* stmt = dynamic_cast<IfStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->branches.size(), 1u);
    ASSERT_EQ(stmt->elseBody.size(), 1u);
}

TEST_P(ParserStmtTest, ParsesIfElsifStatement) {
    auto mod = parseBlock(
        "IF x = 1 THEN x := 10 "
        "ELSIF x = 2 THEN x := 20 "
        "ELSIF x = 3 THEN x := 30 "
        "ELSE x := 0 END");
    auto* stmt = dynamic_cast<IfStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->branches.size(), 3u);
    ASSERT_EQ(stmt->elseBody.size(), 1u);
}

TEST_P(ParserStmtTest, ParsesIfWithoutElse) {
    auto mod = parseBlock("IF x > 0 THEN x := 1 END");
    auto* stmt = dynamic_cast<IfStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->branches.size(), 1u);
    EXPECT_TRUE(stmt->elseBody.empty());
}

TEST_P(ParserStmtTest, ParsesWhileLoop) {
    auto mod = parseBlock("WHILE x < 10 DO x := x + 1 END");
    auto* stmt = dynamic_cast<WhileStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    auto* cond = dynamic_cast<BinaryExpr*>(stmt->cond.get());
    EXPECT_EQ(cond->op, BinaryExpr::Op::Lt);
    ASSERT_EQ(stmt->body.size(), 1u);
}

TEST_P(ParserStmtTest, ParsesRepeatUntilLoop) {
    auto mod = parseBlock("REPEAT x := x + 1 UNTIL x >= 10");
    auto* stmt = dynamic_cast<RepeatStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->body.size(), 1u);
    auto* cond = dynamic_cast<BinaryExpr*>(stmt->untilCond.get());
    EXPECT_EQ(cond->op, BinaryExpr::Op::Ge);
}

TEST_P(ParserStmtTest, ParsesForLoop) {
    auto mod = parseBlock("FOR x := 1 TO 10 BY 2 DO x := 0 END");
    auto* stmt = dynamic_cast<ForStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->varName, "x");
    auto* by = dynamic_cast<LiteralExpr*>(stmt->by.get());
    ASSERT_NE(by, nullptr);
    EXPECT_EQ(by->intValue, 2);
}

TEST_P(ParserStmtTest, ParsesForLoopWithoutBy) {
    auto mod = parseBlock("FOR x := 0 TO 5 DO x := 0 END");
    auto* stmt = dynamic_cast<ForStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->by, nullptr);
}

TEST_P(ParserStmtTest, ParsesCaseStatement) {
    auto mod = parseBlock("CASE x OF 1: x := 10 | 2, 3: x := 20 | 4..6: x := 30 END");
    auto* stmt = dynamic_cast<CaseStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->alts.size(), 3u);
}

TEST_P(ParserStmtTest, ParsesCallStatement) {
    auto mod = parseBlock("Write(x)");
    ASSERT_EQ(mod->block.size(), 1u);
    auto* stmt = dynamic_cast<CallStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->designator->baseName, "Write");
    ASSERT_EQ(stmt->designator->selectors.size(), 1u);
}

TEST_P(ParserStmtTest, ParsesReturnStatement) {
    auto mod = parseFull(
        "MODULE Test; "
        "PROCEDURE Add(a, b: INTEGER): INTEGER; "
        "BEGIN RETURN a + b "
        "END Add; "
        "END Test.");
    ASSERT_NE(mod, nullptr);
    auto* proc = dynamic_cast<ProcDecl*>(mod->decls[0].get());
    ASSERT_NE(proc, nullptr);
    ASSERT_GE(proc->body.size(), 1u);
    auto* ret = dynamic_cast<ReturnStmt*>(proc->body[0].get());
    ASSERT_NE(ret, nullptr);
    auto* val = dynamic_cast<BinaryExpr*>(ret->value.get());
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->op, BinaryExpr::Op::Add);
}

TEST_P(ParserStmtTest, ParsesMultipleStatements) {
    auto mod = parseBlock("x := 1; x := 2; x := 3");
    ASSERT_EQ(mod->block.size(), 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_NE(dynamic_cast<AssignStmt*>(mod->block[i].get()), nullptr);
    }
}

INSTANTIATE_TEST_SUITE_P(AllConfigs, ParserStmtTest,
    ::testing::ValuesIn(allConfigs()), configName);

} // namespace
