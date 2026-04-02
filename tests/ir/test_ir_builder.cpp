#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "test_factory.h"
#include <ir/IRBuilder.h>
#include <ir/IRPrinter.h>

class IRBuilderTest : public ::testing::Test {
protected:
    IRModule buildFromSource(const std::string& code) {
        auto ss = std::make_unique<std::stringstream>(code);
        auto tok = makeTokenizer(availableTokenizers().front(), *ss);
        auto par = makeParser(availableParsers().front());
        streams_.push_back(std::move(ss));
        auto mod = par->parse(std::move(tok));
        EXPECT_NE(mod, nullptr);
        IRBuilder builder;
        return builder.build(*mod);
    }

    std::string dumpIR(const IRModule& m) {
        std::ostringstream os;
        IRPrinter printer;
        printer.print(m, os);
        return os.str();
    }

    std::vector<std::unique_ptr<std::stringstream>> streams_;
};

TEST_F(IRBuilderTest, EmptyModule) {
    auto ir = buildFromSource("MODULE M; END M.");
    EXPECT_EQ(ir.name, "M");
    EXPECT_TRUE(ir.functions.empty());
    EXPECT_FALSE(ir.mainBody.blocks.empty());
    auto* entry = ir.mainBody.entry();
    ASSERT_NE(entry, nullptr);
    EXPECT_FALSE(entry->instrs.empty());
    EXPECT_EQ(entry->instrs.back().op, IROp::Ret);
}

TEST_F(IRBuilderTest, GlobalVariable) {
    auto ir = buildFromSource("MODULE M; VAR x: INTEGER; END M.");
    ASSERT_EQ(ir.globals.size(), 1u);
    EXPECT_EQ(ir.globals[0].name, "x");
    EXPECT_EQ(ir.globals[0].size, 4);
}

TEST_F(IRBuilderTest, ConstantDecl) {
    auto ir = buildFromSource(
        "MODULE M; CONST N = 10; VAR x: INTEGER; "
        "BEGIN x := N END M.");
    ASSERT_FALSE(ir.mainBody.blocks.empty());
    auto& instrs = ir.mainBody.entry()->instrs;
    bool foundStore = false;
    for (auto& i : instrs) {
        if (i.op == IROp::Store && i.src2.isConst() && i.src2.constVal == 10) {
            foundStore = true;
        }
    }
    EXPECT_TRUE(foundStore);
}

TEST_F(IRBuilderTest, ProcedureCreated) {
    auto ir = buildFromSource(
        "MODULE M; "
        "PROCEDURE Foo(n: INTEGER): INTEGER; "
        "RETURN n "
        "END Foo; "
        "END M.");
    ASSERT_EQ(ir.functions.size(), 1u);
    EXPECT_EQ(ir.functions[0].name, "Foo");
    EXPECT_TRUE(ir.functions[0].hasReturn);
    ASSERT_EQ(ir.functions[0].params.size(), 1u);
    EXPECT_EQ(ir.functions[0].params[0], "n");
}

TEST_F(IRBuilderTest, ArithmeticExpressions) {
    auto ir = buildFromSource(
        "MODULE M; VAR x: INTEGER; BEGIN x := 2 + 3 * 4 END M.");
    auto& instrs = ir.mainBody.entry()->instrs;
    bool hasMul = false, hasAdd = false;
    for (auto& i : instrs) {
        if (i.op == IROp::Mul) hasMul = true;
        if (i.op == IROp::Add) hasAdd = true;
    }
    EXPECT_TRUE(hasMul);
    EXPECT_TRUE(hasAdd);
}

TEST_F(IRBuilderTest, IfStmtProducesBranch) {
    auto ir = buildFromSource(
        "MODULE M; VAR x: INTEGER; "
        "BEGIN IF x = 0 THEN x := 1 ELSE x := 2 END END M.");
    bool hasBranch = false;
    for (auto& bb : ir.mainBody.blocks) {
        for (auto& i : bb->instrs) {
            if (i.op == IROp::Branch) hasBranch = true;
        }
    }
    EXPECT_TRUE(hasBranch);
    EXPECT_GT(ir.mainBody.blocks.size(), 1u);
}

TEST_F(IRBuilderTest, WhileStmtProducesLoop) {
    auto ir = buildFromSource(
        "MODULE M; VAR i: INTEGER; "
        "BEGIN i := 0; WHILE i < 10 DO i := i + 1 END END M.");
    bool hasBranch = false, hasJump = false;
    for (auto& bb : ir.mainBody.blocks) {
        for (auto& i : bb->instrs) {
            if (i.op == IROp::Branch) hasBranch = true;
            if (i.op == IROp::Jump) hasJump = true;
        }
    }
    EXPECT_TRUE(hasBranch);
    EXPECT_TRUE(hasJump);
}

TEST_F(IRBuilderTest, ForStmtProducesLoop) {
    auto ir = buildFromSource(
        "MODULE M; VAR i, s: INTEGER; "
        "BEGIN s := 0; FOR i := 1 TO 10 DO s := s + i END END M.");
    bool hasBranch = false;
    for (auto& bb : ir.mainBody.blocks) {
        for (auto& i : bb->instrs)
            if (i.op == IROp::Branch) hasBranch = true;
    }
    EXPECT_TRUE(hasBranch);
}

TEST_F(IRBuilderTest, RepeatStmtProducesLoop) {
    auto ir = buildFromSource(
        "MODULE M; VAR i: INTEGER; "
        "BEGIN i := 0; REPEAT i := i + 1 UNTIL i >= 10 END M.");
    bool hasBranch = false;
    for (auto& bb : ir.mainBody.blocks) {
        for (auto& i : bb->instrs)
            if (i.op == IROp::Branch) hasBranch = true;
    }
    EXPECT_TRUE(hasBranch);
}

TEST_F(IRBuilderTest, CFGSuccessorsAndPredecessors) {
    auto ir = buildFromSource(
        "MODULE M; VAR x: INTEGER; "
        "BEGIN IF x = 0 THEN x := 1 END END M.");
    bool anySuccessors = false;
    bool anyPredecessors = false;
    for (auto& bb : ir.mainBody.blocks) {
        if (!bb->successors.empty()) anySuccessors = true;
        if (!bb->predecessors.empty()) anyPredecessors = true;
    }
    EXPECT_TRUE(anySuccessors);
    EXPECT_TRUE(anyPredecessors);
}

TEST_F(IRBuilderTest, EveryBlockHasTerminator) {
    auto ir = buildFromSource(
        "MODULE M; "
        "PROCEDURE Fact(n: INTEGER): INTEGER; "
        "VAR r: INTEGER; "
        "BEGIN "
        "  IF n <= 1 THEN r := 1 "
        "  ELSE r := n * Fact(n - 1) "
        "  END "
        "RETURN r "
        "END Fact; "
        "END M.");
    for (auto& fn : ir.functions) {
        for (auto& bb : fn.blocks) {
            if (bb->instrs.empty()) continue;
            EXPECT_TRUE(bb->hasTerminator())
                << "Block " << bb->label << " in " << fn.name << " has no terminator";
        }
    }
}

TEST_F(IRBuilderTest, IRPrinterProducesOutput) {
    auto ir = buildFromSource(
        "MODULE M; VAR x: INTEGER; BEGIN x := 42 END M.");
    std::string dump = dumpIR(ir);
    EXPECT_FALSE(dump.empty());
    EXPECT_NE(dump.find("M"), std::string::npos);
    EXPECT_NE(dump.find("store"), std::string::npos);
    EXPECT_NE(dump.find("42"), std::string::npos);
}

TEST_F(IRBuilderTest, ArrayGlobal) {
    auto ir = buildFromSource(
        "MODULE M; VAR a: ARRAY 5 OF INTEGER; END M.");
    ASSERT_EQ(ir.globals.size(), 1u);
    EXPECT_TRUE(ir.globals[0].isArray);
    EXPECT_EQ(ir.globals[0].size, 20);
}
