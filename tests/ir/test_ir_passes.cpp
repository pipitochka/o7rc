#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "test_factory.h"
#include <ir/IRBuilder.h>
#include <ir/PassManager.h>
#include <ir/passes/ConstantFolding.h>
#include <ir/passes/CopyPropagation.h>
#include <ir/passes/DeadCodeElim.h>

class IRPassesTest : public ::testing::Test {
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

    size_t countOp(const IRFunction& fn, IROp op) {
        size_t n = 0;
        for (auto& bb : fn.blocks)
            for (auto& i : bb->instrs)
                if (i.op == op) ++n;
        return n;
    }

    size_t totalInstrs(const IRFunction& fn) {
        size_t n = 0;
        for (auto& bb : fn.blocks)
            n += bb->instrs.size();
        return n;
    }

    std::vector<std::unique_ptr<std::stringstream>> streams_;
};

// --- ConstantFolding ---

TEST_F(IRPassesTest, ConstFoldBasicArith) {
    IRFunction fn;
    fn.name = "test";
    auto* bb = fn.createBlock("entry");

    IRValue t0 = fn.freshTemp();
    bb->instrs.push_back({IROp::Add, t0, IRValue::constant(3), IRValue::constant(4)});

    IRInstr ret;
    ret.op = IROp::Ret;
    ret.src1 = t0;
    bb->instrs.push_back(ret);

    ConstantFolding cf;
    bool changed = cf.run(fn);
    EXPECT_TRUE(changed);
    EXPECT_EQ(bb->instrs[0].op, IROp::Copy);
    EXPECT_TRUE(bb->instrs[0].src1.isConst());
    EXPECT_EQ(bb->instrs[0].src1.constVal, 7);
}

TEST_F(IRPassesTest, ConstFoldNeg) {
    IRFunction fn;
    fn.name = "test";
    auto* bb = fn.createBlock("entry");

    IRValue t0 = fn.freshTemp();
    bb->instrs.push_back({IROp::Neg, t0, IRValue::constant(5), IRValue::voidVal()});

    IRInstr ret;
    ret.op = IROp::Ret;
    ret.src1 = t0;
    bb->instrs.push_back(ret);

    ConstantFolding cf;
    bool changed = cf.run(fn);
    EXPECT_TRUE(changed);
    EXPECT_EQ(bb->instrs[0].op, IROp::Copy);
    EXPECT_EQ(bb->instrs[0].src1.constVal, -5);
}

TEST_F(IRPassesTest, ConstFoldNoChangeOnNonConst) {
    IRFunction fn;
    fn.name = "test";
    auto* bb = fn.createBlock("entry");

    IRValue t0 = fn.freshTemp();
    IRValue t1 = fn.freshTemp();
    bb->instrs.push_back({IROp::Add, t1, t0, IRValue::constant(1)});

    IRInstr ret;
    ret.op = IROp::Ret;
    ret.src1 = t1;
    bb->instrs.push_back(ret);

    ConstantFolding cf;
    bool changed = cf.run(fn);
    EXPECT_FALSE(changed);
    EXPECT_EQ(bb->instrs[0].op, IROp::Add);
}

TEST_F(IRPassesTest, ConstFoldComparison) {
    IRFunction fn;
    fn.name = "test";
    auto* bb = fn.createBlock("entry");

    IRValue t0 = fn.freshTemp();
    bb->instrs.push_back({IROp::Lt, t0, IRValue::constant(1), IRValue::constant(2)});

    IRInstr ret;
    ret.op = IROp::Ret;
    ret.src1 = t0;
    bb->instrs.push_back(ret);

    ConstantFolding cf;
    cf.run(fn);
    EXPECT_EQ(bb->instrs[0].op, IROp::Copy);
    EXPECT_EQ(bb->instrs[0].src1.constVal, 1);
}

// --- CopyPropagation ---

TEST_F(IRPassesTest, CopyPropBasic) {
    IRFunction fn;
    fn.name = "test";
    auto* bb = fn.createBlock("entry");

    IRValue t0 = fn.freshTemp();
    IRValue t1 = fn.freshTemp();
    IRValue t2 = fn.freshTemp();

    bb->instrs.push_back({IROp::Copy, t0, IRValue::constant(5), IRValue::voidVal()});
    bb->instrs.push_back({IROp::Add, t1, t0, IRValue::constant(3)});

    IRInstr ret;
    ret.op = IROp::Ret;
    ret.src1 = t1;
    bb->instrs.push_back(ret);

    CopyPropagation cp;
    bool changed = cp.run(fn);
    EXPECT_TRUE(changed);
    EXPECT_TRUE(bb->instrs[1].src1.isConst());
    EXPECT_EQ(bb->instrs[1].src1.constVal, 5);
}

// --- DeadCodeElim ---

TEST_F(IRPassesTest, DCERemovesUnused) {
    IRFunction fn;
    fn.name = "test";
    auto* bb = fn.createBlock("entry");

    IRValue t0 = fn.freshTemp();
    IRValue t1 = fn.freshTemp();

    bb->instrs.push_back({IROp::Add, t0, IRValue::constant(1), IRValue::constant(2)});
    bb->instrs.push_back({IROp::Add, t1, IRValue::constant(3), IRValue::constant(4)});

    IRInstr ret;
    ret.op = IROp::Ret;
    ret.src1 = t1;
    bb->instrs.push_back(ret);

    DeadCodeElim dce;
    bool changed = dce.run(fn);
    EXPECT_TRUE(changed);
    EXPECT_EQ(bb->instrs.size(), 2u);
    EXPECT_EQ(bb->instrs[0].op, IROp::Add);
    EXPECT_EQ(bb->instrs[0].dst, t1);
}

TEST_F(IRPassesTest, DCEKeepsSideEffects) {
    IRFunction fn;
    fn.name = "test";
    auto* bb = fn.createBlock("entry");

    IRInstr syscall;
    syscall.op = IROp::Syscall;
    syscall.syscallNum = 1;
    syscall.args = {IRValue::constant(42)};
    bb->instrs.push_back(syscall);

    IRInstr ret;
    ret.op = IROp::Ret;
    ret.src1 = IRValue::voidVal();
    bb->instrs.push_back(ret);

    DeadCodeElim dce;
    bool changed = dce.run(fn);
    EXPECT_FALSE(changed);
    EXPECT_EQ(bb->instrs.size(), 2u);
}

// --- PassManager integration ---

TEST_F(IRPassesTest, PassManagerRunsAll) {
    auto ir = buildFromSource(
        "MODULE M; VAR x: INTEGER; BEGIN x := 2 + 3 END M.");

    PassManager pm;
    pm.add<ConstantFolding>()
      .add<CopyPropagation>()
      .add<DeadCodeElim>();

    std::ostringstream log;
    pm.run(ir, &log);

    std::string output = log.str();
    EXPECT_NE(output.find("ConstantFolding"), std::string::npos);
    EXPECT_NE(output.find("CopyPropagation"), std::string::npos);
    EXPECT_NE(output.find("DeadCodeElim"), std::string::npos);
}

TEST_F(IRPassesTest, PassManagerClear) {
    PassManager pm;
    pm.add<ConstantFolding>()
      .add<CopyPropagation>();
    EXPECT_EQ(pm.size(), 2u);
    pm.clear();
    EXPECT_EQ(pm.size(), 0u);
}

// --- Integration: optimize from source ---

TEST_F(IRPassesTest, OptimizeReducesInstructions) {
    auto ir = buildFromSource(
        "MODULE M; VAR x: INTEGER; BEGIN x := 2 + 3 * 4 END M.");

    size_t before = totalInstrs(ir.mainBody);

    PassManager pm;
    pm.add<ConstantFolding>()
      .add<CopyPropagation>()
      .add<DeadCodeElim>();
    pm.run(ir);

    size_t after = totalInstrs(ir.mainBody);
    EXPECT_LE(after, before);
}
