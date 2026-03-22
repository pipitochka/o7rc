#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include <tokenizer/impl/flex/FlexTokenizer.h>

/**
 * Проверяет, что peek() не потребляет токен — три последовательных
 * вызова peek() возвращают один и тот же токен KW_MODULE.
 * После этого next() также возвращает KW_MODULE, а следующий next() — Ident("Test").
 */
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

/**
 * Проверяет корректность чередования peek() и next().
 * Вход: VAR x: INTEGER;
 * peek() должен показывать текущий токен, а next() — продвигать позицию.
 */
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

/**
 * Проверяет обработку пустого входа — лексер должен сразу
 * возвращать Eof при peek() и next(), а повторный next() после Eof
 * также возвращает Eof (идемпотентность).
 */
TEST(FlexTokenizerTest, EofIsHandledCorrectly) {
    std::istringstream input("");
    FlexTokenizer tz(input);

    EXPECT_EQ(tz.peek().type, TokenType::Eof);
    EXPECT_EQ(tz.next().type, TokenType::Eof);

    EXPECT_EQ(tz.next().type, TokenType::Eof);
}

/**
 * Проверяет разбор литеральных значений: целое число, вещественное число, строка.
 * Вход: 12345   3.1415   "hello world"
 * Ожидание: intValue=12345, realValue=3.1415, text="hello world".
 */
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

/**
 * Проверяет правило «максимального захвата» (maximum munch) для операторов.
 * Вход: < <= : := . ..
 * Лексер должен различать Lt/Le, Colon/Assign, Dot/Range.
 */
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

/**
 * Проверяет, что пробельные символы и комментарии (* ... *) пропускаются.
 * Вход: "   \\n\\t  (* this is a comment *)  \\n  BEGIN"
 * Ожидание: единственный токен KW_BEGIN.
 */
TEST(FlexTokenizerTest, IgnoresWhitespaceAndComments) {
    std::istringstream input("   \n\t  (* this is a comment *)  \n  BEGIN");
    FlexTokenizer tz(input);

    Token t = tz.next();
    EXPECT_EQ(t.type, TokenType::KW_BEGIN);
}

/**
 * Проверяет отслеживание позиции (строка:столбец) токенов.
 * Вход: "MODULE\\n  Test;"
 * MODULE — строка 1, столбец 1; Test — строка 2, столбец 3.
 */
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

/**
 * Проверяет, что недопустимые символы ($ и %) распознаются как Unknown.
 * Вход: VAR $ x %
 * Ожидание: KW_VAR, Unknown("$"), Ident("x"), Unknown("%").
 */
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
