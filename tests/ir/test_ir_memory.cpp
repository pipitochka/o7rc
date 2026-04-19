#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "test_factory.h"
#include <ir/IRBuilder.h>
#include <ir/IRPrinter.h>

class IRMemoryTest : public ::testing::Test {
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

    bool hasOp(const IRFunction& fn, IROp op) {
        for (auto& bb : fn.blocks)
            for (auto& i : bb->instrs)
                if (i.op == op) return true;
        return false;
    }

    const IRInstr* findOp(const IRFunction& fn, IROp op) {
        for (auto& bb : fn.blocks)
            for (auto& i : bb->instrs)
                if (i.op == op) return &i;
        return nullptr;
    }

    int countOp(const IRFunction& fn, IROp op) {
        int n = 0;
        for (auto& bb : fn.blocks)
            for (auto& i : bb->instrs)
                if (i.op == op) ++n;
        return n;
    }

    const IRInstr* findSyscall(const IRFunction& fn, int num) {
        for (auto& bb : fn.blocks)
            for (auto& i : bb->instrs)
                if (i.op == IROp::Syscall && i.syscallNum == num) return &i;
        return nullptr;
    }

    std::vector<std::unique_ptr<std::stringstream>> streams_;
};

// --- Record global variable layout ---

TEST_F(IRMemoryTest, RecordGlobalUsesSpace) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Point = RECORD x, y: INTEGER END; "
        "VAR r: Point; END M.");
    ASSERT_EQ(ir.globals.size(), 1u);
    EXPECT_EQ(ir.globals[0].name, "r");
    EXPECT_EQ(ir.globals[0].size, 8);
    EXPECT_TRUE(ir.globals[0].isArray);
}

TEST_F(IRMemoryTest, RecordThreeFieldsSize) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Vec3 = RECORD x, y, z: INTEGER END; "
        "VAR v: Vec3; END M.");
    ASSERT_EQ(ir.globals.size(), 1u);
    EXPECT_EQ(ir.globals[0].size, 12);
}

TEST_F(IRMemoryTest, PointerGlobalIsWord) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Node = RECORD val: INTEGER END; "
        "VAR p: POINTER TO Node; END M.");
    ASSERT_EQ(ir.globals.size(), 1u);
    EXPECT_EQ(ir.globals[0].size, 4);
    EXPECT_FALSE(ir.globals[0].isArray);
}

// --- Local record alloca size ---

TEST_F(IRMemoryTest, LocalRecordAllocaCorrectSize) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Point = RECORD x, y: INTEGER END; "
        "PROCEDURE P; VAR r: Point; BEGIN r.x := 1 END P; END M.");
    ASSERT_EQ(ir.functions.size(), 1u);
    auto* alloca = findOp(ir.functions[0], IROp::Alloca);
    ASSERT_NE(alloca, nullptr);
    EXPECT_EQ(alloca->src1.constVal, 8);
}

// --- NEW allocates correct size ---

TEST_F(IRMemoryTest, NewAllocatesRecordSize) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Point = RECORD x, y: INTEGER END; "
        "VAR p: POINTER TO Point; BEGIN NEW(p) END M.");
    auto* sc = findSyscall(ir.mainBody, 9);
    ASSERT_NE(sc, nullptr);
    ASSERT_FALSE(sc->args.empty());
    EXPECT_EQ(sc->args[0].constVal, 8);
}

TEST_F(IRMemoryTest, NewAllocatesLargerRecord) {
    auto ir = buildFromSource(
        "MODULE M; TYPE R = RECORD a, b, c, d: INTEGER END; "
        "VAR p: POINTER TO R; BEGIN NEW(p) END M.");
    auto* sc = findSyscall(ir.mainBody, 9);
    ASSERT_NE(sc, nullptr);
    ASSERT_FALSE(sc->args.empty());
    EXPECT_EQ(sc->args[0].constVal, 16);
}

TEST_F(IRMemoryTest, NewStoresPointer) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Node = RECORD val: INTEGER END; "
        "VAR p: POINTER TO Node; BEGIN NEW(p) END M.");
    EXPECT_TRUE(hasOp(ir.mainBody, IROp::Store));
}

// --- Field access IR ---

TEST_F(IRMemoryTest, RecordFieldAccessFirstField) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Point = RECORD x, y: INTEGER END; "
        "VAR r: Point; BEGIN r.x := 42 END M.");
    bool foundStore42 = false;
    for (auto& bb : ir.mainBody.blocks)
        for (auto& i : bb->instrs)
            if (i.op == IROp::Store && i.src2.isConst() && i.src2.constVal == 42)
                foundStore42 = true;
    EXPECT_TRUE(foundStore42);
}

TEST_F(IRMemoryTest, RecordFieldAccessSecondField) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Point = RECORD x, y: INTEGER END; "
        "VAR r: Point; BEGIN r.y := 99 END M.");
    bool foundAdd4 = false;
    for (auto& bb : ir.mainBody.blocks)
        for (auto& i : bb->instrs)
            if (i.op == IROp::Add && i.src2.isConst() && i.src2.constVal == 4)
                foundAdd4 = true;
    EXPECT_TRUE(foundAdd4);
}

TEST_F(IRMemoryTest, PointerFieldAccessLoadsPointer) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Node = RECORD val: INTEGER END; "
        "VAR p: POINTER TO Node; BEGIN NEW(p); p.val := 10 END M.");
    int loads = countOp(ir.mainBody, IROp::Load);
    EXPECT_GE(loads, 1);
}

// --- Read back through pointer ---

TEST_F(IRMemoryTest, PointerFieldReadProducesLoad) {
    auto ir = buildFromSource(
        "MODULE M; IMPORT Out; "
        "TYPE Node = RECORD val: INTEGER END; "
        "VAR p: POINTER TO Node; "
        "BEGIN NEW(p); p.val := 5; Out.Int(p.val, 0) END M.");
    int loads = countOp(ir.mainBody, IROp::Load);
    EXPECT_GE(loads, 2);
}

// --- NIL ---

TEST_F(IRMemoryTest, NilAssignment) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Node = RECORD val: INTEGER END; "
        "VAR p: POINTER TO Node; BEGIN p := NIL END M.");
    bool foundStore0 = false;
    for (auto& bb : ir.mainBody.blocks)
        for (auto& i : bb->instrs)
            if (i.op == IROp::Store && i.src2.isConst() && i.src2.constVal == 0)
                foundStore0 = true;
    EXPECT_TRUE(foundStore0);
}

// --- Multiple fields accessed sequentially ---

TEST_F(IRMemoryTest, MultipleFieldWrites) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Point = RECORD x, y: INTEGER END; "
        "VAR r: Point; BEGIN r.x := 10; r.y := 20 END M.");
    int stores = countOp(ir.mainBody, IROp::Store);
    EXPECT_GE(stores, 2);
}

// --- Record with mixed named type fields ---

TEST_F(IRMemoryTest, RecordWithBooleanField) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Flags = RECORD active: BOOLEAN; count: INTEGER END; "
        "VAR f: Flags; END M.");
    ASSERT_EQ(ir.globals.size(), 1u);
    EXPECT_EQ(ir.globals[0].size, 8);
}

// --- Procedure with pointer parameter ---

TEST_F(IRMemoryTest, ProcWithPointerParam) {
    auto ir = buildFromSource(
        "MODULE M; TYPE Node = RECORD val: INTEGER END; "
        "PNode = POINTER TO Node; "
        "PROCEDURE P(p: PNode): INTEGER; "
        "RETURN 0 END P; END M.");
    ASSERT_EQ(ir.functions.size(), 1u);
    EXPECT_EQ(ir.functions[0].params.size(), 1u);
}
