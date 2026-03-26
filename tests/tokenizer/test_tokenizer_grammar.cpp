#include <gtest/gtest.h>
#include <sstream>
#include <vector>

#include "test_factory.h"

class TokenizerGrammarTest : public ::testing::TestWithParam<TokenizerKind> {
protected:
    std::vector<Token> tokenize(const std::string& code) {
        std::istringstream input(code);
        auto tz = makeTokenizer(GetParam(), input);
        std::vector<Token> tokens;
        while (true) {
            Token t = tz->next();
            if (t.type == TokenType::Eof) break;
            tokens.push_back(t);
        }
        return tokens;
    }
};

TEST_P(TokenizerGrammarTest, AllKeywordsAreRecognized) {
    auto tokens = tokenize("MODULE BEGIN END PROCEDURE VAR CONST TYPE ARRAY RECORD POINTER");
    ASSERT_EQ(tokens.size(), 10u);
    EXPECT_EQ(tokens[0].type, TokenType::KW_MODULE);
    EXPECT_EQ(tokens[1].type, TokenType::KW_BEGIN);
    EXPECT_EQ(tokens[2].type, TokenType::KW_END);
    EXPECT_EQ(tokens[3].type, TokenType::KW_PROCEDURE);
    EXPECT_EQ(tokens[4].type, TokenType::KW_VAR);
    EXPECT_EQ(tokens[5].type, TokenType::KW_CONST);
    EXPECT_EQ(tokens[6].type, TokenType::KW_TYPE);
    EXPECT_EQ(tokens[7].type, TokenType::KW_ARRAY);
    EXPECT_EQ(tokens[8].type, TokenType::KW_RECORD);
    EXPECT_EQ(tokens[9].type, TokenType::KW_POINTER);
}

TEST_P(TokenizerGrammarTest, ControlFlowKeywordsAreRecognized) {
    auto tokens = tokenize("IF THEN ELSIF ELSE WHILE DO REPEAT UNTIL FOR BY TO CASE OF RETURN");
    ASSERT_EQ(tokens.size(), 14u);
    EXPECT_EQ(tokens[0].type, TokenType::KW_IF);
    EXPECT_EQ(tokens[1].type, TokenType::KW_THEN);
    EXPECT_EQ(tokens[2].type, TokenType::KW_ELSIF);
    EXPECT_EQ(tokens[3].type, TokenType::KW_ELSE);
    EXPECT_EQ(tokens[4].type, TokenType::KW_WHILE);
    EXPECT_EQ(tokens[5].type, TokenType::KW_DO);
    EXPECT_EQ(tokens[6].type, TokenType::KW_REPEAT);
    EXPECT_EQ(tokens[7].type, TokenType::KW_UNTIL);
    EXPECT_EQ(tokens[8].type, TokenType::KW_FOR);
    EXPECT_EQ(tokens[9].type, TokenType::KW_BY);
    EXPECT_EQ(tokens[10].type, TokenType::KW_TO);
    EXPECT_EQ(tokens[11].type, TokenType::KW_CASE);
    EXPECT_EQ(tokens[12].type, TokenType::KW_OF);
    EXPECT_EQ(tokens[13].type, TokenType::KW_RETURN);
}

TEST_P(TokenizerGrammarTest, OperatorAndLiteralKeywordsAreRecognized) {
    auto tokens = tokenize("DIV MOD OR IN IS NIL TRUE FALSE IMPORT");
    ASSERT_EQ(tokens.size(), 9u);
    EXPECT_EQ(tokens[0].type, TokenType::KW_DIV);
    EXPECT_EQ(tokens[1].type, TokenType::KW_MOD);
    EXPECT_EQ(tokens[2].type, TokenType::KW_OR);
    EXPECT_EQ(tokens[3].type, TokenType::KW_IN);
    EXPECT_EQ(tokens[4].type, TokenType::KW_IS);
    EXPECT_EQ(tokens[5].type, TokenType::KW_NIL);
    EXPECT_EQ(tokens[6].type, TokenType::KW_TRUE);
    EXPECT_EQ(tokens[7].type, TokenType::KW_FALSE);
    EXPECT_EQ(tokens[8].type, TokenType::KW_IMPORT);
}

TEST_P(TokenizerGrammarTest, KeywordsAreCaseSensitive) {
    auto tokens = tokenize("MODULE module Module");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::KW_MODULE);
    EXPECT_EQ(tokens[1].type, TokenType::Ident);
    EXPECT_EQ(tokens[2].type, TokenType::Ident);
}

TEST_P(TokenizerGrammarTest, ParsesHexIntegers) {
    auto tokens = tokenize("0H 0FFH 10H 0DEADBEEFH");
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0].type, TokenType::Integer);
    EXPECT_EQ(tokens[0].intValue, 0x0);
    EXPECT_EQ(tokens[1].type, TokenType::Integer);
    EXPECT_EQ(tokens[1].intValue, 0xFF);
    EXPECT_EQ(tokens[2].type, TokenType::Integer);
    EXPECT_EQ(tokens[2].intValue, 0x10);
    EXPECT_EQ(tokens[3].type, TokenType::Integer);
    EXPECT_EQ(tokens[3].intValue, 0xDEADBEEF);
}

TEST_P(TokenizerGrammarTest, ParsesRealNumbersWithExponents) {
    auto tokens = tokenize("3.1415 1.0E-5 2.5E2");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::Real);
    EXPECT_DOUBLE_EQ(tokens[0].realValue, 3.1415);
    EXPECT_EQ(tokens[1].type, TokenType::Real);
    EXPECT_DOUBLE_EQ(tokens[1].realValue, 1.0e-5);
    EXPECT_EQ(tokens[2].type, TokenType::Real);
    EXPECT_DOUBLE_EQ(tokens[2].realValue, 250.0);
}

TEST_P(TokenizerGrammarTest, ValidIdentifiers) {
    auto tokens = tokenize("a a1 myVar23");
    ASSERT_EQ(tokens.size(), 3u);
    for (const auto& t : tokens) { EXPECT_EQ(t.type, TokenType::Ident); }
    EXPECT_EQ(tokens[2].text, "myVar23");
}

TEST_P(TokenizerGrammarTest, UnderscoreNotAllowedInIdentifiers) {
    auto tokens = tokenize("_bad");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::Unknown);
}

TEST_P(TokenizerGrammarTest, ParsesOberonPunctuation) {
    auto tokens = tokenize(":= = # < <= > >= & ~ | ..");
    ASSERT_EQ(tokens.size(), 11u);
    EXPECT_EQ(tokens[0].type, TokenType::Assign);
    EXPECT_EQ(tokens[1].type, TokenType::Eq);
    EXPECT_EQ(tokens[2].type, TokenType::Neq);
    EXPECT_EQ(tokens[3].type, TokenType::Lt);
    EXPECT_EQ(tokens[4].type, TokenType::Le);
    EXPECT_EQ(tokens[5].type, TokenType::Gt);
    EXPECT_EQ(tokens[6].type, TokenType::Ge);
    EXPECT_EQ(tokens[7].type, TokenType::Amp);
    EXPECT_EQ(tokens[8].type, TokenType::Tilde);
    EXPECT_EQ(tokens[9].type, TokenType::Bar);
    EXPECT_EQ(tokens[10].type, TokenType::Range);
}

TEST_P(TokenizerGrammarTest, ParsesBracketsAndDelimiters) {
    auto tokens = tokenize("( ) [ ] { } , ; : .");
    ASSERT_EQ(tokens.size(), 10u);
    EXPECT_EQ(tokens[0].type, TokenType::LParen);
    EXPECT_EQ(tokens[1].type, TokenType::RParen);
    EXPECT_EQ(tokens[2].type, TokenType::LBrack);
    EXPECT_EQ(tokens[3].type, TokenType::RBrack);
    EXPECT_EQ(tokens[4].type, TokenType::LBrace);
    EXPECT_EQ(tokens[5].type, TokenType::RBrace);
    EXPECT_EQ(tokens[6].type, TokenType::Comma);
    EXPECT_EQ(tokens[7].type, TokenType::Semicolon);
    EXPECT_EQ(tokens[8].type, TokenType::Colon);
    EXPECT_EQ(tokens[9].type, TokenType::Dot);
}

TEST_P(TokenizerGrammarTest, ParsesStrings) {
    auto tokens = tokenize("\"hello\" \"\" \"123\"");
    ASSERT_EQ(tokens.size(), 3u);
    for (const auto& t : tokens) { EXPECT_EQ(t.type, TokenType::String); }
}

TEST_P(TokenizerGrammarTest, HandlesNestedComments) {
    auto tokens = tokenize("VAR (* comment (* nested *) *) x : INTEGER;");
    ASSERT_EQ(tokens.size(), 5u);
    EXPECT_EQ(tokens[0].type, TokenType::KW_VAR);
    EXPECT_EQ(tokens[1].type, TokenType::Ident);
    EXPECT_EQ(tokens[2].type, TokenType::Colon);
    EXPECT_EQ(tokens[3].type, TokenType::Ident);
    EXPECT_EQ(tokens[4].type, TokenType::Semicolon);
}

TEST_P(TokenizerGrammarTest, ParsesArithmeticOperators) {
    auto tokens = tokenize("+ - * / ^");
    ASSERT_EQ(tokens.size(), 5u);
    EXPECT_EQ(tokens[0].type, TokenType::Plus);
    EXPECT_EQ(tokens[1].type, TokenType::Minus);
    EXPECT_EQ(tokens[2].type, TokenType::Star);
    EXPECT_EQ(tokens[3].type, TokenType::Slash);
    EXPECT_EQ(tokens[4].type, TokenType::Caret);
}

INSTANTIATE_TEST_SUITE_P(
    AllTokenizers, TokenizerGrammarTest,
    ::testing::ValuesIn(availableTokenizers()),
    [](const auto& info) { return tokenizerName(info.param); }
);
