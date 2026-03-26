#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include "test_factory.h"

namespace {

struct FrontendConfig {
    TokenizerKind tok;
    ParserKind par;
};

class ParserTypeTest : public ::testing::TestWithParam<FrontendConfig> {
protected:
    std::unique_ptr<Module> parseDecls(const std::string& decls) {
        auto ss = std::make_unique<std::stringstream>(
            "MODULE Test; " + decls + " END Test.");
        auto tokenizer = makeTokenizer(GetParam().tok, *ss);
        auto parser = makeParser(GetParam().par);
        streams_.push_back(std::move(ss));
        return parser->parse(std::move(tokenizer));
    }

    Decl* getDecl(Module* mod, size_t index) {
        if (!mod || index >= mod->decls.size()) return nullptr;
        return mod->decls[index].get();
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

TEST_P(ParserTypeTest, ParsesArrayType) {
    auto mod = parseDecls("VAR a: ARRAY 10 OF INTEGER;");
    auto* vd = dynamic_cast<VarDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(vd, nullptr);
    auto* arr = dynamic_cast<ArrayType*>(vd->type.get());
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(arr->length.size(), 1u);
    auto* len = dynamic_cast<LiteralExpr*>(arr->length[0].get());
    EXPECT_EQ(len->intValue, 10);
    auto* elem = dynamic_cast<NamedType*>(arr->elemType.get());
    EXPECT_EQ(elem->name, "INTEGER");
}

TEST_P(ParserTypeTest, ParsesMultiDimArray) {
    auto mod = parseDecls("VAR m: ARRAY 5, 8 OF CHAR;");
    auto* vd = dynamic_cast<VarDecl*>(getDecl(mod.get(), 0));
    auto* arr = dynamic_cast<ArrayType*>(vd->type.get());
    ASSERT_EQ(arr->length.size(), 2u);
}

TEST_P(ParserTypeTest, ParsesRecordType) {
    auto mod = parseDecls("TYPE Point = RECORD x, y: INTEGER; visible: BOOLEAN END;");
    auto* td = dynamic_cast<TypeDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(td, nullptr);
    auto* rec = dynamic_cast<RecordType*>(td->type.get());
    ASSERT_NE(rec, nullptr);
    ASSERT_EQ(rec->fields.size(), 2u);
    EXPECT_EQ(rec->fields[0]->names.size(), 2u);
}

TEST_P(ParserTypeTest, ParsesRecordWithBaseType) {
    auto mod = parseDecls(
        "TYPE Point = RECORD x, y: INTEGER END; "
        "Point3D = RECORD(Point) z: INTEGER END;");
    auto* td = dynamic_cast<TypeDecl*>(getDecl(mod.get(), 1));
    ASSERT_NE(td, nullptr);
    auto* rec = dynamic_cast<RecordType*>(td->type.get());
    ASSERT_NE(rec, nullptr);
    auto* base = dynamic_cast<NamedType*>(rec->baseType.get());
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->name, "Point");
}

TEST_P(ParserTypeTest, ParsesPointerType) {
    auto mod = parseDecls("TYPE NodePtr = POINTER TO Node;");
    auto* td = dynamic_cast<TypeDecl*>(getDecl(mod.get(), 0));
    auto* ptr = dynamic_cast<PointerType*>(td->type.get());
    ASSERT_NE(ptr, nullptr);
    auto* base = dynamic_cast<NamedType*>(ptr->baseType.get());
    EXPECT_EQ(base->name, "Node");
}

TEST_P(ParserTypeTest, ParsesProcedureWithVarParams) {
    auto mod = parseDecls("PROCEDURE Swap(VAR a, b: INTEGER); BEGIN END Swap;");
    auto* proc = dynamic_cast<ProcDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(proc, nullptr);
    auto* sig = dynamic_cast<ProcType*>(proc->type.get());
    ASSERT_NE(sig, nullptr);
    ASSERT_EQ(sig->params.size(), 1u);
    EXPECT_TRUE(sig->params[0]->isVar);
}

TEST_P(ParserTypeTest, ParsesProcedureWithReturnType) {
    auto mod = parseDecls(
        "PROCEDURE Max(a, b: INTEGER): INTEGER; BEGIN RETURN a END Max;");
    auto* proc = dynamic_cast<ProcDecl*>(getDecl(mod.get(), 0));
    auto* sig = dynamic_cast<ProcType*>(proc->type.get());
    ASSERT_NE(sig, nullptr);
    ASSERT_NE(sig->type, nullptr);
    EXPECT_EQ(sig->type->name, "INTEGER");
}

TEST_P(ParserTypeTest, ParsesMultipleParamSections) {
    auto mod = parseDecls(
        "PROCEDURE P(a: INTEGER; VAR b: BOOLEAN; c, d: REAL); BEGIN END P;");
    auto* proc = dynamic_cast<ProcDecl*>(getDecl(mod.get(), 0));
    auto* sig = dynamic_cast<ProcType*>(proc->type.get());
    ASSERT_NE(sig, nullptr);
    ASSERT_EQ(sig->params.size(), 3u);
    EXPECT_FALSE(sig->params[0]->isVar);
    EXPECT_TRUE(sig->params[1]->isVar);
    EXPECT_EQ(sig->params[2]->names.size(), 2u);
}

TEST_P(ParserTypeTest, ParsesProcedureType) {
    auto mod = parseDecls("TYPE BinaryOp = PROCEDURE(a, b: INTEGER): INTEGER;");
    auto* td = dynamic_cast<TypeDecl*>(getDecl(mod.get(), 0));
    auto* pt = dynamic_cast<ProcType*>(td->type.get());
    ASSERT_NE(pt, nullptr);
    ASSERT_EQ(pt->params.size(), 1u);
    ASSERT_NE(pt->type, nullptr);
    EXPECT_EQ(pt->type->name, "INTEGER");
}

INSTANTIATE_TEST_SUITE_P(AllConfigs, ParserTypeTest,
    ::testing::ValuesIn(allConfigs()), configName);

} // namespace
