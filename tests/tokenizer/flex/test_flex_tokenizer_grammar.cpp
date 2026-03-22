#include <gtest/gtest.h>
#include <sstream>

#include <tokenizer/impl/flex/FlexTokenizer.h>

/**
 * Вспомогательная функция: токенизирует строку code целиком
 * и возвращает вектор всех токенов (без завершающего Eof).
 */
std::vector<Token> tokenize(const std::string &code) {
    std::istringstream input(code);
    FlexTokenizer tz(input);
    std::vector<Token> tokens;
    while (true) {
        Token t = tz.next();
        if (t.type == TokenType::Eof)
            break;
        tokens.push_back(t);
    }
    return tokens;
}

/**
 * Проверяет распознавание основных ключевых слов Oberon-7
 * (MODULE, BEGIN, END, PROCEDURE, VAR, CONST, TYPE, ARRAY, RECORD, POINTER).
 */
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

/**
 * Проверяет распознавание ключевых слов управления потоком:
 * IF, THEN, ELSIF, ELSE, WHILE, DO, REPEAT, UNTIL, FOR, BY, TO, CASE, OF, RETURN.
 */
TEST(FlexTokenizerGrammarTest, ControlFlowKeywordsAreRecognized) {
    auto tokens = tokenize("IF THEN ELSIF ELSE WHILE DO REPEAT UNTIL FOR BY TO CASE OF RETURN");

    ASSERT_EQ(tokens.size(), 14);
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

/**
 * Проверяет распознавание ключевых слов-операторов и литералов:
 * DIV, MOD, OR, IN, IS, NIL, TRUE, FALSE, IMPORT.
 */
TEST(FlexTokenizerGrammarTest, OperatorAndLiteralKeywordsAreRecognized) {
    auto tokens = tokenize("DIV MOD OR IN IS NIL TRUE FALSE IMPORT");

    ASSERT_EQ(tokens.size(), 9);
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

/**
 * Проверяет регистрозависимость ключевых слов.
 * «MODULE» — ключевое слово, «module» и «Module» — идентификаторы.
 */
TEST(FlexTokenizerGrammarTest, KeywordsAreCaseSensitive) {
    auto tokens = tokenize("MODULE module Module");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::KW_MODULE);
    EXPECT_EQ(tokens[1].type, TokenType::Ident);
    EXPECT_EQ(tokens[2].type, TokenType::Ident);
}

/**
 * Проверяет разбор шестнадцатеричных целых чисел (суффикс H).
 * 0H=0, 0FFH=255, 10H=16, 0DEADBEEFH=3735928559.
 */
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

/**
 * Проверяет разбор вещественных чисел с экспонентой.
 * 3.1415 — без экспоненты, 1.0E-5 — с отрицательной, 2.5E2 = 250.0.
 */
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

/**
 * Проверяет, что допустимые идентификаторы Oberon-7 (буквы и цифры,
 * начинающиеся с буквы) распознаются корректно.
 */
TEST(FlexTokenizerGrammarTest, ValidIdentifiers) {
    auto tokens = tokenize("a a1 myVar23");

    ASSERT_EQ(tokens.size(), 3);
    for (const auto &t: tokens) {
        EXPECT_EQ(t.type, TokenType::Ident);
    }
    EXPECT_EQ(tokens[2].text, "myVar23");
}

/**
 * Проверяет, что идентификаторы с подчёркиванием распознаются как
 * Unknown + Ident (подчёркивание запрещено в Oberon-7).
 */
TEST(FlexTokenizerGrammarTest, UnderscoreNotAllowedInIdentifiers) {
    auto tokens = tokenize("_bad");

    ASSERT_GE(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::Unknown);
}

/**
 * Проверяет распознавание всей пунктуации и операторов Oberon-7:
 * :=  =  #  <  <=  >  >=  &  ~  |  ..
 */
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

/**
 * Проверяет распознавание скобок и разделителей:
 * ( ) [ ] { } , ; : .
 */
TEST(FlexTokenizerGrammarTest, ParsesBracketsAndDelimiters) {
    auto tokens = tokenize("( ) [ ] { } , ; : .");

    ASSERT_EQ(tokens.size(), 10);
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

/**
 * Проверяет распознавание строковых литералов в двойных кавычках,
 * включая пустую строку.
 */
TEST(FlexTokenizerGrammarTest, ParsesStrings) {
    auto tokens = tokenize("\"hello\" \"\" \"123\"");

    ASSERT_EQ(tokens.size(), 3);
    for (const auto &t: tokens) {
        EXPECT_EQ(t.type, TokenType::String);
    }
}

/**
 * Проверяет корректную обработку вложенных комментариев.
 * Вход: VAR (* comment (* nested *) *) x : INTEGER;
 * Ожидание: комментарий полностью пропускается, остаётся 5 токенов.
 */
TEST(FlexTokenizerGrammarTest, HandlesNestedComments) {
    auto tokens = tokenize("VAR (* comment (* nested *) *) x : INTEGER;");

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, TokenType::KW_VAR);
    EXPECT_EQ(tokens[1].type, TokenType::Ident);
    EXPECT_EQ(tokens[2].type, TokenType::Colon);
    EXPECT_EQ(tokens[3].type, TokenType::Ident);
    EXPECT_EQ(tokens[4].type, TokenType::Semicolon);
}

/**
 * Проверяет, что арифметические операторы + - * / ^ распознаются
 * как отдельные токены.
 */
TEST(FlexTokenizerGrammarTest, ParsesArithmeticOperators) {
    auto tokens = tokenize("+ - * / ^");

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, TokenType::Plus);
    EXPECT_EQ(tokens[1].type, TokenType::Minus);
    EXPECT_EQ(tokens[2].type, TokenType::Star);
    EXPECT_EQ(tokens[3].type, TokenType::Slash);
    EXPECT_EQ(tokens[4].type, TokenType::Caret);
}
