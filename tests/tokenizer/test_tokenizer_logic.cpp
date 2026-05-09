#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include "test_factory.h"

class TokenizerLogicTest : public ::testing::TestWithParam<TokenizerKind> {
protected:
    ITokenizerPtr make(const std::string& code) {
        ss_ = std::make_unique<std::istringstream>(code);
        return makeTokenizer(GetParam(), *ss_);
    }
    std::unique_ptr<std::istringstream> ss_;
};

TEST_P(TokenizerLogicTest, PeekDoesNotConsumeToken) {
    auto tz = make("MODULE Test;");
    EXPECT_EQ(tz->peek().type, TokenType::KW_MODULE);
    EXPECT_EQ(tz->peek().type, TokenType::KW_MODULE);
    EXPECT_EQ(tz->peek().type, TokenType::KW_MODULE);
    EXPECT_EQ(tz->next().type, TokenType::KW_MODULE);
    Token t = tz->next();
    EXPECT_EQ(t.type, TokenType::Ident);
    EXPECT_EQ(t.text, "Test");
}

TEST_P(TokenizerLogicTest, AlternatingPeekAndNext) {
    auto tz = make("VAR x: INTEGER;");
    EXPECT_EQ(tz->peek().type, TokenType::KW_VAR);
    EXPECT_EQ(tz->next().type, TokenType::KW_VAR);
    EXPECT_EQ(tz->next().type, TokenType::Ident);
    EXPECT_EQ(tz->peek().type, TokenType::Colon);
    EXPECT_EQ(tz->peek().type, TokenType::Colon);
    EXPECT_EQ(tz->next().type, TokenType::Colon);
}

TEST_P(TokenizerLogicTest, EofIsHandledCorrectly) {
    auto tz = make("");
    EXPECT_EQ(tz->peek().type, TokenType::Eof);
    EXPECT_EQ(tz->next().type, TokenType::Eof);
    EXPECT_EQ(tz->next().type, TokenType::Eof);
}

TEST_P(TokenizerLogicTest, LiteralValuesParsing) {
    auto tz = make("12345   3.1415   \"hello world\"");
    Token tInt = tz->next();
    EXPECT_EQ(tInt.type, TokenType::Integer);
    EXPECT_EQ(tInt.intValue, 12345);

    Token tReal = tz->next();
    EXPECT_EQ(tReal.type, TokenType::Real);
    EXPECT_DOUBLE_EQ(tReal.realValue, 3.1415);

    Token tStr = tz->next();
    EXPECT_EQ(tStr.type, TokenType::String);
    EXPECT_EQ(tStr.text, "hello world");
}

TEST_P(TokenizerLogicTest, OperatorsAndMaximumMunch) {
    auto tz = make("< <= : := . ..");
    EXPECT_EQ(tz->next().type, TokenType::Lt);
    EXPECT_EQ(tz->next().type, TokenType::Le);
    EXPECT_EQ(tz->next().type, TokenType::Colon);
    EXPECT_EQ(tz->next().type, TokenType::Assign);
    EXPECT_EQ(tz->next().type, TokenType::Dot);
    EXPECT_EQ(tz->next().type, TokenType::Range);
}

TEST_P(TokenizerLogicTest, IgnoresWhitespaceAndComments) {
    auto tz = make("   \n\t  (* this is a comment *)  \n  BEGIN");
    EXPECT_EQ(tz->next().type, TokenType::KW_BEGIN);
}

TEST_P(TokenizerLogicTest, TracksLineAndColumn) {
    auto tz = make("MODULE\n  Test;");
    Token t1 = tz->next();
    EXPECT_EQ(t1.line, 1u);
    EXPECT_EQ(t1.col, 1u);
    Token t2 = tz->next();
    EXPECT_EQ(t2.line, 2u);
    EXPECT_EQ(t2.col, 3u);
}

TEST_P(TokenizerLogicTest, CapturesUnknownCharacters) {
    auto tz = make("VAR $ x %");
    EXPECT_EQ(tz->next().type, TokenType::KW_VAR);
    Token err1 = tz->next();
    EXPECT_EQ(err1.type, TokenType::Unknown);
    EXPECT_EQ(err1.text, "$");
    EXPECT_EQ(tz->next().type, TokenType::Ident);
    Token err2 = tz->next();
    EXPECT_EQ(err2.type, TokenType::Unknown);
    EXPECT_EQ(err2.text, "%");
}

TEST_P(TokenizerLogicTest, ParsesDoublePrecisionLiteral) {
    auto tz = make("1.2345678901234567");
    Token t = tz->next();
    EXPECT_EQ(t.type, TokenType::Real);
    EXPECT_DOUBLE_EQ(t.realValue, 1.2345678901234567);
}

TEST_P(TokenizerLogicTest, ParsesDoubleLiteralLargeIntegerMantissa) {
    // Целое > 2^53 не представимо точно в IEEE double, но значение задаётся через литерал double.
    auto tz = make("9007199254740993.0");
    Token t = tz->next();
    EXPECT_EQ(t.type, TokenType::Real);
    EXPECT_DOUBLE_EQ(t.realValue, 9007199254740993.0);
}

TEST_P(TokenizerLogicTest, ParsesScientificNotationLiterals) {
    // Oberon real требует точку в мантиссе; литерал вида 2E+5 без точки даёт Integer + Ident.
    auto tz = make("6.02214076E23  1.5e-10  2.0E+5");
    Token a = tz->next();
    EXPECT_EQ(a.type, TokenType::Real);
    EXPECT_DOUBLE_EQ(a.realValue, 6.02214076e23);

    Token b = tz->next();
    EXPECT_EQ(b.type, TokenType::Real);
    EXPECT_DOUBLE_EQ(b.realValue, 1.5e-10);

    Token c = tz->next();
    EXPECT_EQ(c.type, TokenType::Real);
    EXPECT_DOUBLE_EQ(c.realValue, 2e5);
}

INSTANTIATE_TEST_SUITE_P(
    AllTokenizers, TokenizerLogicTest,
    ::testing::ValuesIn(availableTokenizers()),
    [](const auto& info) { return tokenizerName(info.param); }
);
