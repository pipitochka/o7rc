#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include <parser/impl/bison/BisonParser.h>
#include <tokenizer/impl/flex/FlexTokenizer.h>
#include <util/ast/Ast.h>

namespace {

class BisonParserTypeTest : public ::testing::Test {
protected:
    std::unique_ptr<Module> parseDecls(const std::string& decls) {
        std::stringstream ss("MODULE Test; " + decls + " END Test.");
        auto tokenizer = std::make_unique<FlexTokenizer>(ss);
        BisonParser parser;
        return parser.parse(std::move(tokenizer));
    }

    Decl* getDecl(Module* mod, int index) {
        if (!mod || index >= mod->decls.size()) return nullptr;
        return mod->decls[index].get();
    }
};

TEST_F(BisonParserTypeTest, ParsesArrayType) {
    auto mod = parseDecls("VAR a: ARRAY 10 OF INTEGER;");

    auto* varDecl = dynamic_cast<VarDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(varDecl, nullptr);

    auto* arrType = dynamic_cast<ArrayType*>(varDecl->type.get());
    ASSERT_NE(arrType, nullptr);

    ASSERT_EQ(arrType->length.size(), 1);
    auto* len = dynamic_cast<LiteralExpr*>(arrType->length[0].get());
    EXPECT_EQ(len->intValue, 10);

    auto* elem = dynamic_cast<NamedType*>(arrType->elemType.get());
    EXPECT_EQ(elem->name, "INTEGER");
}

TEST_F(BisonParserTypeTest, ParsesMultiDimArray) {
    auto mod = parseDecls("VAR m: ARRAY 5, 8 OF CHAR;");
    auto* varDecl = dynamic_cast<VarDecl*>(getDecl(mod.get(), 0));
    auto* arrType = dynamic_cast<ArrayType*>(varDecl->type.get());

    ASSERT_EQ(arrType->length.size(), 2);
    auto* l1 = dynamic_cast<LiteralExpr*>(arrType->length[0].get());
    auto* l2 = dynamic_cast<LiteralExpr*>(arrType->length[1].get());

    EXPECT_EQ(l1->intValue, 5);
    EXPECT_EQ(l2->intValue, 8);
}

TEST_F(BisonParserTypeTest, ParsesRecordType) {
    auto mod = parseDecls("TYPE Point = RECORD x, y: INTEGER; visible: BOOLEAN END;");

    auto* typeDecl = dynamic_cast<TypeDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(typeDecl, nullptr);
    EXPECT_EQ(typeDecl->name, "Point");

    auto* rec = dynamic_cast<RecordType*>(typeDecl->type.get());
    ASSERT_NE(rec, nullptr);
    ASSERT_EQ(rec->fields.size(), 2); // (x,y) и (visible)

    EXPECT_EQ(rec->fields[0]->names.size(), 2);
    EXPECT_EQ(rec->fields[0]->names[0], "x");

    EXPECT_EQ(rec->fields[1]->names[0], "visible");
}

TEST_F(BisonParserTypeTest, ParsesProcedureWithVarParams) {
    auto mod = parseDecls("PROCEDURE Swap(VAR a, b: INTEGER); BEGIN END Swap;");

    auto* proc = dynamic_cast<ProcDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(proc, nullptr);

    auto* sig = dynamic_cast<ProcType*>(proc->type.get());
    ASSERT_NE(sig, nullptr);
    ASSERT_EQ(sig->params.size(), 1); // Одна секция параметров

    auto* paramSection = sig->params[0].get();
    EXPECT_TRUE(paramSection->isVar) << "Failed to detect VAR parameter section";
    EXPECT_EQ(paramSection->names[0], "a");
    EXPECT_EQ(paramSection->names[1], "b");
}

} // namespace
