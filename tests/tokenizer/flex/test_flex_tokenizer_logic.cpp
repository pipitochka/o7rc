#include <gtest/gtest.h>
#include <sstream>
#include <memory>

#include <tokenizer/impl/flex/FlexTokenizer.h>

TEST(FlexTokenizerTest, PeekDoesNotConsumeToken) {
    std::istringstream input("MODULE Test;");
    FlexTokenizer tz(input);

    Token t1 = tz.peek();
    Token t2 = tz.peek();
    Token t3 = tz.peek();

    EXPECT_EQ(t1.type, TokenType::KW_MODULE);
    EXPECT_EQ(t2.type, TokenType::KW_MODULE);
    EXPECT_EQ(t3.type, TokenType::KW_MODULE);

    Token t4 = tz.next();
    EXPECT_EQ(t4.type, TokenType::KW_MODULE);

    Token t5 = tz.next();
    EXPECT_EQ(t5.type, TokenType::Ident);
    EXPECT_EQ(t5.text, "Test");
}

TEST(FlexTokenizerTest, AlternatingPeekAndNext) {
    std::istringstream input("VAR x: INTEGER;");
    FlexTokenizer tz(input);

    EXPECT_EQ(tz.peek().type, TokenType::KW_VAR);
    EXPECT_EQ(tz.next().type, TokenType::KW_VAR);
    
    EXPECT_EQ(tz.next().type, TokenType::Ident);
    
    EXPECT_EQ(tz.peek().type, TokenType::Colon);
    EXPECT_EQ(tz.peek().type, TokenType::Colon);
    EXPECT_EQ(tz.next().type, TokenType::Colon);
}

TEST(FlexTokenizerTest, EofIsHandledCorrectly) {
    std::istringstream input("");
    FlexTokenizer tz(input);

    EXPECT_EQ(tz.peek().type, TokenType::Eof);
    EXPECT_EQ(tz.next().type, TokenType::Eof);
    
    EXPECT_EQ(tz.next().type, TokenType::Eof);
}

TEST(FlexTokenizerTest, LiteralValuesParsing) {
    std::istringstream input("12345   3.1415   \"hello world\"");
    FlexTokenizer tz(input);

    Token tInt = tz.next();
    EXPECT_EQ(tInt.type, TokenType::Integer);
    EXPECT_EQ(tInt.intValue, 12345);

    Token tReal = tz.next();
    EXPECT_EQ(tReal.type, TokenType::Real);
    EXPECT_DOUBLE_EQ(tReal.realValue, 3.1415);

    Token tStr = tz.next();
    EXPECT_EQ(tStr.type, TokenType::String);
    EXPECT_EQ(tStr.text, "hello world"); 
}

TEST(FlexTokenizerTest, OperatorsAndMaximumMunch) {
    std::istringstream input("< <= : := . ..");
    FlexTokenizer tz(input);

    EXPECT_EQ(tz.next().type, TokenType::Lt);
    EXPECT_EQ(tz.next().type, TokenType::Le);
    EXPECT_EQ(tz.next().type, TokenType::Colon);
    EXPECT_EQ(tz.next().type, TokenType::Assign);
    EXPECT_EQ(tz.next().type, TokenType::Dot);
    EXPECT_EQ(tz.next().type, TokenType::Range);
}

TEST(FlexTokenizerTest, IgnoresWhitespaceAndComments) {
    std::istringstream input("   \n\t  (* this is a comment *)  \n  BEGIN");
    FlexTokenizer tz(input);

    Token t = tz.next();
    EXPECT_EQ(t.type, TokenType::KW_BEGIN);
}

TEST(FlexTokenizerTest, TracksLineAndColumn) {
    std::istringstream input("MODULE\n  Test;");
    FlexTokenizer tz(input);

    Token t1 = tz.next();
    EXPECT_EQ(t1.line, 1);
    EXPECT_EQ(t1.col, 1);

    Token t2 = tz.next();
    EXPECT_EQ(t2.line, 2);
    EXPECT_EQ(t2.col, 3);
}

TEST(FlexTokenizerTest, CapturesUnknownCharacters) {
    std::istringstream input("VAR $ x %");
    FlexTokenizer tz(input);

    EXPECT_EQ(tz.next().type, TokenType::KW_VAR);
    
    Token err1 = tz.next();
    EXPECT_EQ(err1.type, TokenType::Unknown);
    EXPECT_EQ(err1.text, "$");

    EXPECT_EQ(tz.next().type, TokenType::Ident);

    Token err2 = tz.next();
    EXPECT_EQ(err2.type, TokenType::Unknown);
    EXPECT_EQ(err2.text, "%");
}
