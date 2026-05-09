#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "test_factory.h"
#include <sema/Sema.h>

struct FrontendConfig {
    TokenizerKind tok;
    ParserKind par;
};

class SemaTest : public ::testing::TestWithParam<FrontendConfig> {
protected:
    SemaErrors analyze(const std::string& code) {
        auto ss = std::make_unique<std::stringstream>(code);
        auto tokenizer = makeTokenizer(GetParam().tok, *ss);
        auto parser = makeParser(GetParam().par);
        streams_.push_back(std::move(ss));
        auto mod = parser->parse(std::move(tokenizer));
        EXPECT_NE(mod, nullptr);
        Sema sema;
        return sema.analyze(*mod);
    }

    bool hasError(const SemaErrors& errs, SemaError::Kind kind) {
        for (auto& e : errs)
            if (e.kind == kind) return true;
        return false;
    }

    bool hasErrorContaining(const SemaErrors& errs, const std::string& substr) {
        for (auto& e : errs)
            if (e.message.find(substr) != std::string::npos) return true;
        return false;
    }

    std::vector<std::unique_ptr<std::stringstream>> streams_;
};

static std::vector<FrontendConfig> allConfigs() {
    std::vector<FrontendConfig> v;
    for (auto t : availableTokenizers())
        for (auto p : availableParsers())
            v.push_back({t, p});
    return v;
}

static std::string configName(const ::testing::TestParamInfo<FrontendConfig>& info) {
    return std::string(tokenizerName(info.param.tok)) + "_" +
           parserName(info.param.par);
}

// --- Valid programs should have zero errors ---

TEST_P(SemaTest, EmptyModuleNoErrors) {
    auto errs = analyze("MODULE M; END M.");
    EXPECT_TRUE(errs.empty());
}

TEST_P(SemaTest, VarDeclAndAssignNoErrors) {
    auto errs = analyze("MODULE M; VAR x: INTEGER; BEGIN x := 42 END M.");
    EXPECT_TRUE(errs.empty());
}

TEST_P(SemaTest, ConstDeclNoErrors) {
    auto errs = analyze("MODULE M; CONST N = 10; END M.");
    EXPECT_TRUE(errs.empty());
}

TEST_P(SemaTest, LongrealVarDeclNoErrors) {
    auto errs = analyze("MODULE M; VAR x: LONGREAL; END M.");
    EXPECT_TRUE(errs.empty());
}

TEST_P(SemaTest, LongrealAssignmentLiteralNoErrors) {
    auto errs = analyze(
        "MODULE M; VAR x: LONGREAL; BEGIN x := 3.141592653589793 END M.");
    EXPECT_TRUE(errs.empty());
}

TEST_P(SemaTest, ProcedureNoErrors) {
    auto errs = analyze(
        "MODULE M; "
        "PROCEDURE Add(a, b: INTEGER): INTEGER; "
        "RETURN a + b "
        "END Add; "
        "END M.");
    EXPECT_TRUE(errs.empty());
}

TEST_P(SemaTest, ProcCallNoErrors) {
    auto errs = analyze(
        "MODULE M; "
        "VAR x: INTEGER; "
        "PROCEDURE Inc(n: INTEGER): INTEGER; "
        "RETURN n + 1 "
        "END Inc; "
        "BEGIN x := Inc(5) "
        "END M.");
    EXPECT_TRUE(errs.empty()) << errs[0].message;
}

TEST_P(SemaTest, ImportNoErrors) {
    auto errs = analyze(
        "MODULE M; IMPORT Out; "
        "BEGIN Out.Int(1, 0); Out.Ln "
        "END M.");
    EXPECT_TRUE(errs.empty());
}

TEST_P(SemaTest, IfWhileForRepeatNoErrors) {
    auto errs = analyze(
        "MODULE M; VAR i, s: INTEGER; "
        "BEGIN "
        "  s := 0; "
        "  IF s = 0 THEN s := 1 END; "
        "  WHILE s < 10 DO s := s + 1 END; "
        "  FOR i := 1 TO 10 DO s := s + i END; "
        "  REPEAT s := s - 1 UNTIL s = 0 "
        "END M.");
    EXPECT_TRUE(errs.empty());
}

TEST_P(SemaTest, BuiltinProcsNoErrors) {
    auto errs = analyze(
        "MODULE M; VAR x: INTEGER; "
        "BEGIN x := 5; INC(x); DEC(x); x := ABS(x); x := ORD(TRUE) "
        "END M.");
    EXPECT_TRUE(errs.empty()) << errs[0].message;
}

TEST_P(SemaTest, ArrayDeclNoErrors) {
    auto errs = analyze(
        "MODULE M; VAR a: ARRAY 10 OF INTEGER; BEGIN a[0] := 1 END M.");
    EXPECT_TRUE(errs.empty());
}

TEST_P(SemaTest, TypeDeclNoErrors) {
    auto errs = analyze(
        "MODULE M; "
        "TYPE Vec = ARRAY 3 OF INTEGER; "
        "VAR v: Vec; "
        "END M.");
    EXPECT_TRUE(errs.empty());
}

TEST_P(SemaTest, NestedProcedureNoErrors) {
    auto errs = analyze(
        "MODULE M; "
        "PROCEDURE Outer; "
        "  VAR x: INTEGER; "
        "  PROCEDURE Inner; "
        "    VAR y: INTEGER; "
        "  BEGIN y := 1 "
        "  END Inner; "
        "BEGIN x := 2; Inner "
        "END Outer; "
        "END M.");
    EXPECT_TRUE(errs.empty());
}

// --- Error detection ---

TEST_P(SemaTest, UndefinedVariable) {
    auto errs = analyze("MODULE M; BEGIN x := 42 END M.");
    EXPECT_TRUE(hasError(errs, SemaError::UndefinedSymbol));
    EXPECT_TRUE(hasErrorContaining(errs, "'x'"));
}

TEST_P(SemaTest, UndefinedProcedure) {
    auto errs = analyze(
        "MODULE M; VAR x: INTEGER; BEGIN x := Foo(1) END M.");
    EXPECT_TRUE(hasError(errs, SemaError::UndefinedSymbol));
    EXPECT_TRUE(hasErrorContaining(errs, "'Foo'"));
}

TEST_P(SemaTest, DuplicateVarDecl) {
    auto errs = analyze("MODULE M; VAR x: INTEGER; x: BOOLEAN; END M.");
    EXPECT_TRUE(hasError(errs, SemaError::DuplicateSymbol));
    EXPECT_TRUE(hasErrorContaining(errs, "'x'"));
}

TEST_P(SemaTest, DuplicateConstDecl) {
    auto errs = analyze("MODULE M; CONST N = 1; M = 2; N = 3; END M.");
    EXPECT_TRUE(hasError(errs, SemaError::DuplicateSymbol));
}

TEST_P(SemaTest, DuplicateProcDecl) {
    auto errs = analyze(
        "MODULE M; "
        "PROCEDURE P; BEGIN END P; "
        "PROCEDURE P; BEGIN END P; "
        "END M.");
    EXPECT_TRUE(hasError(errs, SemaError::DuplicateSymbol));
}

TEST_P(SemaTest, AssignToConst) {
    auto errs = analyze("MODULE M; CONST N = 5; BEGIN N := 10 END M.");
    EXPECT_TRUE(hasError(errs, SemaError::ConstAssign));
    EXPECT_TRUE(hasErrorContaining(errs, "'N'"));
}

TEST_P(SemaTest, MissingReturn) {
    auto errs = analyze(
        "MODULE M; "
        "PROCEDURE F(n: INTEGER): INTEGER; "
        "  VAR x: INTEGER; "
        "BEGIN x := n "
        "END F; "
        "END M.");
    EXPECT_TRUE(hasError(errs, SemaError::MissingReturn));
    EXPECT_TRUE(hasErrorContaining(errs, "'F'"));
}

TEST_P(SemaTest, ArityMismatch) {
    auto errs = analyze(
        "MODULE M; "
        "VAR x: INTEGER; "
        "PROCEDURE Add(a, b: INTEGER): INTEGER; RETURN a + b END Add; "
        "BEGIN x := Add(1) "
        "END M.");
    EXPECT_TRUE(hasError(errs, SemaError::ArityMismatch));
    EXPECT_TRUE(hasErrorContaining(errs, "2"));
}

TEST_P(SemaTest, ArityMismatchTooMany) {
    auto errs = analyze(
        "MODULE M; "
        "VAR x: INTEGER; "
        "PROCEDURE One(a: INTEGER): INTEGER; RETURN a END One; "
        "BEGIN x := One(1, 2, 3) "
        "END M.");
    EXPECT_TRUE(hasError(errs, SemaError::ArityMismatch));
}

TEST_P(SemaTest, ModuleNameMismatch) {
    auto errs = analyze("MODULE A; END B.");
    EXPECT_TRUE(hasError(errs, SemaError::ModuleNameMismatch));
}

TEST_P(SemaTest, UndefinedForVariable) {
    auto errs = analyze(
        "MODULE M; VAR s: INTEGER; "
        "BEGIN s := 0; FOR i := 1 TO 10 DO s := s + 1 END "
        "END M.");
    EXPECT_TRUE(hasError(errs, SemaError::UndefinedSymbol));
    EXPECT_TRUE(hasErrorContaining(errs, "'i'"));
}

TEST_P(SemaTest, UndefinedType) {
    auto errs = analyze("MODULE M; VAR x: Foo; END M.");
    EXPECT_TRUE(hasError(errs, SemaError::UndefinedSymbol));
    EXPECT_TRUE(hasErrorContaining(errs, "'Foo'"));
}

TEST_P(SemaTest, UndefinedModule) {
    auto errs = analyze("MODULE M; BEGIN Foo.Bar END M.");
    EXPECT_TRUE(hasError(errs, SemaError::UndefinedSymbol));
    EXPECT_TRUE(hasErrorContaining(errs, "'Foo'"));
}

// --- Procedure without return doesn't need RETURN ---

TEST_P(SemaTest, VoidProcNoReturnOk) {
    auto errs = analyze(
        "MODULE M; "
        "PROCEDURE DoStuff; "
        "  VAR x: INTEGER; "
        "BEGIN x := 1 "
        "END DoStuff; "
        "END M.");
    EXPECT_FALSE(hasError(errs, SemaError::MissingReturn));
}

// --- Scope: local var shadows global ---

TEST_P(SemaTest, LocalShadowsGlobal) {
    auto errs = analyze(
        "MODULE M; "
        "VAR x: INTEGER; "
        "PROCEDURE P; VAR x: BOOLEAN; BEGIN x := TRUE END P; "
        "BEGIN x := 1 "
        "END M.");
    EXPECT_TRUE(errs.empty());
}

// --- Multiple errors in one program ---

TEST_P(SemaTest, MultipleErrors) {
    auto errs = analyze(
        "MODULE M; "
        "CONST N = 5; "
        "BEGIN "
        "  N := 10; "
        "  y := 42 "
        "END M.");
    EXPECT_GE(errs.size(), 2u);
    EXPECT_TRUE(hasError(errs, SemaError::ConstAssign));
    EXPECT_TRUE(hasError(errs, SemaError::UndefinedSymbol));
}

INSTANTIATE_TEST_SUITE_P(
    AllConfigs, SemaTest,
    ::testing::ValuesIn(allConfigs()),
    configName
);
