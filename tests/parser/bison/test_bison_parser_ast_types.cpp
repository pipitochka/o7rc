#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include <parser/impl/bison/BisonParser.h>
#include <tokenizer/impl/flex/FlexTokenizer.h>
#include <util/ast/Ast.h>

namespace {

/**
 * Набор тестов для проверки построения AST объявлений типов и переменных.
 * Каждый тест оборачивает объявления в минимальный модуль:
 * MODULE Test; <decls> END Test.
 */
class BisonParserTypeTest : public ::testing::Test {
protected:
    /**
     * Оборачивает секцию объявлений в модуль, запускает лексер и парсер
     * и возвращает корневой узел AST (Module).
     */
    std::unique_ptr<Module> parseDecls(const std::string& decls) {
        std::stringstream ss("MODULE Test; " + decls + " END Test.");
        auto tokenizer = std::make_unique<FlexTokenizer>(ss);
        BisonParser parser;
        return parser.parse(std::move(tokenizer));
    }

    /**
     * Возвращает объявление (Decl) по индексу из списка mod->decls.
     * Возвращает nullptr при выходе за границы.
     */
    Decl* getDecl(Module* mod, int index) {
        if (!mod || index >= mod->decls.size()) return nullptr;
        return mod->decls[index].get();
    }
};

/**
 * Проверяет разбор одномерного массива.
 * Вход: VAR a: ARRAY 10 OF INTEGER;
 * Ожидание: VarDecl -> ArrayType с length=[10] и elemType=NamedType("INTEGER").
 */
TEST_F(BisonParserTypeTest, ParsesArrayType) {
    auto mod = parseDecls("VAR a: ARRAY 10 OF INTEGER;");

    auto* varDecl = dynamic_cast<VarDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(varDecl, nullptr);

    auto* arrType = dynamic_cast<ArrayType*>(varDecl->type.get());
    ASSERT_NE(arrType, nullptr);

    ASSERT_EQ(arrType->length.size(), 1);
    auto* len = dynamic_cast<LiteralExpr*>(arrType->length[0].get());
    EXPECT_EQ(len->intValue, 10);

    auto* elem = dynamic_cast<NamedType*>(arrType->elemType.get());
    EXPECT_EQ(elem->name, "INTEGER");
}

/**
 * Проверяет разбор многомерного массива (ARRAY 5, 8 OF CHAR).
 * Ожидание: ArrayType с length=[5, 8].
 */
TEST_F(BisonParserTypeTest, ParsesMultiDimArray) {
    auto mod = parseDecls("VAR m: ARRAY 5, 8 OF CHAR;");
    auto* varDecl = dynamic_cast<VarDecl*>(getDecl(mod.get(), 0));
    auto* arrType = dynamic_cast<ArrayType*>(varDecl->type.get());

    ASSERT_EQ(arrType->length.size(), 2);
    auto* l1 = dynamic_cast<LiteralExpr*>(arrType->length[0].get());
    auto* l2 = dynamic_cast<LiteralExpr*>(arrType->length[1].get());

    EXPECT_EQ(l1->intValue, 5);
    EXPECT_EQ(l2->intValue, 8);
}

/**
 * Проверяет разбор записи (RECORD) с несколькими группами полей.
 * Вход: TYPE Point = RECORD x, y: INTEGER; visible: BOOLEAN END;
 * Ожидание: RecordType с 2-мя FieldDecl:
 *   fields[0] — имена [x, y], fields[1] — имя [visible].
 */
TEST_F(BisonParserTypeTest, ParsesRecordType) {
    auto mod = parseDecls("TYPE Point = RECORD x, y: INTEGER; visible: BOOLEAN END;");

    auto* typeDecl = dynamic_cast<TypeDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(typeDecl, nullptr);
    EXPECT_EQ(typeDecl->name, "Point");

    auto* rec = dynamic_cast<RecordType*>(typeDecl->type.get());
    ASSERT_NE(rec, nullptr);
    ASSERT_EQ(rec->fields.size(), 2); // (x,y) и (visible)

    EXPECT_EQ(rec->fields[0]->names.size(), 2);
    EXPECT_EQ(rec->fields[0]->names[0], "x");

    EXPECT_EQ(rec->fields[1]->names[0], "visible");
}

/**
 * Проверяет разбор записи с базовым типом (расширение типа).
 * Вход: TYPE Point3D = RECORD(Point) z: INTEGER END;
 * Ожидание: RecordType с baseType=NamedType("Point") и одним полем z.
 */
TEST_F(BisonParserTypeTest, ParsesRecordWithBaseType) {
    auto mod = parseDecls(
        "TYPE Point = RECORD x, y: INTEGER END; "
        "Point3D = RECORD(Point) z: INTEGER END;"
    );

    auto* typeDecl = dynamic_cast<TypeDecl*>(getDecl(mod.get(), 1));
    ASSERT_NE(typeDecl, nullptr);
    EXPECT_EQ(typeDecl->name, "Point3D");

    auto* rec = dynamic_cast<RecordType*>(typeDecl->type.get());
    ASSERT_NE(rec, nullptr);

    auto* base = dynamic_cast<NamedType*>(rec->baseType.get());
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->name, "Point");

    ASSERT_EQ(rec->fields.size(), 1);
    EXPECT_EQ(rec->fields[0]->names[0], "z");
}

/**
 * Проверяет разбор типа-указателя (POINTER TO).
 * Вход: TYPE NodePtr = POINTER TO Node;
 * Ожидание: TypeDecl -> PointerType с baseType=NamedType("Node").
 */
TEST_F(BisonParserTypeTest, ParsesPointerType) {
    auto mod = parseDecls("TYPE NodePtr = POINTER TO Node;");

    auto* typeDecl = dynamic_cast<TypeDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(typeDecl, nullptr);
    EXPECT_EQ(typeDecl->name, "NodePtr");

    auto* ptr = dynamic_cast<PointerType*>(typeDecl->type.get());
    ASSERT_NE(ptr, nullptr);

    auto* base = dynamic_cast<NamedType*>(ptr->baseType.get());
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->name, "Node");
}

/**
 * Проверяет разбор процедуры с VAR-параметрами (передача по ссылке).
 * Вход: PROCEDURE Swap(VAR a, b: INTEGER); BEGIN END Swap;
 * Ожидание: ProcDecl -> ProcType с одной секцией параметров,
 * у которой isVar=true и имена [a, b].
 */
TEST_F(BisonParserTypeTest, ParsesProcedureWithVarParams) {
    auto mod = parseDecls("PROCEDURE Swap(VAR a, b: INTEGER); BEGIN END Swap;");

    auto* proc = dynamic_cast<ProcDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(proc, nullptr);

    auto* sig = dynamic_cast<ProcType*>(proc->type.get());
    ASSERT_NE(sig, nullptr);
    ASSERT_EQ(sig->params.size(), 1); // Одна секция параметров

    auto* paramSection = sig->params[0].get();
    EXPECT_TRUE(paramSection->isVar) << "Failed to detect VAR parameter section";
    EXPECT_EQ(paramSection->names[0], "a");
    EXPECT_EQ(paramSection->names[1], "b");
}

/**
 * Проверяет разбор процедуры с возвращаемым типом.
 * Вход: PROCEDURE Max(a, b: INTEGER): INTEGER; BEGIN RETURN a END Max;
 * Ожидание: ProcType с returnType=NamedType("INTEGER").
 */
TEST_F(BisonParserTypeTest, ParsesProcedureWithReturnType) {
    auto mod = parseDecls(
        "PROCEDURE Max(a, b: INTEGER): INTEGER; BEGIN RETURN a END Max;"
    );

    auto* proc = dynamic_cast<ProcDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(proc, nullptr);

    auto* sig = dynamic_cast<ProcType*>(proc->type.get());
    ASSERT_NE(sig, nullptr);

    ASSERT_NE(sig->type, nullptr);
    EXPECT_EQ(sig->type->name, "INTEGER");
}

/**
 * Проверяет разбор процедуры с несколькими секциями параметров
 * (обычные и VAR, разделённые ;).
 * Вход: PROCEDURE P(a: INTEGER; VAR b: BOOLEAN; c, d: REAL);
 * Ожидание: ProcType с 3 секциями: [a:INT], [VAR b:BOOL], [c,d:REAL].
 */
TEST_F(BisonParserTypeTest, ParsesMultipleParamSections) {
    auto mod = parseDecls(
        "PROCEDURE P(a: INTEGER; VAR b: BOOLEAN; c, d: REAL); BEGIN END P;"
    );

    auto* proc = dynamic_cast<ProcDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(proc, nullptr);

    auto* sig = dynamic_cast<ProcType*>(proc->type.get());
    ASSERT_NE(sig, nullptr);
    ASSERT_EQ(sig->params.size(), 3);

    EXPECT_FALSE(sig->params[0]->isVar);
    EXPECT_EQ(sig->params[0]->names.size(), 1);
    EXPECT_EQ(sig->params[0]->names[0], "a");

    EXPECT_TRUE(sig->params[1]->isVar);
    EXPECT_EQ(sig->params[1]->names[0], "b");

    EXPECT_FALSE(sig->params[2]->isVar);
    EXPECT_EQ(sig->params[2]->names.size(), 2);
    EXPECT_EQ(sig->params[2]->names[0], "c");
    EXPECT_EQ(sig->params[2]->names[1], "d");
}

/**
 * Проверяет разбор типа-процедуры (PROCEDURE TYPE).
 * Вход: TYPE BinaryOp = PROCEDURE(a, b: INTEGER): INTEGER;
 * Ожидание: TypeDecl -> ProcType с 1 секцией [a,b:INT] и returnType=INTEGER.
 */
TEST_F(BisonParserTypeTest, ParsesProcedureType) {
    auto mod = parseDecls("TYPE BinaryOp = PROCEDURE(a, b: INTEGER): INTEGER;");

    auto* typeDecl = dynamic_cast<TypeDecl*>(getDecl(mod.get(), 0));
    ASSERT_NE(typeDecl, nullptr);
    EXPECT_EQ(typeDecl->name, "BinaryOp");

    auto* procType = dynamic_cast<ProcType*>(typeDecl->type.get());
    ASSERT_NE(procType, nullptr);

    ASSERT_EQ(procType->params.size(), 1);
    EXPECT_EQ(procType->params[0]->names.size(), 2);

    ASSERT_NE(procType->type, nullptr);
    EXPECT_EQ(procType->type->name, "INTEGER");
}

} // namespace
