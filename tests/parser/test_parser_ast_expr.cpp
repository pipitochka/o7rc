#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include "test_factory.h"

namespace {

struct FrontendConfig {
    TokenizerKind tok;
    ParserKind par;
};

class ParserExprTest : public ::testing::TestWithParam<FrontendConfig> {
protected:
    std::unique_ptr<Module> parseModule(const std::string& body) {
        auto ss = std::make_unique<std::stringstream>(
            "MODULE Test; VAR x: INTEGER; BEGIN " + body + " END Test.");
        auto tokenizer = makeTokenizer(GetParam().tok, *ss);
        auto parser = makeParser(GetParam().par);
        streams_.push_back(std::move(ss));
        return parser->parse(std::move(tokenizer));
    }

    Expr* getFirstAssignmentExpr(Module* mod) {
        if (!mod || mod->block.empty()) return nullptr;
        auto* assign = dynamic_cast<AssignStmt*>(mod->block[0].get());
        return assign ? assign->rhs.get() : nullptr;
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

TEST_P(ParserExprTest, CorrectlyParsesIntegerLiteral) {
    auto mod = parseModule("x := 123");
    auto* lit = dynamic_cast<LiteralExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::Int);
    EXPECT_EQ(lit->intValue, 123);
}

TEST_P(ParserExprTest, ParsesRealLiteral) {
    auto mod = parseModule("x := 2.718");
    auto* lit = dynamic_cast<LiteralExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::Real);
    EXPECT_DOUBLE_EQ(lit->realValue, 2.718);
}

TEST_P(ParserExprTest, ParsesHighPrecisionRealLiteralAsDouble) {
    auto mod = parseModule("x := 1.2345678901234567");
    auto* lit = dynamic_cast<LiteralExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::Real);
    EXPECT_DOUBLE_EQ(lit->realValue, 1.2345678901234567);
}

TEST_P(ParserExprTest, ParsesRealLiteralScientificNotation) {
    auto mod = parseModule("x := 3.141592653589793e0");
    auto* lit = dynamic_cast<LiteralExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::Real);
    EXPECT_DOUBLE_EQ(lit->realValue, 3.141592653589793);
}

TEST_P(ParserExprTest, ParsesStringLiteral) {
    auto mod = parseModule("x := \"hello\"");
    auto* lit = dynamic_cast<LiteralExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::String);
    EXPECT_EQ(lit->strValue, "hello");
}

TEST_P(ParserExprTest, ParsesBoolLiterals) {
    auto mod = parseModule("x := TRUE; x := FALSE");
    ASSERT_GE(mod->block.size(), 2u);
    auto* a1 = dynamic_cast<AssignStmt*>(mod->block[0].get());
    auto* l1 = dynamic_cast<LiteralExpr*>(a1->rhs.get());
    ASSERT_NE(l1, nullptr);
    EXPECT_TRUE(l1->boolValue);
    auto* a2 = dynamic_cast<AssignStmt*>(mod->block[1].get());
    auto* l2 = dynamic_cast<LiteralExpr*>(a2->rhs.get());
    ASSERT_NE(l2, nullptr);
    EXPECT_FALSE(l2->boolValue);
}

TEST_P(ParserExprTest, ParsesNilLiteral) {
    auto mod = parseModule("x := NIL");
    auto* lit = dynamic_cast<LiteralExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::Nil);
}

TEST_P(ParserExprTest, RespectsPrecedenceMulOverAdd) {
    auto mod = parseModule("x := 2 + 3 * 4");
    auto* root = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->op, BinaryExpr::Op::Add);
    auto* rhs = dynamic_cast<BinaryExpr*>(root->rhs.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(rhs->op, BinaryExpr::Op::Mul);
}

TEST_P(ParserExprTest, RespectsParentheses) {
    auto mod = parseModule("x := (2 + 3) * 4");
    auto* root = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->op, BinaryExpr::Op::Mul);
    auto* lhs = dynamic_cast<BinaryExpr*>(root->lhs.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->op, BinaryExpr::Op::Add);
}

TEST_P(ParserExprTest, ParsesUnaryNegation) {
    auto mod = parseModule("x := -5");
    auto* unary = dynamic_cast<UnaryExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op, UnaryExpr::Op::Neg);
    auto* val = dynamic_cast<LiteralExpr*>(unary->rhs.get());
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->intValue, 5);
}

TEST_P(ParserExprTest, ParsesLogicalNot) {
    auto mod = parseModule("x := ~x");
    auto* unary = dynamic_cast<UnaryExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op, UnaryExpr::Op::Not);
}

TEST_P(ParserExprTest, ParsesSetExpression) {
    auto mod = parseModule("x := {1, 3..7}");
    auto* set = dynamic_cast<SetExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(set, nullptr);
    ASSERT_EQ(set->elements.size(), 2u);
    EXPECT_EQ(set->elements[0].high, nullptr);
    EXPECT_NE(set->elements[1].high, nullptr);
}

TEST_P(ParserExprTest, ParsesDivAndMod) {
    auto m1 = parseModule("x := 10 DIV 3");
    auto* e1 = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(m1.get()));
    ASSERT_NE(e1, nullptr);
    EXPECT_EQ(e1->op, BinaryExpr::Op::IDiv);

    auto m2 = parseModule("x := 10 MOD 3");
    auto* e2 = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(m2.get()));
    ASSERT_NE(e2, nullptr);
    EXPECT_EQ(e2->op, BinaryExpr::Op::Mod);
}

TEST_P(ParserExprTest, ParsesLogicalAndOr) {
    auto mod = parseModule("x := x & x OR x");
    auto* root = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->op, BinaryExpr::Op::Or);
    auto* lhs = dynamic_cast<BinaryExpr*>(root->lhs.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->op, BinaryExpr::Op::And);
}

TEST_P(ParserExprTest, ParsesRelationalOperators) {
    struct RC { const char* code; BinaryExpr::Op op; };
    RC cases[] = {
        {"x := x = x",  BinaryExpr::Op::Eq},
        {"x := x # x",  BinaryExpr::Op::Neq},
        {"x := x < x",  BinaryExpr::Op::Lt},
        {"x := x <= x", BinaryExpr::Op::Le},
        {"x := x > x",  BinaryExpr::Op::Gt},
        {"x := x >= x", BinaryExpr::Op::Ge},
    };
    for (auto& c : cases) {
        auto m = parseModule(c.code);
        auto* e = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(m.get()));
        ASSERT_NE(e, nullptr) << c.code;
        EXPECT_EQ(e->op, c.op) << c.code;
    }
}

TEST_P(ParserExprTest, LeftAssociativityOfAddition) {
    auto mod = parseModule("x := 1 + 2 + 3");
    auto* root = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->op, BinaryExpr::Op::Add);
    auto* lhs = dynamic_cast<BinaryExpr*>(root->lhs.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->op, BinaryExpr::Op::Add);
}

TEST_P(ParserExprTest, ParsesDesignatorChain) {
    auto mod = parseModule("x := a.b[10]^.c");
    auto* des = dynamic_cast<DesignatorExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(des, nullptr);
    EXPECT_EQ(des->baseName, "a.b");
    ASSERT_EQ(des->selectors.size(), 3u);
    EXPECT_NE(dynamic_cast<IndexSelector*>(des->selectors[0].get()), nullptr);
    EXPECT_NE(dynamic_cast<DerefSelector*>(des->selectors[1].get()), nullptr);
    EXPECT_NE(dynamic_cast<FieldSelector*>(des->selectors[2].get()), nullptr);
}

TEST_P(ParserExprTest, ParsesFunctionCall) {
    auto mod = parseModule("x := Math.Sin(3.14)");
    auto* des = dynamic_cast<DesignatorExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(des, nullptr);
    EXPECT_EQ(des->baseName, "Math.Sin");
    ASSERT_EQ(des->selectors.size(), 1u);
    auto* args = dynamic_cast<ArgsSelector*>(des->selectors[0].get());
    ASSERT_NE(args, nullptr);
    ASSERT_EQ(args->args.size(), 1u);
}

INSTANTIATE_TEST_SUITE_P(AllConfigs, ParserExprTest,
    ::testing::ValuesIn(allConfigs()), configName);

} // namespace
