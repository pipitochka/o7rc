#include <gtest/gtest.h>
#include <sstream>
#include <vector>

#include "test_factory.h"

struct SnippetTestParam {
    TokenizerKind tokKind;
    std::string name;
    std::string code;
    std::vector<TokenType> expected;
};

class TokenizerSnippetsTest : public ::testing::TestWithParam<SnippetTestParam> {};

TEST_P(TokenizerSnippetsTest, TokenizesCorrectly) {
    const auto& param = GetParam();
    std::istringstream input(param.code);
    auto tz = makeTokenizer(param.tokKind, input);

    for (size_t i = 0; i < param.expected.size(); ++i) {
        Token t = tz->next();
        ASSERT_NE(t.type, TokenType::Eof)
            << "Premature EOF at index " << i << " in test case: " << param.name;
        EXPECT_EQ(t.type, param.expected[i])
            << "Mismatch at index " << i << " (Got text: '" << t.text
            << "') in test case: " << param.name;
    }
    EXPECT_EQ(tz->next().type, TokenType::Eof)
        << "Expected EOF at the end of test case: " << param.name;
}

static std::vector<SnippetTestParam> generateSnippetParams() {
    struct Snippet {
        std::string name;
        std::string code;
        std::vector<TokenType> expected;
    };

    std::vector<Snippet> snippets = {
        {"TightMath",
         "x:=a+b*c-10/2;",
         {TokenType::Ident, TokenType::Assign, TokenType::Ident, TokenType::Plus,
          TokenType::Ident, TokenType::Star, TokenType::Ident, TokenType::Minus,
          TokenType::Integer, TokenType::Slash, TokenType::Integer, TokenType::Semicolon}},

        {"LogicalCondition",
         "IF(a#b)&~c OR(d<=e)THEN",
         {TokenType::KW_IF, TokenType::LParen, TokenType::Ident, TokenType::Neq,
          TokenType::Ident, TokenType::RParen, TokenType::Amp, TokenType::Tilde,
          TokenType::Ident, TokenType::KW_OR, TokenType::LParen, TokenType::Ident,
          TokenType::Le, TokenType::Ident, TokenType::RParen, TokenType::KW_THEN}},

        {"Selectors",
         "arr[i+1].field^:=NIL",
         {TokenType::Ident, TokenType::LBrack, TokenType::Ident, TokenType::Plus,
          TokenType::Integer, TokenType::RBrack, TokenType::Dot, TokenType::Ident,
          TokenType::Caret, TokenType::Assign, TokenType::KW_NIL}},

        {"SetsAndRanges",
         "s:={0, 5..10};",
         {TokenType::Ident, TokenType::Assign, TokenType::LBrace, TokenType::Integer,
          TokenType::Comma, TokenType::Integer, TokenType::Range, TokenType::Integer,
          TokenType::RBrace, TokenType::Semicolon}},

        {"ProcedureSignature",
         "PROCEDURE Math*(x:REAL;VAR y:REAL);",
         {TokenType::KW_PROCEDURE, TokenType::Ident, TokenType::Star, TokenType::LParen,
          TokenType::Ident, TokenType::Colon, TokenType::Ident, TokenType::Semicolon,
          TokenType::KW_VAR, TokenType::Ident, TokenType::Colon, TokenType::Ident,
          TokenType::RParen, TokenType::Semicolon}},

        {"VarDeclarations",
         "VAR x,y:INTEGER; p:POINTER TO Node;",
         {TokenType::KW_VAR, TokenType::Ident, TokenType::Comma, TokenType::Ident,
          TokenType::Colon, TokenType::Ident, TokenType::Semicolon, TokenType::Ident,
          TokenType::Colon, TokenType::KW_POINTER, TokenType::KW_TO, TokenType::Ident,
          TokenType::Semicolon}},
    };

    std::vector<SnippetTestParam> params;
    for (auto kind : availableTokenizers()) {
        for (auto& s : snippets) {
            params.push_back({kind, std::string(tokenizerName(kind)) + "_" + s.name,
                              s.code, s.expected});
        }
    }
    return params;
}

INSTANTIATE_TEST_SUITE_P(
    AllTokenizers, TokenizerSnippetsTest,
    ::testing::ValuesIn(generateSnippetParams()),
    [](const auto& info) { return info.param.name; }
);
