#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include <parser/impl/bison/BisonParser.h>
#include <tokenizer/impl/flex/FlexTokenizer.h>
#include <util/ast/Ast.h>

namespace {

/**
 * Набор тестов для проверки построения AST операторов (Statement).
 * Код оборачивается в модуль с переменной x: INTEGER,
 * чтобы операторы могли ссылаться на неё.
 */
class BisonParserStmtTest : public ::testing::Test {
protected:
    /**
     * Оборачивает произвольный фрагмент кода (операторы) в минимальный модуль:
     * MODULE Test; VAR x: INTEGER; BEGIN <code> END Test.
     * Возвращает корневой узел AST (Module).
     */
    std::unique_ptr<Module> parseBlock(const std::string& code) {
        std::stringstream ss("MODULE Test; VAR x: INTEGER; BEGIN " + code + " END Test.");
        auto tokenizer = std::make_unique<FlexTokenizer>(ss);
        BisonParser parser;
        return parser.parse(std::move(tokenizer));
    }

    /**
     * Парсит модуль с объявлениями и телом.
     * Для тестов процедур с RETURN.
     */
    std::unique_ptr<Module> parseFull(const std::string& code) {
        std::stringstream ss(code);
        auto tokenizer = std::make_unique<FlexTokenizer>(ss);
        BisonParser parser;
        return parser.parse(std::move(tokenizer));
    }
};

/**
 * Проверяет разбор простого оператора присваивания.
 * Вход: x := 10
 * Ожидание: AssignStmt с lhs.baseName="x" и rhs=LiteralExpr(10).
 */
TEST_F(BisonParserStmtTest, ParsesAssignStatement) {
    auto mod = parseBlock("x := 10");
    ASSERT_EQ(mod->block.size(), 1);

    auto* stmt = dynamic_cast<AssignStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    EXPECT_EQ(stmt->lhs->baseName, "x");
    auto* lit = dynamic_cast<LiteralExpr*>(stmt->rhs.get());
    EXPECT_EQ(lit->intValue, 10);
}

/**
 * Проверяет разбор условного оператора IF-ELSE.
 * Вход: IF x > 0 THEN x := 1 ELSE x := 2 END
 * Ожидание: IfStmt с одной веткой (x>0 → x:=1) и блоком ELSE (x:=2).
 */
TEST_F(BisonParserStmtTest, ParsesIfElseStatement) {
    auto mod = parseBlock("IF x > 0 THEN x := 1 ELSE x := 2 END");

    auto* stmt = dynamic_cast<IfStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    ASSERT_EQ(stmt->branches.size(), 1);
    auto* cond = dynamic_cast<BinaryExpr*>(stmt->branches[0].cond.get());
    EXPECT_EQ(cond->op, BinaryExpr::Op::Gt);

    ASSERT_EQ(stmt->branches[0].body.size(), 1); // x := 1 inside

    ASSERT_EQ(stmt->elseBody.size(), 1); // x := 2 inside
    auto* elseStmt = dynamic_cast<AssignStmt*>(stmt->elseBody[0].get());
    auto* val = dynamic_cast<LiteralExpr*>(elseStmt->rhs.get());
    EXPECT_EQ(val->intValue, 2);
}

/**
 * Проверяет разбор IF с ветками ELSIF.
 * Вход: IF x = 1 THEN x := 10 ELSIF x = 2 THEN x := 20 ELSIF x = 3 THEN x := 30 ELSE x := 0 END
 * Ожидание: IfStmt с 3 ветками (IF + 2 ELSIF) и блоком ELSE.
 */
TEST_F(BisonParserStmtTest, ParsesIfElsifStatement) {
    auto mod = parseBlock(
        "IF x = 1 THEN x := 10 "
        "ELSIF x = 2 THEN x := 20 "
        "ELSIF x = 3 THEN x := 30 "
        "ELSE x := 0 END"
    );

    auto* stmt = dynamic_cast<IfStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    ASSERT_EQ(stmt->branches.size(), 3);

    auto* c1 = dynamic_cast<BinaryExpr*>(stmt->branches[0].cond.get());
    EXPECT_EQ(c1->op, BinaryExpr::Op::Eq);
    auto* c2 = dynamic_cast<BinaryExpr*>(stmt->branches[1].cond.get());
    EXPECT_EQ(c2->op, BinaryExpr::Op::Eq);
    auto* c3 = dynamic_cast<BinaryExpr*>(stmt->branches[2].cond.get());
    EXPECT_EQ(c3->op, BinaryExpr::Op::Eq);

    ASSERT_EQ(stmt->elseBody.size(), 1);
    auto* elseAssign = dynamic_cast<AssignStmt*>(stmt->elseBody[0].get());
    auto* elseVal = dynamic_cast<LiteralExpr*>(elseAssign->rhs.get());
    EXPECT_EQ(elseVal->intValue, 0);
}

/**
 * Проверяет разбор IF без ELSE.
 * Вход: IF x > 0 THEN x := 1 END
 * Ожидание: IfStmt с одной веткой и пустым elseBody.
 */
TEST_F(BisonParserStmtTest, ParsesIfWithoutElse) {
    auto mod = parseBlock("IF x > 0 THEN x := 1 END");

    auto* stmt = dynamic_cast<IfStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->branches.size(), 1);
    EXPECT_TRUE(stmt->elseBody.empty());
}

/**
 * Проверяет разбор цикла WHILE.
 * Вход: WHILE x < 10 DO x := x + 1 END
 * Ожидание: WhileStmt с условием Lt и телом из одного оператора.
 */
TEST_F(BisonParserStmtTest, ParsesWhileLoop) {
    auto mod = parseBlock("WHILE x < 10 DO x := x + 1 END");

    auto* stmt = dynamic_cast<WhileStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    auto* cond = dynamic_cast<BinaryExpr*>(stmt->cond.get());
    EXPECT_EQ(cond->op, BinaryExpr::Op::Lt);

    ASSERT_EQ(stmt->body.size(), 1);
}

/**
 * Проверяет разбор цикла REPEAT-UNTIL.
 * Вход: REPEAT x := x + 1 UNTIL x >= 10
 * Ожидание: RepeatStmt с телом из одного оператора и условием Ge.
 */
TEST_F(BisonParserStmtTest, ParsesRepeatUntilLoop) {
    auto mod = parseBlock("REPEAT x := x + 1 UNTIL x >= 10");

    auto* stmt = dynamic_cast<RepeatStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    ASSERT_EQ(stmt->body.size(), 1);

    auto* cond = dynamic_cast<BinaryExpr*>(stmt->untilCond.get());
    ASSERT_NE(cond, nullptr);
    EXPECT_EQ(cond->op, BinaryExpr::Op::Ge);
}

/**
 * Проверяет разбор цикла FOR с шагом BY.
 * Вход: FOR x := 1 TO 10 BY 2 DO x := 0 END
 * Ожидание: ForStmt с varName="x", from=1, to=10, by=2.
 */
TEST_F(BisonParserStmtTest, ParsesForLoop) {
    auto mod = parseBlock("FOR x := 1 TO 10 BY 2 DO x := 0 END");

    auto* stmt = dynamic_cast<ForStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    EXPECT_EQ(stmt->varName, "x");

    auto* from = dynamic_cast<LiteralExpr*>(stmt->from.get());
    EXPECT_EQ(from->intValue, 1);

    auto* to = dynamic_cast<LiteralExpr*>(stmt->to.get());
    EXPECT_EQ(to->intValue, 10);

    auto* by = dynamic_cast<LiteralExpr*>(stmt->by.get());
    ASSERT_NE(by, nullptr);
    EXPECT_EQ(by->intValue, 2);
}

/**
 * Проверяет разбор FOR без секции BY (шаг по умолчанию).
 * Вход: FOR x := 0 TO 5 DO x := 0 END
 * Ожидание: ForStmt с by=nullptr.
 */
TEST_F(BisonParserStmtTest, ParsesForLoopWithoutBy) {
    auto mod = parseBlock("FOR x := 0 TO 5 DO x := 0 END");

    auto* stmt = dynamic_cast<ForStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    EXPECT_EQ(stmt->varName, "x");
    EXPECT_EQ(stmt->by, nullptr);
}

/**
 * Проверяет разбор оператора CASE с несколькими альтернативами.
 * Вход: CASE x OF 1: x := 10 | 2, 3: x := 20 | 4..6: x := 30 END
 * Ожидание: CaseStmt с expr=designator(x) и 3 альтернативами.
 */
TEST_F(BisonParserStmtTest, ParsesCaseStatement) {
    auto mod = parseBlock("CASE x OF 1: x := 10 | 2, 3: x := 20 | 4..6: x := 30 END");

    auto* stmt = dynamic_cast<CaseStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    auto* caseExpr = dynamic_cast<DesignatorExpr*>(stmt->expr.get());
    ASSERT_NE(caseExpr, nullptr);
    EXPECT_EQ(caseExpr->baseName, "x");

    ASSERT_EQ(stmt->alts.size(), 3);

    ASSERT_EQ(stmt->alts[0]->labels.size(), 1);
    auto* label1 = dynamic_cast<LiteralExpr*>(stmt->alts[0]->labels[0]->from.get());
    EXPECT_EQ(label1->intValue, 1);
    EXPECT_EQ(stmt->alts[0]->labels[0]->to, nullptr);

    ASSERT_EQ(stmt->alts[1]->labels.size(), 2);

    ASSERT_EQ(stmt->alts[2]->labels.size(), 1);
    auto* rangeFrom = dynamic_cast<LiteralExpr*>(stmt->alts[2]->labels[0]->from.get());
    auto* rangeTo = dynamic_cast<LiteralExpr*>(stmt->alts[2]->labels[0]->to.get());
    ASSERT_NE(rangeFrom, nullptr);
    ASSERT_NE(rangeTo, nullptr);
    EXPECT_EQ(rangeFrom->intValue, 4);
    EXPECT_EQ(rangeTo->intValue, 6);
}

/**
 * Проверяет разбор вызова процедуры как отдельного оператора.
 * Вход: Write(x)
 * Ожидание: CallStmt с designator.baseName="Write" и ArgsSelector с 1 аргументом.
 */
TEST_F(BisonParserStmtTest, ParsesCallStatement) {
    auto mod = parseBlock("Write(x)");
    ASSERT_EQ(mod->block.size(), 1);

    auto* stmt = dynamic_cast<CallStmt*>(mod->block[0].get());
    ASSERT_NE(stmt, nullptr);

    auto* des = stmt->designator.get();
    ASSERT_NE(des, nullptr);
    EXPECT_EQ(des->baseName, "Write");

    ASSERT_EQ(des->selectors.size(), 1);
    auto* args = dynamic_cast<ArgsSelector*>(des->selectors[0].get());
    ASSERT_NE(args, nullptr);
    EXPECT_EQ(args->args.size(), 1);
}

/**
 * Проверяет разбор оператора RETURN с выражением.
 * Вход: процедура с RETURN a + b.
 */
TEST_F(BisonParserStmtTest, ParsesReturnStatement) {
    auto mod = parseFull(
        "MODULE Test; "
        "PROCEDURE Add(a, b: INTEGER): INTEGER; "
        "BEGIN RETURN a + b "
        "END Add; "
        "END Test."
    );
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->decls.size(), 1);

    auto* proc = dynamic_cast<ProcDecl*>(mod->decls[0].get());
    ASSERT_NE(proc, nullptr);
    ASSERT_GE(proc->body.size(), 1);

    auto* ret = dynamic_cast<ReturnStmt*>(proc->body[0].get());
    ASSERT_NE(ret, nullptr);

    auto* val = dynamic_cast<BinaryExpr*>(ret->value.get());
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->op, BinaryExpr::Op::Add);
}

/**
 * Проверяет разбор нескольких операторов, разделённых точкой с запятой.
 * Вход: x := 1; x := 2; x := 3
 * Ожидание: 3 оператора AssignStmt в блоке.
 */
TEST_F(BisonParserStmtTest, ParsesMultipleStatements) {
    auto mod = parseBlock("x := 1; x := 2; x := 3");

    ASSERT_EQ(mod->block.size(), 3);
    for (int i = 0; i < 3; ++i) {
        auto* stmt = dynamic_cast<AssignStmt*>(mod->block[i].get());
        ASSERT_NE(stmt, nullptr) << "Statement " << i << " is not AssignStmt";
    }
}

} // namespace
