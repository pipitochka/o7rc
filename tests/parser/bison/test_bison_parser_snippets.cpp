#include <gtest/gtest.h>
#include <sstream>

#include <parser/impl/bison/BisonParser.h>
#include <tokenizer/impl/flex/FlexTokenizer.h>
#include <util/ast/Ast.h>

namespace {

    /**
     * Параметр для параметризованного теста: содержит имя теста,
     * исходный код Oberon-7 и флаг, должен ли разбор завершиться успешно.
     */
    struct ParserSnippetTestCase {
        std::string name;
        std::string code;
        bool shouldPass;
    };

    /**
     * Параметризованный набор тестов: проверяет, что парсер Bison
     * корректно принимает или отклоняет полные модули Oberon-7.
     */
    class BisonParserSnippetTest : public ::testing::TestWithParam<ParserSnippetTestCase> {};

}

/**
 * Запускает парсер на полном исходном коде из параметра.
 * Если shouldPass=true  — ожидается успешный разбор (mod != nullptr).
 * Если shouldPass=false — ожидается ошибка (mod == nullptr или исключение).
 */
TEST_P(BisonParserSnippetTest, RunsSnippet) {
    const auto& param = GetParam();
    std::stringstream ss(param.code);
    auto tokenizer = std::make_unique<FlexTokenizer>(ss);
    BisonParser parser;

    bool failed = false;
    try {
        auto mod = parser.parse(std::move(tokenizer));
        if (!mod) failed = true;
    } catch (...) {
        failed = true;
    }

    if (param.shouldPass) {
        EXPECT_FALSE(failed) << "Valid code failed to parse: " << param.name;
    } else {
        EXPECT_TRUE(failed) << "Invalid code parsed successfully: " << param.name;
    }
}

/**
 * Набор реальных программ Oberon-7 для интеграционного тестирования парсера.
 *
 * Factorial  — рекурсивная функция факториала с RETURN внутри IF.
 * QuickSort  — процедура быстрой сортировки с массивом, REPEAT-UNTIL,
 *              вложенными WHILE и IF.
 * LinkedList — модуль со структурами данных: POINTER TO, RECORD, NEW.
 */
INSTANTIATE_TEST_SUITE_P(
    FullOberonPrograms,
    BisonParserSnippetTest,
    ::testing::Values(
        ParserSnippetTestCase{"Factorial",
            "MODULE Factorial; PROCEDURE Fact(n: INTEGER): INTEGER; BEGIN IF n = 0 THEN RETURN 1 ELSE RETURN n * Fact(n-1) END END Fact; END Factorial.",
            true},

        ParserSnippetTestCase{"QuickSort",
            R"(MODULE QuickSort;
               VAR a: ARRAY 10 OF INTEGER;
               PROCEDURE Sort(l, r: INTEGER);
                 VAR i, j, x, w: INTEGER;
               BEGIN i:=l; j:=r; x:=a[(l+r) DIV 2];
                 REPEAT
                   WHILE a[i] < x DO INC(i) END;
                   WHILE x < a[j] DO DEC(j) END;
                   IF i <= j THEN w:=a[i]; a[i]:=a[j]; a[j]:=w; INC(i); DEC(j) END
                 UNTIL i > j;
                 IF l < j THEN Sort(l, j) END;
                 IF i < r THEN Sort(i, r) END
               END Sort;
               END QuickSort.)",
            true},

        ParserSnippetTestCase{"LinkedList",
            R"(MODULE LinkedList;
               TYPE
                 NodePtr = POINTER TO Node;
                 Node = RECORD
                   value: INTEGER;
                   next: NodePtr
                 END;
               VAR head: NodePtr;
               PROCEDURE Insert(val: INTEGER);
                 VAR n: NodePtr;
               BEGIN
                 NEW(n);
                 n.value := val;
                 n.next := head;
                 head := n
               END Insert;
               END LinkedList.)",
            true}
    )
);

/**
 * Набор синтаксически некорректных программ для проверки,
 * что парсер корректно отклоняет невалидный код.
 *
 * MissingSemicolon     — пропущена ; после MODULE M.
 * MissingEnd           — нет закрывающего END M.
 * MissingDot           — нет точки в конце модуля.
 * InvalidExpression    — оператор без правого операнда (x := +).
 */
INSTANTIATE_TEST_SUITE_P(
    InvalidPrograms,
    BisonParserSnippetTest,
    ::testing::Values(
        ParserSnippetTestCase{"MissingSemicolon",
            "MODULE M END M.",
            false},

        ParserSnippetTestCase{"MissingEnd",
            "MODULE M; VAR x: INTEGER;",
            false},

        ParserSnippetTestCase{"MissingDot",
            "MODULE M; END M",
            false},

        ParserSnippetTestCase{"InvalidExpression",
            "MODULE M; VAR x: INTEGER; BEGIN x := + END M.",
            false}
    )
);
