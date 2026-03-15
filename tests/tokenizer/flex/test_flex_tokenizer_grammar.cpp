#include <gtest/gtest.h>
#include <sstream>

#include <tokenizer/impl/flex/FlexTokenizer.h>

std::vector<Token> tokenize(const std::string& code) {
    std::istringstream input(code);
    FlexTokenizer tz(input);
    std::vector<Token> tokens;
    while (true) {
        Token t = tz.next();
        if (t.type == TokenType::Eof) break;
        tokens.push_back(t);
    }
    return tokens;
}


TEST(FlexTokenizerGrammarTest, AllKeywordsAreRecognized) {
    auto tokens = tokenize("MODULE BEGIN END PROCEDURE VAR CONST TYPE ARRAY RECORD POINTER");
    
    ASSERT_EQ(tokens.size(), 10);
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

TEST(FlexTokenizerGrammarTest, KeywordsAreCaseSensitive) {
    auto tokens = tokenize("MODULE module Module");
    
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::KW_MODULE);
    EXPECT_EQ(tokens[1].type, TokenType::Ident);
    EXPECT_EQ(tokens[2].type, TokenType::Ident);
}

TEST(FlexTokenizerGrammarTest, ParsesHexIntegers) {
    auto tokens = tokenize("0H 0FFH 10H 0DEADBEEFH");
    
    ASSERT_EQ(tokens.size(), 4);
    
    EXPECT_EQ(tokens[0].type, TokenType::Integer);
    EXPECT_EQ(tokens[0].intValue, 0x0);
    
    EXPECT_EQ(tokens[1].type, TokenType::Integer);
    EXPECT_EQ(tokens[1].intValue, 0xFF);
    
    EXPECT_EQ(tokens[2].type, TokenType::Integer);
    EXPECT_EQ(tokens[2].intValue, 0x10);
    
    EXPECT_EQ(tokens[3].type, TokenType::Integer);
    EXPECT_EQ(tokens[3].intValue, 0xDEADBEEF);
}

TEST(FlexTokenizerGrammarTest, ParsesRealNumbersWithExponents) {
    auto tokens = tokenize("3.1415 1.0E-5 2.5E2");
    
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::Real);
    EXPECT_DOUBLE_EQ(tokens[0].realValue, 3.1415);
    
    EXPECT_EQ(tokens[1].type, TokenType::Real);
    EXPECT_DOUBLE_EQ(tokens[1].realValue, 1.0e-5);
    
    EXPECT_EQ(tokens[2].type, TokenType::Real);
    EXPECT_DOUBLE_EQ(tokens[2].realValue, 250.0);
}


TEST(FlexTokenizerGrammarTest, ValidIdentifiers) {
    auto tokens = tokenize("a a1 myVar23");
    
    ASSERT_EQ(tokens.size(), 3);
    for(const auto& t : tokens) {
        EXPECT_EQ(t.type, TokenType::Ident);
    }
    EXPECT_EQ(tokens[2].text, "myVar23");
}

TEST(FlexTokenizerGrammarTest, ParsesOberonPunctuation) {
    auto tokens = tokenize(":= = # < <= > >= & ~ | ..");
    
    ASSERT_EQ(tokens.size(), 11);
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

TEST(FlexTokenizerGrammarTest, ParsesStrings) {
    auto tokens = tokenize("\"hello\" \"\" \"123\"");
    
    ASSERT_EQ(tokens.size(), 3);
    for(const auto& t : tokens) {
        EXPECT_EQ(t.type, TokenType::String);
    }
}


TEST(FlexTokenizerGrammarTest, HandlesNestedComments) {
    auto tokens = tokenize("VAR (* comment (* nested *) *) x : INTEGER;");
    
    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, TokenType::KW_VAR);
    EXPECT_EQ(tokens[1].type, TokenType::Ident);
    EXPECT_EQ(tokens[2].type, TokenType::Colon);
    EXPECT_EQ(tokens[3].type, TokenType::Ident);
    EXPECT_EQ(tokens[4].type, TokenType::Semicolon);
}
