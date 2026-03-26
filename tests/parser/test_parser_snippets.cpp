#include <gtest/gtest.h>
#include <sstream>

#include "test_factory.h"

struct FrontendConfig {
    TokenizerKind tok;
    ParserKind par;
};

struct ParserSnippetParam {
    FrontendConfig cfg;
    std::string name;
    std::string code;
    bool shouldPass;
};

class ParserSnippetTest : public ::testing::TestWithParam<ParserSnippetParam> {};

TEST_P(ParserSnippetTest, RunsSnippet) {
    const auto& p = GetParam();
    auto ss = std::make_unique<std::stringstream>(p.code);
    auto tokenizer = makeTokenizer(p.cfg.tok, *ss);
    auto parser = makeParser(p.cfg.par);

    bool failed = false;
    try {
        auto mod = parser->parse(std::move(tokenizer));
        if (!mod) failed = true;
    } catch (...) {
        failed = true;
    }

    if (p.shouldPass) {
        EXPECT_FALSE(failed) << "Valid code failed to parse: " << p.name;
    } else {
        EXPECT_TRUE(failed) << "Invalid code parsed successfully: " << p.name;
    }
}

static std::vector<FrontendConfig> allConfigs() {
    std::vector<FrontendConfig> v;
    for (auto t : availableTokenizers())
        for (auto p : availableParsers())
            v.push_back({t, p});
    return v;
}

static std::vector<ParserSnippetParam> generateParams() {
    struct Snippet {
        std::string name;
        std::string code;
        bool shouldPass;
    };

    std::vector<Snippet> valid = {
        {"Factorial",
         "MODULE Factorial; PROCEDURE Fact(n: INTEGER): INTEGER; BEGIN "
         "IF n = 0 THEN RETURN 1 ELSE RETURN n * Fact(n-1) END "
         "END Fact; END Factorial.",
         true},

        {"QuickSort",
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

        {"LinkedList",
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
         true},
    };

    std::vector<Snippet> invalid = {
        {"MissingSemicolon", "MODULE M END M.", false},
        {"MissingEnd", "MODULE M; VAR x: INTEGER;", false},
        {"MissingDot", "MODULE M; END M", false},
        {"InvalidExpression", "MODULE M; VAR x: INTEGER; BEGIN x := + END M.", false},
    };

    std::vector<ParserSnippetParam> params;
    for (auto cfg : allConfigs()) {
        std::string prefix = std::string(tokenizerName(cfg.tok)) + "_" +
                             parserName(cfg.par) + "_";
        for (auto& s : valid) {
            params.push_back({cfg, prefix + s.name, s.code, s.shouldPass});
        }
        for (auto& s : invalid) {
            params.push_back({cfg, prefix + s.name, s.code, s.shouldPass});
        }
    }
    return params;
}

INSTANTIATE_TEST_SUITE_P(
    AllConfigs, ParserSnippetTest,
    ::testing::ValuesIn(generateParams()),
    [](const auto& info) { return info.param.name; }
);
