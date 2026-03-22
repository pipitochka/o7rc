#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include <parser/impl/bison/BisonParser.h>
#include <tokenizer/impl/flex/FlexTokenizer.h>
#include <util/ast/Ast.h>

namespace {

/**
 * Набор тестов для проверки построения AST выражений (Expression)
 * парсером Bison. Каждый тест оборачивает выражение в минимальный
 * модуль: MODULE Test; VAR x: INTEGER; BEGIN <body> END Test.
 */
class BisonParserExprTest : public ::testing::Test {
protected:
    /**
     * Оборачивает произвольный фрагмент кода body в минимальный модуль,
     * запускает лексер и парсер и возвращает корневой узел AST (Module).
     */
    std::unique_ptr<Module> parseModule(const std::string& body) {
        std::stringstream ss("MODULE Test; VAR x: INTEGER; BEGIN " + body + " END Test.");
        auto tokenizer = std::make_unique<FlexTokenizer>(ss);
        BisonParser parser;
        return parser.parse(std::move(tokenizer));
    }

    /**
     * Извлекает правую часть (rhs) первого оператора присваивания
     * из блока BEGIN модуля. Возвращает nullptr, если модуль пуст
     * или первый оператор не является присваиванием.
     */
    Expr* getFirstAssignmentExpr(Module* mod) {
        if (!mod || mod->block.empty()) return nullptr;
        auto* assign = dynamic_cast<AssignStmt*>(mod->block[0].get());
        if (!assign) return nullptr;
        return assign->rhs.get();
    }
};

/**
 * Проверяет корректный разбор целочисленного литерала.
 * Вход: x := 123
 * Ожидание: LiteralExpr с kind=Int и intValue=123.
 */
TEST_F(BisonParserExprTest, CorrectlyParsesIntegerLiteral) {
    auto mod = parseModule("x := 123");
    auto* expr = getFirstAssignmentExpr(mod.get());

    auto* lit = dynamic_cast<LiteralExpr*>(expr);
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::Int);
    EXPECT_EQ(lit->intValue, 123);
}

/**
 * Проверяет корректный разбор вещественного литерала.
 * Вход: x := 2.718
 * Ожидание: LiteralExpr с kind=Real и realValue=2.718.
 */
TEST_F(BisonParserExprTest, ParsesRealLiteral) {
    auto mod = parseModule("x := 2.718");
    auto* expr = getFirstAssignmentExpr(mod.get());

    auto* lit = dynamic_cast<LiteralExpr*>(expr);
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::Real);
    EXPECT_DOUBLE_EQ(lit->realValue, 2.718);
}

/**
 * Проверяет корректный разбор строкового литерала.
 * Вход: x := "hello"
 * Ожидание: LiteralExpr с kind=String и strValue="hello".
 */
TEST_F(BisonParserExprTest, ParsesStringLiteral) {
    auto mod = parseModule("x := \"hello\"");
    auto* expr = getFirstAssignmentExpr(mod.get());

    auto* lit = dynamic_cast<LiteralExpr*>(expr);
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::String);
    EXPECT_EQ(lit->strValue, "hello");
}

/**
 * Проверяет разбор булевых литералов TRUE и FALSE.
 * Вход: x := TRUE; x := FALSE
 * Ожидание: LiteralExpr с kind=Bool, boolValue=true/false.
 */
TEST_F(BisonParserExprTest, ParsesBoolLiterals) {
    auto mod = parseModule("x := TRUE; x := FALSE");
    ASSERT_GE(mod->block.size(), 2);

    auto* assign1 = dynamic_cast<AssignStmt*>(mod->block[0].get());
    auto* lit1 = dynamic_cast<LiteralExpr*>(assign1->rhs.get());
    ASSERT_NE(lit1, nullptr);
    EXPECT_EQ(lit1->kind, LiteralExpr::Kind::Bool);
    EXPECT_TRUE(lit1->boolValue);

    auto* assign2 = dynamic_cast<AssignStmt*>(mod->block[1].get());
    auto* lit2 = dynamic_cast<LiteralExpr*>(assign2->rhs.get());
    ASSERT_NE(lit2, nullptr);
    EXPECT_EQ(lit2->kind, LiteralExpr::Kind::Bool);
    EXPECT_FALSE(lit2->boolValue);
}

/**
 * Проверяет разбор литерала NIL.
 * Вход: x := NIL
 * Ожидание: LiteralExpr с kind=Nil.
 */
TEST_F(BisonParserExprTest, ParsesNilLiteral) {
    auto mod = parseModule("x := NIL");
    auto* expr = getFirstAssignmentExpr(mod.get());

    auto* lit = dynamic_cast<LiteralExpr*>(expr);
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->kind, LiteralExpr::Kind::Nil);
}

/**
 * Проверяет приоритет операций: умножение выше сложения.
 * Вход: x := 2 + 3 * 4
 * Ожидание: корень — Add(2, Mul(3, 4)), т.е. 3*4 группируется первым.
 */
TEST_F(BisonParserExprTest, RespectsPrecedenceMulOverAdd) {
    auto mod = parseModule("x := 2 + 3 * 4");
    auto* root = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->op, BinaryExpr::Op::Add);

    auto* lhs = dynamic_cast<LiteralExpr*>(root->lhs.get());
    auto* rhs = dynamic_cast<BinaryExpr*>(root->rhs.get());

    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->intValue, 2);

    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(rhs->op, BinaryExpr::Op::Mul);

    auto* r_lhs = dynamic_cast<LiteralExpr*>(rhs->lhs.get());
    auto* r_rhs = dynamic_cast<LiteralExpr*>(rhs->rhs.get());
    EXPECT_EQ(r_lhs->intValue, 3);
    EXPECT_EQ(r_rhs->intValue, 4);
}

/**
 * Проверяет, что скобки переопределяют приоритет операций.
 * Вход: x := (2 + 3) * 4
 * Ожидание: корень — Mul(Add(2,3), 4).
 */
TEST_F(BisonParserExprTest, RespectsParentheses) {
    auto mod = parseModule("x := (2 + 3) * 4");
    auto* root = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->op, BinaryExpr::Op::Mul);

    auto* lhs = dynamic_cast<BinaryExpr*>(root->lhs.get());
    auto* rhs = dynamic_cast<LiteralExpr*>(root->rhs.get());

    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->op, BinaryExpr::Op::Add);
    EXPECT_EQ(rhs->intValue, 4);
}

/**
 * Проверяет унарный минус (отрицание).
 * Вход: x := -5
 * Ожидание: UnaryExpr с op=Neg, rhs=LiteralExpr(5).
 */
TEST_F(BisonParserExprTest, ParsesUnaryNegation) {
    auto mod = parseModule("x := -5");
    auto* expr = getFirstAssignmentExpr(mod.get());

    auto* unary = dynamic_cast<UnaryExpr*>(expr);
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op, UnaryExpr::Op::Neg);

    auto* val = dynamic_cast<LiteralExpr*>(unary->rhs.get());
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->intValue, 5);
}

/**
 * Проверяет логическое отрицание (~).
 * Вход: x := ~x
 * Ожидание: UnaryExpr с op=Not, rhs=DesignatorExpr("x").
 */
TEST_F(BisonParserExprTest, ParsesLogicalNot) {
    auto mod = parseModule("x := ~x");
    auto* expr = getFirstAssignmentExpr(mod.get());

    auto* unary = dynamic_cast<UnaryExpr*>(expr);
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op, UnaryExpr::Op::Not);

    auto* operand = dynamic_cast<DesignatorExpr*>(unary->rhs.get());
    ASSERT_NE(operand, nullptr);
    EXPECT_EQ(operand->baseName, "x");
}

/**
 * Проверяет разбор конструктора множества с диапазонами.
 * Вход: x := {1, 3..7}
 * Ожидание: SetExpr с 2 элементами — одиночный 1 и диапазон 3..7.
 */
TEST_F(BisonParserExprTest, ParsesSetExpression) {
    auto mod = parseModule("x := {1, 3..7}");
    auto* expr = getFirstAssignmentExpr(mod.get());

    auto* set = dynamic_cast<SetExpr*>(expr);
    ASSERT_NE(set, nullptr);
    ASSERT_EQ(set->elements.size(), 2);

    auto* e1 = dynamic_cast<LiteralExpr*>(set->elements[0].low.get());
    ASSERT_NE(e1, nullptr);
    EXPECT_EQ(e1->intValue, 1);
    EXPECT_EQ(set->elements[0].high, nullptr);

    auto* e2low = dynamic_cast<LiteralExpr*>(set->elements[1].low.get());
    auto* e2high = dynamic_cast<LiteralExpr*>(set->elements[1].high.get());
    ASSERT_NE(e2low, nullptr);
    ASSERT_NE(e2high, nullptr);
    EXPECT_EQ(e2low->intValue, 3);
    EXPECT_EQ(e2high->intValue, 7);
}

/**
 * Проверяет разбор пустого множества {}.
 * Ожидание: SetExpr с 0 элементов.
 */
TEST_F(BisonParserExprTest, ParsesEmptySet) {
    auto mod = parseModule("x := {}");
    auto* expr = getFirstAssignmentExpr(mod.get());

    auto* set = dynamic_cast<SetExpr*>(expr);
    ASSERT_NE(set, nullptr);
    EXPECT_EQ(set->elements.size(), 0);
}

/**
 * Проверяет оператор DIV (целочисленное деление).
 * Вход: x := 10 DIV 3
 * Ожидание: BinaryExpr с op=IDiv.
 */
TEST_F(BisonParserExprTest, ParsesDivOperator) {
    auto mod = parseModule("x := 10 DIV 3");
    auto* expr = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));

    ASSERT_NE(expr, nullptr);
    EXPECT_EQ(expr->op, BinaryExpr::Op::IDiv);
}

/**
 * Проверяет оператор MOD (остаток от деления).
 * Вход: x := 10 MOD 3
 * Ожидание: BinaryExpr с op=Mod.
 */
TEST_F(BisonParserExprTest, ParsesModOperator) {
    auto mod = parseModule("x := 10 MOD 3");
    auto* expr = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));

    ASSERT_NE(expr, nullptr);
    EXPECT_EQ(expr->op, BinaryExpr::Op::Mod);
}

/**
 * Проверяет логическое «И» (&) и «ИЛИ» (OR).
 * Вход: x := x & x OR x
 * & имеет приоритет выше OR, поэтому ожидается: OR(And(x, x), x).
 */
TEST_F(BisonParserExprTest, ParsesLogicalAndOr) {
    auto mod = parseModule("x := x & x OR x");
    auto* root = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->op, BinaryExpr::Op::Or);

    auto* lhs = dynamic_cast<BinaryExpr*>(root->lhs.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->op, BinaryExpr::Op::And);
}

/**
 * Проверяет операторы сравнения: =, #, <, <=, >, >=.
 * Каждый оператор порождает BinaryExpr с соответствующим Op.
 */
TEST_F(BisonParserExprTest, ParsesRelationalOperators) {
    struct RelCase { const char* code; BinaryExpr::Op op; };
    RelCase cases[] = {
        {"x := x = x",  BinaryExpr::Op::Eq},
        {"x := x # x",  BinaryExpr::Op::Neq},
        {"x := x < x",  BinaryExpr::Op::Lt},
        {"x := x <= x", BinaryExpr::Op::Le},
        {"x := x > x",  BinaryExpr::Op::Gt},
        {"x := x >= x", BinaryExpr::Op::Ge},
    };

    for (auto& c : cases) {
        auto m = parseModule(c.code);
        auto* expr = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(m.get()));
        ASSERT_NE(expr, nullptr) << "Failed for: " << c.code;
        EXPECT_EQ(expr->op, c.op) << "Wrong op for: " << c.code;
    }
}

/**
 * Проверяет левоассоциативность сложения.
 * Вход: x := 1 + 2 + 3
 * Ожидание: Add(Add(1, 2), 3) — левый узел группируется первым.
 */
TEST_F(BisonParserExprTest, LeftAssociativityOfAddition) {
    auto mod = parseModule("x := 1 + 2 + 3");
    auto* root = dynamic_cast<BinaryExpr*>(getFirstAssignmentExpr(mod.get()));

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->op, BinaryExpr::Op::Add);

    auto* lhs = dynamic_cast<BinaryExpr*>(root->lhs.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->op, BinaryExpr::Op::Add);

    auto* rhs = dynamic_cast<LiteralExpr*>(root->rhs.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(rhs->intValue, 3);
}

/**
 * Проверяет разбор цепочки селекторов у designator.
 * Вход: x := a.b[10]^.c
 * «a.b» — qualident (baseName="a.b"), далее 3 селектора:
 *   [0] IndexSelector  — индексация [10]
 *   [1] DerefSelector   — разыменование ^
 *   [2] FieldSelector   — доступ к полю .c
 */
TEST_F(BisonParserExprTest, ParsesDesignatorChain) {
    auto mod = parseModule("x := a.b[10]^.c");

    auto* des = dynamic_cast<DesignatorExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(des, nullptr);
    EXPECT_EQ(des->baseName, "a.b");

    ASSERT_EQ(des->selectors.size(), 3);

    auto* s1 = dynamic_cast<IndexSelector*>(des->selectors[0].get());
    ASSERT_NE(s1, nullptr);
    ASSERT_EQ(s1->index.size(), 1);
    auto* idxVal = dynamic_cast<LiteralExpr*>(s1->index[0].get());
    EXPECT_EQ(idxVal->intValue, 10);

    auto* s2 = dynamic_cast<DerefSelector*>(des->selectors[1].get());
    ASSERT_NE(s2, nullptr);

    auto* s3 = dynamic_cast<FieldSelector*>(des->selectors[2].get());
    ASSERT_NE(s3, nullptr);
    EXPECT_EQ(s3->name, "c");
}

/**
 * Проверяет разбор вызова функции как DesignatorExpr + ArgsSelector.
 * Вход: x := Math.Sin(3.14)
 * Ожидание: DesignatorExpr с baseName="Math.Sin" и одним ArgsSelector,
 * содержащим один аргумент — вещественный литерал 3.14.
 */
TEST_F(BisonParserExprTest, ParsesFunctionCall) {
    auto mod = parseModule("x := Math.Sin(3.14)");

    auto* des = dynamic_cast<DesignatorExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(des, nullptr);
    EXPECT_EQ(des->baseName, "Math.Sin");

    ASSERT_EQ(des->selectors.size(), 1);
    auto* args = dynamic_cast<ArgsSelector*>(des->selectors[0].get());
    ASSERT_NE(args, nullptr);

    ASSERT_EQ(args->args.size(), 1);
    auto* arg = dynamic_cast<LiteralExpr*>(args->args[0].get());
    ASSERT_NE(arg, nullptr);
    EXPECT_DOUBLE_EQ(arg->realValue, 3.14);
}

/**
 * Проверяет вызов функции без аргументов: f().
 * Ожидание: ArgsSelector с пустым вектором args.
 */
TEST_F(BisonParserExprTest, ParsesFunctionCallNoArgs) {
    auto mod = parseModule("x := f()");

    auto* des = dynamic_cast<DesignatorExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(des, nullptr);
    EXPECT_EQ(des->baseName, "f");

    ASSERT_EQ(des->selectors.size(), 1);
    auto* args = dynamic_cast<ArgsSelector*>(des->selectors[0].get());
    ASSERT_NE(args, nullptr);
    EXPECT_EQ(args->args.size(), 0);
}

/**
 * Проверяет вызов функции с несколькими аргументами.
 * Вход: x := Add(1, 2, 3)
 * Ожидание: ArgsSelector с 3 аргументами.
 */
TEST_F(BisonParserExprTest, ParsesFunctionCallMultipleArgs) {
    auto mod = parseModule("x := Add(1, 2, 3)");

    auto* des = dynamic_cast<DesignatorExpr*>(getFirstAssignmentExpr(mod.get()));
    ASSERT_NE(des, nullptr);

    ASSERT_EQ(des->selectors.size(), 1);
    auto* args = dynamic_cast<ArgsSelector*>(des->selectors[0].get());
    ASSERT_NE(args, nullptr);
    ASSERT_EQ(args->args.size(), 3);

    auto* a1 = dynamic_cast<LiteralExpr*>(args->args[0].get());
    auto* a2 = dynamic_cast<LiteralExpr*>(args->args[1].get());
    auto* a3 = dynamic_cast<LiteralExpr*>(args->args[2].get());
    EXPECT_EQ(a1->intValue, 1);
    EXPECT_EQ(a2->intValue, 2);
    EXPECT_EQ(a3->intValue, 3);
}

} // namespace
