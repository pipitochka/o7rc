#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <sstream>

#include "test_factory.h"

struct FrontendConfig {
    TokenizerKind tok;
    ParserKind par;
};

class ParserStructureTest : public ::testing::TestWithParam<FrontendConfig> {
protected:
    std::unique_ptr<Module> parse(const std::string& code) {
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

TEST_P(ParserStructureTest, ParsesEmptyModule) {
    auto mod = parse("MODULE M; END M.");
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->name, "M");
    EXPECT_EQ(mod->endName, "M");
}

TEST_P(ParserStructureTest, ParsesImports) {
    auto mod = parse("MODULE M; IMPORT Out, Math := SomeMath; END M.");
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->imports.size(), 2u);
    EXPECT_EQ(mod->imports[0].name, "Out");
    EXPECT_EQ(mod->imports[1].alias, "Math");
    EXPECT_EQ(mod->imports[1].name, "SomeMath");
}

TEST_P(ParserStructureTest, ParsesVarDeclarations) {
    auto mod = parse("MODULE M; VAR x, y: INTEGER; b*: BOOLEAN; END M.");
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->decls.size(), 2u);

    auto* v1 = dynamic_cast<VarDecl*>(mod->decls[0].get());
    ASSERT_NE(v1, nullptr);
    ASSERT_EQ(v1->names.size(), 2u);
    EXPECT_EQ(v1->names[0], "x");
    EXPECT_EQ(v1->names[1], "y");

    auto* v2 = dynamic_cast<VarDecl*>(mod->decls[1].get());
    ASSERT_NE(v2, nullptr);
    ASSERT_EQ(v2->names.size(), 1u);
    EXPECT_EQ(v2->names[0], "b");
}

TEST_P(ParserStructureTest, ParsesConstDeclarations) {
    auto mod = parse("MODULE M; CONST Max = 100; Pi = 3.14; END M.");
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->decls.size(), 2u);

    auto* c1 = dynamic_cast<ConstDecl*>(mod->decls[0].get());
    ASSERT_NE(c1, nullptr);
    EXPECT_EQ(c1->name, "Max");
    auto* v1 = dynamic_cast<LiteralExpr*>(c1->value.get());
    ASSERT_NE(v1, nullptr);
    EXPECT_EQ(v1->intValue, 100);

    auto* c2 = dynamic_cast<ConstDecl*>(mod->decls[1].get());
    ASSERT_NE(c2, nullptr);
    EXPECT_EQ(c2->name, "Pi");
    auto* v2 = dynamic_cast<LiteralExpr*>(c2->value.get());
    ASSERT_NE(v2, nullptr);
    EXPECT_DOUBLE_EQ(v2->realValue, 3.14);
}

TEST_P(ParserStructureTest, ParsesExportFlags) {
    auto mod = parse(
        "MODULE M; "
        "CONST Exported* = 1; NotExported = 2; "
        "TYPE Pub* = INTEGER; "
        "VAR a*: INTEGER; b: BOOLEAN; "
        "PROCEDURE Do*; BEGIN END Do; "
        "END M.");
    ASSERT_NE(mod, nullptr);

    auto* c1 = dynamic_cast<ConstDecl*>(mod->decls[0].get());
    EXPECT_TRUE(c1->exported);
    auto* c2 = dynamic_cast<ConstDecl*>(mod->decls[1].get());
    EXPECT_FALSE(c2->exported);

    auto* t1 = dynamic_cast<TypeDecl*>(mod->decls[2].get());
    EXPECT_TRUE(t1->exported);

    auto* vd1 = dynamic_cast<VarDecl*>(mod->decls[3].get());
    ASSERT_EQ(vd1->exportedFlags.size(), 1u);
    EXPECT_TRUE(vd1->exportedFlags[0]);

    auto* vd2 = dynamic_cast<VarDecl*>(mod->decls[4].get());
    ASSERT_EQ(vd2->exportedFlags.size(), 1u);
    EXPECT_FALSE(vd2->exportedFlags[0]);

    auto* p1 = dynamic_cast<ProcDecl*>(mod->decls[5].get());
    EXPECT_TRUE(p1->exported);
}

TEST_P(ParserStructureTest, ParsesProcedureStructure) {
    auto mod = parse(
        "MODULE M; "
        "PROCEDURE Compute(n: INTEGER): INTEGER; "
        "  VAR result: INTEGER; "
        "BEGIN "
        "  result := n "
        "RETURN result "
        "END Compute; "
        "END M.");
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->decls.size(), 1u);

    auto* proc = dynamic_cast<ProcDecl*>(mod->decls[0].get());
    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->name, "Compute");

    auto* sig = dynamic_cast<ProcType*>(proc->type.get());
    ASSERT_NE(sig, nullptr);
    ASSERT_EQ(sig->params.size(), 1u);
    EXPECT_EQ(sig->params[0]->names[0], "n");
    ASSERT_NE(sig->type, nullptr);
    EXPECT_EQ(sig->type->name, "INTEGER");

    ASSERT_EQ(proc->decls.size(), 1u);
    auto* localVar = dynamic_cast<VarDecl*>(proc->decls[0].get());
    ASSERT_NE(localVar, nullptr);
    EXPECT_EQ(localVar->names[0], "result");
    EXPECT_GE(proc->body.size(), 1u);
}

TEST_P(ParserStructureTest, ParsesMixedDeclarations) {
    auto mod = parse(
        "MODULE M; "
        "CONST N = 5; "
        "TYPE Arr = ARRAY 5 OF INTEGER; "
        "VAR data: Arr; "
        "END M.");
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->decls.size(), 3u);
    EXPECT_NE(dynamic_cast<ConstDecl*>(mod->decls[0].get()), nullptr);
    EXPECT_NE(dynamic_cast<TypeDecl*>(mod->decls[1].get()), nullptr);
    EXPECT_NE(dynamic_cast<VarDecl*>(mod->decls[2].get()), nullptr);
}

TEST_P(ParserStructureTest, ParsesModuleWithBodyAndImport) {
    auto mod = parse(
        "MODULE M; "
        "IMPORT Out; "
        "VAR x: INTEGER; "
        "BEGIN x := 42 "
        "END M.");
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->name, "M");
    ASSERT_EQ(mod->imports.size(), 1u);
    EXPECT_EQ(mod->imports[0].name, "Out");
    ASSERT_EQ(mod->decls.size(), 1u);
    ASSERT_EQ(mod->block.size(), 1u);

    auto* stmt = dynamic_cast<AssignStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    auto* val = dynamic_cast<LiteralExpr*>(stmt->rhs.get());
    EXPECT_EQ(val->intValue, 42);
}

INSTANTIATE_TEST_SUITE_P(
    AllConfigs, ParserStructureTest,
    ::testing::ValuesIn(allConfigs()),
    configName
);
