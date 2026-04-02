# o7rc

**o7rc** — учебный кросс-компилятор языка Oberon-7 в ассемблер RISC-V (RV32IM), совместимый с симулятором [RARS](https://github.com/TheThirdOne/rars).

Компилятор спроектирован для демонстрации основных этапов компиляции: лексический анализ, синтаксический анализ, построение AST, трансформация в промежуточное представление (IR) с графом потока управления (CFG), оптимизации и кодогенерация.

## Оглавление
- [Архитектура](#архитектура)
- [Docker](#docker)
- [Зависимости](#зависимости)
- [Сборка](#сборка)
- [Запуск](#запуск)
- [IR и оптимизации](#ir-и-оптимизации)
- [Тестирование](#тестирование)
- [CI/CD](#cicd)
- [Структура проекта](#структура-проекта)
- [Синтаксис Oberon-7](#синтаксис-oberon-7)

---

## Архитектура

```
                          ┌──────────────┐
                          │  .obr файл   │
                          └──────┬───────┘
                                 │
                   ┌─────────────┴─────────────┐
                   │         Лексер             │
                   │   (Flex или рукописный)    │
                   └─────────────┬─────────────┘
                                 │  поток токенов
                   ┌─────────────┴─────────────┐
                   │         Парсер             │
                   │  (Bison или рукописный)    │
                   └─────────────┬─────────────┘
                                 │  AST
                     ┌───────────┴───────────┐
                     │                       │
              ┌──────┴──────┐         ┌──────┴──────┐
              │  Прямая     │         │  IRBuilder  │
              │  кодоген.   │         │  AST → IR   │
              │  (AST→asm)  │         └──────┬──────┘
              └──────┬──────┘                │  IR / CFG
                     │                ┌──────┴──────┐
                     │                │ PassManager  │
                     │                │ (оптимизации)│
                     │                └──────┬──────┘
                     │                       │  IR / CFG
                     │                ┌──────┴──────┐
                     │                │ RiscVIRCode  │
                     │                │ Gen (IR→asm) │
                     │                └──────┬──────┘
                     └───────────┬───────────┘
                                 │
                          ┌──────┴───────┐
                          │  .asm файл   │
                          │  (RISC-V)    │
                          └──────────────┘
```

| Компонент | Описание |
|---|---|
| **Лексер (Flex)** | Токенизация исходного кода Oberon-7 (генерируемый) |
| **Лексер (Hand)** | Токенизация исходного кода Oberon-7 (рукописный) |
| **Парсер (Bison)** | Построение AST по грамматике Oberon-7 (генерируемый) |
| **Парсер (Hand)** | Рекурсивный спуск, построение AST (рукописный) |
| **IR / CFG** | Трёхадресный код в базовых блоках с графом потока управления |
| **PassManager** | Управление проходами оптимизации с fluent API |
| **Кодогенерация (прямая)** | Генерация RISC-V ассемблера напрямую из AST |
| **Кодогенерация (IR)** | Генерация RISC-V ассемблера из оптимизированного IR |
| **E2E-тесты** | Автоматическое сравнение вывода o7rc+RARS с эталонным компилятором OBNC |

---

## Docker

```bash
docker build -t o7rc .

docker run -it --rm -v "$(pwd)":/work o7rc

# Внутри контейнера:
mkdir build && cd build
cmake .. && cmake --build . --parallel
ctest --output-on-failure
```

---

## Зависимости

### Обязательные

| Пакет | Версия | Назначение |
|---|---|---|
| C++ компилятор | C++17 | GCC >= 9 или Clang >= 10 |
| CMake | >= 3.20 | Система сборки |

### Опциональные (фронтенд)

| Пакет | Назначение |
|---|---|
| Flex | Генерация лексера (при `USE_FLEX=ON`, по умолчанию) |
| Bison >= 3.0 | Генерация парсера (при `USE_BISON=ON`, по умолчанию) |

> Flex и Bison **не требуются**, если используются рукописные реализации (`USE_HAND_TOKENIZER=ON`, `USE_HAND_PARSER=ON`).

### Для E2E-тестов

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
| `USE_HAND_TOKENIZER` | `OFF` | Использовать рукописный лексер |
| `USE_HAND_PARSER` | `OFF` | Использовать рукописный парсер |
| `USE_CODEGEN` | `ON` | Включить прямую кодогенерацию RISC-V (AST → asm) |
| `USE_RARS` | `ON` | Скачать RARS/OBNC и включить e2e-тесты |
| `USE_DEBUG` | `OFF` | Отладочный вывод токенов |

Должен быть включён хотя бы один токенизатор (`USE_FLEX` или `USE_HAND_TOKENIZER`) и хотя бы один парсер (`USE_BISON` или `USE_HAND_PARSER`).

**Примеры конфигураций:**

```bash
# По умолчанию (Flex + Bison)
cmake ..

# Только рукописные реализации (без Flex/Bison)
cmake .. -DUSE_FLEX=OFF -DUSE_BISON=OFF \
         -DUSE_HAND_TOKENIZER=ON -DUSE_HAND_PARSER=ON

# Все 4 комбинации (для полного тестирования)
cmake .. -DUSE_HAND_TOKENIZER=ON -DUSE_HAND_PARSER=ON
```

---

## Запуск

```bash
# Компиляция через прямую кодогенерацию (AST → asm)
./o7rc input.obr -o output.asm

# Компиляция через IR-пайплайн
./o7rc input.obr -o output.asm --ir

# IR-пайплайн с оптимизациями
./o7rc input.obr -o output.asm --opt

# Запуск в RARS
java -jar rars.jar nc sm output.asm
```

### Все флаги CLI

| Флаг | Описание |
|---|---|
| `-o <file>` | Путь к выходному `.asm` файлу |
| `--tokenizer <flex\|hand>` | Выбор лексера (если собрано несколько) |
| `--parser <bison\|hand>` | Выбор парсера (если собрано несколько) |
| `--ir` | Кодогенерация через IR (вместо прямого AST → asm) |
| `--opt` | Включить оптимизации (подразумевает `--ir`) |
| `--dump-ir` | Напечатать IR в stderr (до и после оптимизаций) |
| `--dump-ir-passes` | Напечатать IR после каждого прохода |
| `--dump-dot <file>` | Экспортировать CFG в формате Graphviz DOT |

Флаги `--opt`, `--dump-ir`, `--dump-ir-passes` и `--dump-dot` автоматически включают IR-пайплайн. Флаги `--tokenizer` и `--parser` имеют смысл только когда собрано несколько реализаций одновременно; по умолчанию используются Flex / Bison.

---

## IR и оптимизации

Промежуточное представление — трёхадресный код (TAC) в базовых блоках, образующих граф потока управления (CFG).

### Пайплайн

```
AST → IRBuilder → IR/CFG → PassManager (оптимизации) → RiscVIRCodeGen → .asm
```

### Примеры использования

```bash
# Дамп IR + визуализация CFG
./o7rc input.obr -o out.asm --dump-ir --dump-dot cfg.dot
dot -Tpng cfg.dot -o cfg.png

# Оптимизации с пошаговым дампом каждого прохода
./o7rc input.obr -o out.asm --opt --dump-ir --dump-ir-passes
```

### Пример IR

Для процедуры `Fact(n)`:

```
function Fact(n) -> i32 {
  entry:
    %0 = alloca "n" (4 bytes)
    store_local [%0], %p0
    %1 = alloca "res" (4 bytes)
    %2 = load [%0]
    %3 = le %2, 1
    br %3, bb2, bb3
  bb2:
    store [%1], 1
    jmp bb1
  bb3:
    %4 = load [%0]
    %5 = load [%0]
    %6 = sub %5, 1
    %7 = call Fact(%6)
    %8 = mul %4, %7
    store [%1], %8
    jmp bb1
  bb1:                              ; preds: bb2 bb3
    %9 = load [%1]
    ret %9
}
```

### Встроенные оптимизации

| Проход | Описание |
|---|---|
| `ConstantFolding` | Свёртка константных выражений (`add 2, 3` → `copy 5`) |
| `CopyPropagation` | Подстановка копий (`%1 = copy 5; add %1, %2` → `add 5, %2`) |
| `DeadCodeElim` | Удаление неиспользуемых инструкций и недостижимых блоков |

### Добавление своей оптимизации

Каждая оптимизация — класс-наследник `IPass`:

```cpp
// src/ir/passes/MyPass.h
#pragma once
#include "IPass.h"

class MyPass : public IPass {
public:
    std::string name() const override { return "MyPass"; }
    bool run(IRFunction& fn) override;
};
```

```cpp
// src/ir/passes/MyPass.cpp
#include "MyPass.h"

bool MyPass::run(IRFunction& fn) {
    bool changed = false;
    for (auto& bb : fn.blocks) {
        for (auto& instr : bb->instrs) {
            // Логика трансформации IR
        }
    }
    return changed;
}
```

Подключение через `PassManager` с fluent API:

```cpp
PassManager pm;
pm.add<ConstantFolding>()
  .add<MyPass>()
  .add<DeadCodeElim>();

pm.run(irModule);                  // без логирования
pm.run(irModule, &std::cerr);     // с дампом IR после каждого прохода
```

Порядок вызовов `add()` определяет порядок выполнения проходов.

### Набор IR-инструкций

| Категория | Инструкции |
|---|---|
| Арифметика | `add`, `sub`, `mul`, `div`, `mod`, `neg` |
| Сравнение | `eq`, `neq`, `lt`, `le`, `gt`, `ge` |
| Логика | `and`, `or`, `not` |
| Память | `load`, `store`, `alloca`, `index`, `addr_global`, `addr_local` |
| Поток управления | `br cond, bbTrue, bbFalse`, `jmp bb`, `ret` |
| Вызовы | `call`, `syscall` |
| Копирование | `copy` |

---

## Тестирование

Юнит-тесты и E2E-тесты **параметризованы** — они автоматически запускаются для всех включённых при сборке комбинаций токенизатор + парсер.

### Юнит-тесты (Google Test)

```bash
cd build
ctest -E e2e --output-on-failure
# или напрямую:
./o7rc_tests
```

При сборке с дефолтными опциями тесты запускаются только для Flex + Bison. При включении рукописных реализаций каждый тест запускается для всех скомпилированных комбинаций.

### E2E-тесты (OBNC vs o7rc + RARS)

Требуется Java, libgc-dev и сборка с `-DUSE_RARS=ON`:

```bash
cd build
ctest -L e2e --output-on-failure
```

E2E-тесты регистрируются для каждой включённой комбинации фронтенда (например, `e2e_Factorial_flex_bison`, `e2e_Factorial_hand_hand`). Каждый тест:
1. Компилирует `.obr` через OBNC → запускает нативный бинарник → эталонный вывод
2. Компилирует `.obr` → `.asm` через `o7rc` (с `--tokenizer`/`--parser`) → запускает в RARS → фактический вывод
3. Сравнивает выводы

Тестовые программы: `tests/e2e/programs/`.

### Все тесты

```bash
ctest --test-dir build --output-on-failure

# Полное тестирование всех 4 комбинаций фронтенда:
cmake -B build -DUSE_HAND_TOKENIZER=ON -DUSE_HAND_PARSER=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

---

## CI/CD

Проект использует GitHub Actions (`.github/workflows/ci.yml`). CI запускается на push и pull request в ветки `develop` и `main`.

Workflow тестирует 4 конфигурации фронтенда параллельно:

| Конфигурация | Лексер | Парсер |
|---|---|---|
| Flex + HandParser | Flex | рукописный |
| HandTokenizer + Bison | рукописный | Bison |
| HandTokenizer + HandParser | рукописный | рукописный |
| All | Flex + рукописный | Bison + рукописный |

Каждая конфигурация проходит сборку, юнит-тесты и E2E-тесты.

---

## Структура проекта

```
o7rc/
├── src/
│   ├── main.cpp                         # Точка входа, CLI
│   ├── tokenizer/
│   │   ├── ITokenizer.h                 # Интерфейс лексера
│   │   └── impl/
│   │       ├── flex/                    # Flex-лексер (FlexTokenizer, oberon.l)
│   │       ├── hand/                    # Рукописный лексер (HandTokenizer)
│   │       └── debug/                   # Отладочная обёртка (BufferedTokenizer)
│   ├── parser/
│   │   ├── IParser.h                    # Интерфейс парсера
│   │   └── impl/
│   │       ├── bison/                   # Bison-парсер (BisonParser, oberon.y)
│   │       └── hand/                    # Рукописный парсер (HandParser)
│   ├── util/
│   │   ├── Token.h                      # Определение токенов
│   │   └── ast/                         # AST-узлы (Expr, Stmt, Type, Decl, Module)
│   ├── codegen/                         # Прямая кодогенерация AST → RISC-V
│   │   ├── ICodeGen.h
│   │   ├── RiscVCodeGen.cpp
│   │   ├── Emitter.cpp
│   │   ├── SymbolTable.cpp
│   │   └── TypeInfo.cpp
│   └── ir/                              # IR-пайплайн
│       ├── Value.h                      # IRValue (Temp, Const, Param)
│       ├── Instruction.h                # IROp, IRInstr
│       ├── BasicBlock.h                 # Базовые блоки
│       ├── Function.h                   # IRFunction, IRModule, IRGlobal
│       ├── IRBuilder.{h,cpp}            # AST → IR
│       ├── IRPrinter.{h,cpp}            # Текстовый дамп IR
│       ├── IRDotExporter.{h,cpp}        # Экспорт CFG в Graphviz DOT
│       ├── PassManager.{h,cpp}          # Менеджер оптимизационных проходов
│       ├── RiscVIRCodeGen.{h,cpp}       # IR → RISC-V asm
│       └── passes/
│           ├── IPass.h                  # Интерфейс прохода
│           ├── ConstantFolding.{h,cpp}
│           ├── CopyPropagation.{h,cpp}
│           └── DeadCodeElim.{h,cpp}
├── tests/
│   ├── basic.cpp                        # Базовые тесты
│   ├── test_factory.h                   # Фабрика для параметризованных тестов
│   ├── tokenizer/                       # Параметризованные тесты лексера
│   ├── parser/                          # Параметризованные тесты парсера
│   └── e2e/
│       ├── run_test.sh                  # Скрипт запуска E2E-теста
│       └── programs/                    # Тестовые .obr программы
├── .github/workflows/ci.yml            # GitHub Actions CI
├── Dockerfile
├── CMakeLists.txt
└── README.md
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
