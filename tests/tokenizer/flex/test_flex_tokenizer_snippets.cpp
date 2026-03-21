#include <gtest/gtest.h>
#include <sstream>
#include <vector>

#include <tokenizer/impl/flex/FlexTokenizer.h>

struct SnippetTestCase {
    std::string name;
    std::string code;
    std::vector<TokenType> expected;
};

class FlexTokenizerSnippetsTest : public ::testing::TestWithParam<SnippetTestCase> {};

TEST_P(FlexTokenizerSnippetsTest, TokenizesCorrectly) {
    const auto &param = GetParam();
    std::istringstream input(param.code);
    FlexTokenizer tz(input);

    for (size_t i = 0; i < param.expected.size(); ++i) {
        Token t = tz.next();

        ASSERT_NE(t.type, TokenType::Eof) << "Premature EOF at index " << i << " in test case: " << param.name;

        EXPECT_EQ(t.type, param.expected[i])
                << "Mismatch at index " << i << " (Got text: '" << t.text << "') in test case: " << param.name;
    }

    EXPECT_EQ(tz.next().type, TokenType::Eof) << "Expected EOF at the end of test case: " << param.name;
}

INSTANTIATE_TEST_SUITE_P(
        OberonSnippets, FlexTokenizerSnippetsTest,
        ::testing::Values(


                SnippetTestCase{"TightMath",
                                "x:=a+b*c-10/2;",
                                {TokenType::Ident, TokenType::Assign, TokenType::Ident, TokenType::Plus,
                                 TokenType::Ident, TokenType::Star, TokenType::Ident, TokenType::Minus,
                                 TokenType::Integer, TokenType::Slash, TokenType::Integer, TokenType::Semicolon}},

                SnippetTestCase{"LogicalCondition",
                                "IF(a#b)&~c OR(d<=e)THEN",
                                {TokenType::KW_IF, TokenType::LParen, TokenType::Ident, TokenType::Neq,
                                 TokenType::Ident, TokenType::RParen, TokenType::Amp, TokenType::Tilde,
                                 TokenType::Ident, TokenType::KW_OR, TokenType::LParen, TokenType::Ident, TokenType::Le,
                                 TokenType::Ident, TokenType::RParen, TokenType::KW_THEN}},


                SnippetTestCase{"Selectors",
                                "arr[i+1].field^:=NIL",
                                {TokenType::Ident, TokenType::LBrack, TokenType::Ident, TokenType::Plus,
                                 TokenType::Integer, TokenType::RBrack, TokenType::Dot, TokenType::Ident,
                                 TokenType::Caret, TokenType::Assign, TokenType::KW_NIL}},


                SnippetTestCase{"SetsAndRanges",
                                "s:={0, 5..10};",
                                {TokenType::Ident, TokenType::Assign, TokenType::LBrace, TokenType::Integer,
                                 TokenType::Comma, TokenType::Integer, TokenType::Range, TokenType::Integer,
                                 TokenType::RBrace, TokenType::Semicolon}},


                SnippetTestCase{"ProcedureSignature",
                                "PROCEDURE Math*(x:REAL;VAR y:REAL);",
                                {TokenType::KW_PROCEDURE, TokenType::Ident, TokenType::Star, TokenType::LParen,
                                 TokenType::Ident, TokenType::Colon, TokenType::Ident, TokenType::Semicolon,
                                 TokenType::KW_VAR, TokenType::Ident, TokenType::Colon, TokenType::Ident,
                                 TokenType::RParen, TokenType::Semicolon}},

                SnippetTestCase{"VarDeclarations",
                                "VAR x,y:INTEGER; p:POINTER TO Node;",
                                {TokenType::KW_VAR, TokenType::Ident, TokenType::Comma, TokenType::Ident,
                                 TokenType::Colon, TokenType::Ident, TokenType::Semicolon, TokenType::Ident,
                                 TokenType::Colon, TokenType::KW_POINTER, TokenType::KW_TO, TokenType::Ident,
                                 TokenType::Semicolon}},

                SnippetTestCase{
                        "ComplexAlgorithms",
                        R"(
                PROCEDURE ReadInt(VAR x: INTEGER);
                    VAR i : INTEGER; ch: CHAR;
                BEGIN i := 0; Read(ch);
                    WHILE ("0" <= ch) & (ch <= "9") DO
                        i := 10*i + (ORD(ch)-ORD("0")); Read(ch)
                    END ;
                    x := i
                END ReadInt

                PROCEDURE WriteInt(x: INTEGER); (* 0 <= x < 10^5 *)
                    VAR i: INTEGER;
                        buf: ARRAY 5 OF INTEGER;
                BEGIN i := 0;
                    REPEAT buf[i] := x MOD 10; x := x DIV 10; INC(i) UNTIL x = 0;
                    REPEAT DEC(i); Write(CHR(buf[i] + ORD("0"))) UNTIL i = 0
                END WriteInt

                PROCEDURE log2(x: INTEGER): INTEGER;
                    VAR y: INTEGER; (*assume x>0*)
                BEGIN y := 0;
                    WHILE x > 1 DO x := x DIV 2; INC(y) END ;
                    RETURN y
                END log2
            )",
                        {TokenType::KW_PROCEDURE, TokenType::Ident,     TokenType::LParen,    TokenType::KW_VAR,
                         TokenType::Ident,        TokenType::Colon,     TokenType::Ident,     TokenType::RParen,
                         TokenType::Semicolon,    TokenType::KW_VAR,    TokenType::Ident,     TokenType::Colon,
                         TokenType::Ident,        TokenType::Semicolon, TokenType::Ident,     TokenType::Colon,
                         TokenType::Ident,        TokenType::Semicolon, TokenType::KW_BEGIN,  TokenType::Ident,
                         TokenType::Assign,       TokenType::Integer,   TokenType::Semicolon, TokenType::Ident,
                         TokenType::LParen,       TokenType::Ident,     TokenType::RParen,    TokenType::Semicolon,
                         TokenType::KW_WHILE,     TokenType::LParen,    TokenType::String,    TokenType::Le,
                         TokenType::Ident,        TokenType::RParen,    TokenType::Amp,       TokenType::LParen,
                         TokenType::Ident,        TokenType::Le,        TokenType::String,    TokenType::RParen,
                         TokenType::KW_DO,        TokenType::Ident,     TokenType::Assign,    TokenType::Integer,
                         TokenType::Star,         TokenType::Ident,     TokenType::Plus,      TokenType::LParen,
                         TokenType::Ident,        TokenType::LParen,    TokenType::Ident,     TokenType::RParen,
                         TokenType::Minus,        TokenType::Ident,     TokenType::LParen,    TokenType::String,
                         TokenType::RParen,       TokenType::RParen,    TokenType::Semicolon, TokenType::Ident,
                         TokenType::LParen,       TokenType::Ident,     TokenType::RParen,    TokenType::KW_END,
                         TokenType::Semicolon,    TokenType::Ident,     TokenType::Assign,    TokenType::Ident,
                         TokenType::KW_END,       TokenType::Ident,

                         TokenType::KW_PROCEDURE, TokenType::Ident,     TokenType::LParen,    TokenType::Ident,
                         TokenType::Colon,        TokenType::Ident,     TokenType::RParen,    TokenType::Semicolon,
                         TokenType::KW_VAR,       TokenType::Ident,     TokenType::Colon,     TokenType::Ident,
                         TokenType::Semicolon,    TokenType::Ident,     TokenType::Colon,     TokenType::KW_ARRAY,
                         TokenType::Integer,      TokenType::KW_OF,     TokenType::Ident,     TokenType::Semicolon,
                         TokenType::KW_BEGIN,     TokenType::Ident,     TokenType::Assign,    TokenType::Integer,
                         TokenType::Semicolon,    TokenType::KW_REPEAT, TokenType::Ident,     TokenType::LBrack,
                         TokenType::Ident,        TokenType::RBrack,    TokenType::Assign,    TokenType::Ident,
                         TokenType::KW_MOD,       TokenType::Integer,   TokenType::Semicolon, TokenType::Ident,
                         TokenType::Assign,       TokenType::Ident,     TokenType::KW_DIV,    TokenType::Integer,
                         TokenType::Semicolon,    TokenType::Ident,     TokenType::LParen,    TokenType::Ident,
                         TokenType::RParen,       TokenType::KW_UNTIL,  TokenType::Ident,     TokenType::Eq,
                         TokenType::Integer,      TokenType::Semicolon, TokenType::KW_REPEAT, TokenType::Ident,
                         TokenType::LParen,       TokenType::Ident,     TokenType::RParen,    TokenType::Semicolon,
                         TokenType::Ident,        TokenType::LParen,    TokenType::Ident,     TokenType::LParen,
                         TokenType::Ident,        TokenType::LBrack,    TokenType::Ident,     TokenType::RBrack,
                         TokenType::Plus,         TokenType::Ident,     TokenType::LParen,    TokenType::String,
                         TokenType::RParen,       TokenType::RParen,    TokenType::RParen,    TokenType::KW_UNTIL,
                         TokenType::Ident,        TokenType::Eq,        TokenType::Integer,   TokenType::KW_END,
                         TokenType::Ident,

                         TokenType::KW_PROCEDURE, TokenType::Ident,     TokenType::LParen,    TokenType::Ident,
                         TokenType::Colon,        TokenType::Ident,     TokenType::RParen,    TokenType::Colon,
                         TokenType::Ident,        TokenType::Semicolon, TokenType::KW_VAR,    TokenType::Ident,
                         TokenType::Colon,        TokenType::Ident,     TokenType::Semicolon, TokenType::KW_BEGIN,
                         TokenType::Ident,        TokenType::Assign,    TokenType::Integer,   TokenType::Semicolon,
                         TokenType::KW_WHILE,     TokenType::Ident,     TokenType::Gt,        TokenType::Integer,
                         TokenType::KW_DO,        TokenType::Ident,     TokenType::Assign,    TokenType::Ident,
                         TokenType::KW_DIV,       TokenType::Integer,   TokenType::Semicolon, TokenType::Ident,
                         TokenType::LParen,       TokenType::Ident,     TokenType::RParen,    TokenType::KW_END,
                         TokenType::Semicolon,    TokenType::KW_RETURN, TokenType::Ident,     TokenType::KW_END,
                         TokenType::Ident}}

                ));
