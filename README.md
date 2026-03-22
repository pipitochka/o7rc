# o7rc

**o7rc** — кросс-компилятор языка Oberon-7 в ассемблер RISC-V (RV32IM), совместимый с симулятором [RARS](https://github.com/TheThirdOne/rars).

## Оглавление
- [Функционал](#функционал)
- [Зависимости](#зависимости)
- [Сборка](#сборка)
- [Запуск](#запуск)
- [Тестирование](#тестирование)
- [Docker](#docker)
- [Синтаксис Oberon-7](#синтаксис-oberon-7)

---

## Функционал

| Компонент | Описание |
|---|---|
| **Лексер** | Flex — токенизация исходного кода Oberon-7 |
| **Парсер** | Bison — построение AST по грамматике Oberon-7 |
| **Кодогенерация** | Генерация RISC-V ассемблера (RV32IM) из AST |
| **E2E-тесты** | Сравнение вывода o7rc+RARS с эталонным компилятором OBNC |

---

## Зависимости

### Обязательные

| Пакет | Версия | Назначение |
|---|---|---|
| C++ компилятор | C++17 | GCC >= 9 или Clang >= 10 |
| CMake | >= 3.20 | Система сборки |
| Flex | любая | Генерация лексера |
| Bison | >= 3.0 | Генерация парсера |

### Для E2E-тестов (опционально)

| Пакет | Версия | Назначение |
|---|---|---|
| Java (JRE) | >= 8 | Запуск RARS |
| libgc-dev / bdw-gc | любая | Зависимость OBNC (Boehm GC) |

RARS и OBNC скачиваются и собираются автоматически при сборке с `-DUSE_RARS=ON`.

### Установка

**macOS:**
```bash
xcode-select --install
brew install bison flex cmake
# Для e2e-тестов:
brew install openjdk bdw-gc
```

**Ubuntu / Debian:**
```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential cmake flex bison libfl-dev
# Для e2e-тестов:
sudo apt-get install -y default-jre-headless libgc-dev
```

---

## Сборка

```bash
mkdir build && cd build
cmake ..
cmake --build . --parallel
```

### Опции CMake

| Опция | По умолчанию | Описание |
|---|---|---|
| `USE_FLEX` | `ON` | Использовать Flex-лексер |
| `USE_BISON` | `ON` | Использовать Bison-парсер |
| `USE_CODEGEN` | `ON` | Включить кодогенерацию RISC-V |
| `USE_RARS` | `OFF` | Скачать RARS и включить e2e-тесты |
| `USE_DEBUG` | `OFF` | Дополнительный отладочный вывод |

Пример сборки со всеми тестами:

```bash
cmake .. -DUSE_RARS=ON
cmake --build . --parallel
```

---

## Запуск

```bash
# Компиляция .obr → .asm
./o7rc input.obr -o output.asm

# Запуск в RARS
java -jar rars.jar nc sm output.asm
```

---

## Тестирование

### Юнит-тесты (Google Test)

```bash
cd build
ctest -E e2e --output-on-failure
# или напрямую:
./o7rc_tests
```

### E2E-тесты (OBNC vs o7rc+RARS)

Требуется Java, libgc-dev и сборка с `-DUSE_RARS=ON`:

```bash
cd build
ctest -L e2e --output-on-failure
```

Каждый e2e-тест:
1. Компилирует `.obr` через OBNC → запускает нативный бинарник → эталонный вывод
2. Компилирует `.obr` → `.asm` через `o7rc` → запускает в RARS → фактический вывод
3. Сравнивает вывод OBNC и o7rc+RARS

Тестовые программы находятся в `tests/e2e/programs/`.

### Все тесты сразу

```bash
ctest --test-dir build --output-on-failure
```

---

## Docker

```bash
# Сборка образа
docker build -t o7rc .

# Запуск контейнера
docker run -it --rm -v "$(pwd)":/work o7rc

# Внутри контейнера:
mkdir build && cd build
cmake .. -DUSE_RARS=ON && cmake --build . --parallel
ctest --output-on-failure
```

---

## Синтаксис Oberon-7

```ebnf
letter     = "A" | "B" | … | "Z" | "a" | "b" | … | "z".
digit      = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9".
hexDigit   = digit | "A" | "B" | "C" | "D" | "E" | "F".

ident      = letter {letter | digit}.
qualident  = [ident "."] ident.
identdef   = ident ["*"].

integer    = digit {digit} | digit {hexDigit} "H".
real       = digit {digit} "." {digit} [ScaleFactor].
ScaleFactor = "E" ["+" | "-"] digit {digit}.
number     = integer | real.
string     = """ {character} """ | digit {hexDigit} "X".

ConstDeclaration = identdef "=" ConstExpression.
ConstExpression  = expression.

TypeDeclaration   = identdef "=" type.
type              = qualident | ArrayType | RecordType | PointerType | ProcedureType.
ArrayType         = ARRAY length {"," length} OF type.
length            = ConstExpression.
RecordType        = RECORD ["(" BaseType ")"] [FieldListSequence] END.
BaseType          = qualident.
FieldListSequence = FieldList {";" FieldList}.
FieldList         = IdentList ":" type.
IdentList         = identdef {"," identdef}.
PointerType       = POINTER TO type.
ProcedureType     = PROCEDURE [FormalParameters].

VariableDeclaration = IdentList ":" type.

expression        = SimpleExpression [relation SimpleExpression].
relation          = "=" | "#" | "<" | "<=" | ">" | ">=" | IN | IS.
SimpleExpression  = ["+" | "-"] term {AddOperator term}.
AddOperator       = "+" | "-" | OR.
term              = factor {MulOperator factor}.
MulOperator       = "*" | "/" | DIV | MOD | "&".
factor            = number | string | NIL | TRUE | FALSE |
                    set | designator [ActualParameters] | "(" expression ")" | "~" factor.
designator        = qualident {selector}.
selector          = "." ident | "[" ExpList "]" | "^" | "(" qualident ")".
set               = "{" [element {"," element}] "}".
element           = expression [".." expression].
ExpList           = expression {"," expression}.
ActualParameters  = "(" [ExpList] ")" .

statement         = [assignment | ProcedureCall | IfStatement | CaseStatement |
                     WhileStatement | RepeatStatement | ForStatement].
assignment        = designator ":=" expression.
ProcedureCall     = designator [ActualParameters].
StatementSequence = statement {";" statement}.

IfStatement       = IF expression THEN StatementSequence
                    {ELSIF expression THEN StatementSequence}
                    [ELSE StatementSequence] END.

CaseStatement     = CASE expression OF case {"|" case} END.
case              = [CaseLabelList ":" StatementSequence].
CaseLabelList     = LabelRange {"," LabelRange}.
LabelRange        = label [".." label].
label             = integer | string | qualident.

WhileStatement    = WHILE expression DO StatementSequence
                    {ELSIF expression DO StatementSequence} END.

RepeatStatement   = REPEAT StatementSequence UNTIL expression.

ForStatement      = FOR ident ":=" expression TO expression [BY ConstExpression]
                    DO StatementSequence END.

ProcedureDeclaration = ProcedureHeading ";" ProcedureBody ident.
ProcedureHeading   = PROCEDURE identdef [FormalParameters].
ProcedureBody      = DeclarationSequence [BEGIN StatementSequence]
                     [RETURN expression] END.

DeclarationSequence = [CONST {ConstDeclaration ";"}]
                      [TYPE {TypeDeclaration ";"}]
                      [VAR {VariableDeclaration ";"}]
                      {ProcedureDeclaration ";"}.

FormalParameters   = "(" [FPSection {";" FPSection}] ")" [":" qualident].
FPSection          = [VAR] ident {"," ident} ":" FormalType.
FormalType         = {ARRAY OF} qualident.

module             = MODULE ident ";" [ImportList] DeclarationSequence
                     [BEGIN StatementSequence] END ident "." .
ImportList         = IMPORT import {"," import} ";".
import             = ident [":=" ident].
```
