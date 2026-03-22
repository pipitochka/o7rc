#include <gtest/gtest.h>
#include <sstream>

#include <parser/impl/bison/BisonParser.h>
#include <tokenizer/impl/flex/FlexTokenizer.h>
#include <util/ast/Ast.h>

namespace {

    struct ParserSnippetTestCase {
        std::string name;
        std::string code;
        bool shouldPass;
    };

    class BisonParserSnippetTest : public ::testing::TestWithParam<ParserSnippetTestCase> {};

}

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
            true}
    )
);
