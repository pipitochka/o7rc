#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <sstream>

#include <parser/impl/bison/BisonParser.h>
#include <tokenizer/impl/flex/FlexTokenizer.h>
#include <util/ast/Ast.h>

class BisonParserStructureTest : public ::testing::Test {
protected:
    std::unique_ptr<Module> parse(const std::string& code) {
        std::stringstream ss(code);
        auto tokenizer = std::make_unique<FlexTokenizer>(ss);
        BisonParser parser;
        return parser.parse(std::move(tokenizer));
    }
};

TEST_F(BisonParserStructureTest, ParsesEmptyModule) {
    auto mod = parse("MODULE M; END M.");
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->name, "M");
    EXPECT_EQ(mod->endName, "M");
}

TEST_F(BisonParserStructureTest, ParsesImports) {
    auto mod = parse("MODULE M; IMPORT Out, Math := SomeMath; END M.");
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->imports.size(), 2);
    EXPECT_EQ(mod->imports[0].name, "Out");
    EXPECT_EQ(mod->imports[1].alias, "Math");
    EXPECT_EQ(mod->imports[1].name, "SomeMath");
}

TEST_F(BisonParserStructureTest, ParsesVarDeclarations) {
    auto mod = parse("MODULE M; VAR x, y: INTEGER; b*: BOOLEAN; END M.");
    ASSERT_NE(mod, nullptr);

    ASSERT_EQ(mod->decls.size(), 2);

    auto* varDecl1 = dynamic_cast<VarDecl*>(mod->decls[0].get());
    ASSERT_NE(varDecl1, nullptr);
    ASSERT_EQ(varDecl1->names.size(), 2);
    EXPECT_EQ(varDecl1->names[0], "x");
    EXPECT_EQ(varDecl1->names[1], "y");

    auto* varDecl2 = dynamic_cast<VarDecl*>(mod->decls[1].get());
    ASSERT_NE(varDecl2, nullptr);
    ASSERT_EQ(varDecl2->names.size(), 1);
    EXPECT_EQ(varDecl2->names[0], "b");
}
