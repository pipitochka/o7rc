# o7rc

**o7rc** — учебный кросс-компилятор языка Oberon-7 в ассемблер RISC-V (RV32IM), совместимый с симулятором [RARS](https://github.com/TheThirdOne/rars).

Компилятор спроектирован для демонстрации основных этапов компиляции: лексический анализ, синтаксический анализ, семантический анализ, построение промежуточного представления (IR) с графом потока управления (CFG), оптимизации и кодогенерация.

## Оглавление
- [Архитектура](#архитектура)
- [Docker](#docker)
- [Зависимости](#зависимости)
- [Сборка](#сборка)
- [Запуск](#запуск)
- [Семантический анализ](#семантический-анализ)
- [IR и оптимизации](#ir-и-оптимизации)
- [Написание своих оптимизаций](#написание-своих-оптимизаций)
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
                   ┌─────────────┴─────────────┐
                   │    Семантический анализ    │
                   │         (Sema)             │
                   └─────────────┬─────────────┘
                                 │  AST (проверено)
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
| **Sema** | Семантический анализ: проверка областей видимости, типов, арности |
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
| `--no-sema` | Отключить семантический анализ |

Флаги `--opt`, `--dump-ir`, `--dump-ir-passes` и `--dump-dot` автоматически включают IR-пайплайн. Флаги `--tokenizer` и `--parser` имеют смысл только когда собрано несколько реализаций одновременно; по умолчанию используются Flex / Bison.

---

## Семантический анализ

Семантический анализатор (`Sema`) запускается автоматически после парсинга и проверяет корректность программы перед генерацией кода. Реализован как Visitor по AST.

### Проверки

| Проверка | Пример ошибки |
|---|---|
| Неопределённый символ | `x := 1` без объявления `VAR x` |
| Дублирование объявления | `VAR x: INTEGER; x: BOOLEAN;` |
| Присваивание константе | `CONST N = 5; BEGIN N := 10` |
| Арность вызова процедуры | `Add(1)` при `PROCEDURE Add(a, b: INTEGER)` |
| Отсутствие RETURN в функции | Функция с `: INTEGER` без `RETURN` |
| Несовпадение имени модуля | `MODULE A; END B.` |
| Неопределённая переменная цикла | `FOR i := 1 TO 10` без `VAR i` |
| Неопределённый тип | `VAR x: Foo` без `TYPE Foo` |
| Неопределённый модуль | `Foo.Bar` без `IMPORT Foo` |

Встроенные процедуры (`INC`, `DEC`, `ABS`, `Out.Int`, `Out.Ln` и т.д.) и типы (`INTEGER`, `BOOLEAN`, `REAL`, `CHAR`) известны анализатору.

Отключение: `--no-sema`.

---

## IR и оптимизации

Промежуточное представление — трёхадресный код (TAC) в базовых блоках, образующих граф потока управления (CFG).

### Пайплайн

```
AST → Sema → IRBuilder → IR/CFG → PassManager (оптимизации) → RiscVIRCodeGen → .asm
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

---

## Написание своих оптимизаций

Этот раздел описывает структуры данных IR и процесс создания собственных проходов оптимизации.

### Структура данных: IRModule

Верхний уровень IR — `IRModule`. Он содержит глобальные переменные, функции и тело модуля:

```cpp
struct IRModule {
    std::string name;                      // Имя модуля
    std::vector<IRGlobal> globals;         // Глобальные переменные и строки
    std::vector<IRFunction> functions;     // Процедуры
    IRFunction mainBody;                   // Тело BEGIN...END модуля
};
```

### Структура данных: IRFunction

Каждая функция содержит список базовых блоков, образующих CFG:

```cpp
struct IRFunction {
    std::string name;
    std::vector<std::string> params;                    // Имена параметров
    std::vector<bool> varParams;                        // VAR-параметры
    bool hasReturn = false;                             // Есть ли возвращаемое значение
    std::vector<std::unique_ptr<BasicBlock>> blocks;    // Базовые блоки (CFG)
    int nextBlockId = 0;
    int nextTempId = 0;

    struct LocalInfo { int tempId; int size; bool isVarParam; };
    std::unordered_map<std::string, LocalInfo> locals;  // Локальные переменные

    BasicBlock* createBlock();             // Создать новый блок
    IRValue freshTemp();                   // Создать новую временную переменную
    BasicBlock* entry();                   // Первый блок (точка входа)
    BasicBlock* blockById(int id);         // Найти блок по ID
    void linkBlocks(int from, int to);     // Связать блоки (CFG-ребро)
};
```

### Структура данных: BasicBlock

Базовый блок — линейная последовательность инструкций. Последняя инструкция — терминатор (`br`, `jmp` или `ret`). Блоки связаны рёбрами CFG:

```cpp
struct BasicBlock {
    int id;                                // Уникальный идентификатор
    std::string label;                     // Метка (например, "entry", "bb3")
    std::vector<IRInstr> instrs;           // Инструкции блока

    std::vector<int> predecessors;         // ID блоков-предшественников
    std::vector<int> successors;           // ID блоков-потомков

    bool hasTerminator() const;            // Есть ли терминатор
    void addSuccessor(int bbId);
    void addPredecessor(int bbId);
};
```

### Структура данных: IRInstr

Каждая инструкция — трёхадресная операция:

```cpp
struct IRInstr {
    IROp op;            // Код операции (Add, Sub, Load, Branch, ...)
    IRValue dst;        // Результат (куда записать)
    IRValue src1;       // Первый операнд
    IRValue src2;       // Второй операнд

    std::string name;                // Для Call/Alloca/AddrGlobal — имя
    std::vector<IRValue> args;       // Для Call/Syscall — аргументы
    int targetBlock = -1;            // Для Jump/Branch(true) — целевой блок
    int falseBlock = -1;             // Для Branch(false) — целевой блок
    int syscallNum = 0;              // Для Syscall — номер

    bool isTerminator() const;       // br, jmp, ret
    bool hasDst() const;             // Есть ли запись в dst
    bool readsSrc1() const;          // Читает ли src1
    bool readsSrc2() const;          // Читает ли src2
};
```

### Структура данных: IRValue

Значение в IR — временная переменная, константа, параметр или void:

```cpp
struct IRValue {
    enum Kind { Temp, Const, Param, Void };
    Kind kind;
    int id;              // Для Temp/Param — номер (%0, %1, %p0)
    int64_t constVal;    // Для Const — числовое значение

    bool isVoid() / isConst() / isTemp() / isParam() const;
    std::string str() const;   // "%0", "42", "%p1", "void"

    static IRValue temp(int id);
    static IRValue constant(int64_t v);
    static IRValue param(int index);
    static IRValue voidVal();
};
```

### Набор IR-инструкций (IROp)

| Категория | Операция | Формат | Описание |
|---|---|---|---|
| Арифметика | `Add` | `dst = add src1, src2` | Сложение |
| | `Sub` | `dst = sub src1, src2` | Вычитание |
| | `Mul` | `dst = mul src1, src2` | Умножение |
| | `Div` | `dst = div src1, src2` | Целочисленное деление |
| | `Mod` | `dst = mod src1, src2` | Остаток от деления |
| | `Neg` | `dst = neg src1` | Унарный минус |
| Сравнение | `Eq` | `dst = eq src1, src2` | Равно (→ 0/1) |
| | `Neq` | `dst = neq src1, src2` | Не равно |
| | `Lt` | `dst = lt src1, src2` | Меньше |
| | `Le` | `dst = le src1, src2` | Меньше или равно |
| | `Gt` | `dst = gt src1, src2` | Больше |
| | `Ge` | `dst = ge src1, src2` | Больше или равно |
| Логика | `And` | `dst = and src1, src2` | Побитовое И |
| | `Or` | `dst = or src1, src2` | Побитовое ИЛИ |
| | `Not` | `dst = not src1` | Побитовое НЕ |
| Память | `Alloca` | `dst = alloca "name" (N bytes)` | Выделить слот на стеке |
| | `Load` | `dst = load [src1]` | Загрузить значение по адресу |
| | `Store` | `store [src1], src2` | Записать значение по адресу |
| | `Index` | `dst = index src1, src2` | Адрес элемента массива |
| | `AddrGlobal` | `dst = addr_global "name"` | Адрес глобальной переменной |
| | `AddrLocal` | `dst = addr_local "name"` | Адрес локальной переменной |
| Управление | `Branch` | `br src1, bbT, bbF` | Условный переход |
| | `Jump` | `jmp bbTarget` | Безусловный переход |
| | `Ret` | `ret src1` | Возврат из функции |
| Вызовы | `Call` | `dst = call "name"(args...)` | Вызов процедуры |
| | `Syscall` | `syscall N(args...)` | Системный вызов (I/O) |
| Прочее | `Copy` | `dst = copy src1` | Копирование значения |

### Создание прохода оптимизации

Каждый проход — класс-наследник `IPass`. Метод `run()` получает `IRFunction&` и обходит её блоки и инструкции:

**1. Заголовок** (`src/ir/passes/MyPass.h`):

```cpp
#pragma once
#include "IPass.h"

class MyPass : public IPass {
public:
    std::string name() const override { return "MyPass"; }
    bool run(IRFunction& fn) override;
};
```

**2. Реализация** (`src/ir/passes/MyPass.cpp`):

```cpp
#include "MyPass.h"

bool MyPass::run(IRFunction& fn) {
    bool changed = false;

    // Обход всех блоков функции
    for (auto& bb : fn.blocks) {

        // Обход всех инструкций блока
        for (auto& instr : bb->instrs) {

            // Пример: заменить умножение на 2 сложением
            if (instr.op == IROp::Mul
                && instr.src2.isConst()
                && instr.src2.constVal == 2)
            {
                instr.op = IROp::Add;
                instr.src2 = instr.src1;  // dst = add src1, src1
                changed = true;
            }
        }
    }

    return changed;  // true если IR был изменён
}
```

**3. Подключение через `CMakeLists.txt`** — добавить `.cpp` файл:

```cmake
target_sources(core PRIVATE
        ${SRC_DIR}/ir/passes/MyPass.cpp
        # ...
)
```

**4. Регистрация в `main.cpp`** (или в любом месте, где создаётся `PassManager`):

```cpp
#include <ir/passes/MyPass.h>

PassManager pm;
pm.add<ConstantFolding>()
  .add<MyPass>()              // ← ваш проход
  .add<CopyPropagation>()
  .add<DeadCodeElim>();

pm.run(irModule, &std::cerr);  // дамп IR после каждого прохода
```

### Типичные паттерны оптимизаций

**Замена инструкции** — изменить `op`, `src1`, `src2` у существующей `IRInstr`:

```cpp
// Свёртка: add 3, 4 → copy 7
if (instr.op == IROp::Add && instr.src1.isConst() && instr.src2.isConst()) {
    instr.op = IROp::Copy;
    instr.src1 = IRValue::constant(instr.src1.constVal + instr.src2.constVal);
    instr.src2 = IRValue::voidVal();
    changed = true;
}
```

**Подстановка значений** — заменить использования временной переменной:

```cpp
// Если %5 = copy 42, заменить все %5 на 42
std::unordered_map<int, IRValue> copies;
for (auto& bb : fn.blocks)
    for (auto& i : bb->instrs)
        if (i.op == IROp::Copy && i.dst.isTemp())
            copies[i.dst.id] = i.src1;

for (auto& bb : fn.blocks)
    for (auto& i : bb->instrs) {
        if (i.readsSrc1() && i.src1.isTemp() && copies.count(i.src1.id))
            i.src1 = copies[i.src1.id];
        if (i.readsSrc2() && i.src2.isTemp() && copies.count(i.src2.id))
            i.src2 = copies[i.src2.id];
    }
```

**Удаление инструкций** — `std::remove_if` по вектору `bb->instrs`:

```cpp
for (auto& bb : fn.blocks) {
    auto& instrs = bb->instrs;
    auto it = std::remove_if(instrs.begin(), instrs.end(),
        [&](const IRInstr& i) {
            if (i.isTerminator() || i.op == IROp::Store || i.op == IROp::Syscall)
                return false;  // нельзя удалить побочные эффекты
            if (!i.hasDst() || !i.dst.isTemp())
                return false;
            return usedTemps.find(i.dst.id) == usedTemps.end();
        });
    if (it != instrs.end()) {
        instrs.erase(it, instrs.end());
        changed = true;
    }
}
```

**Работа с CFG** — использование `predecessors`/`successors`:

```cpp
for (auto& bb : fn.blocks) {
    // Блок с единственным предшественником — кандидат на слияние
    if (bb->predecessors.size() == 1) {
        auto* pred = fn.blockById(bb->predecessors[0]);
        // ...
    }
    // Переход на блок с одним преемником
    if (bb->successors.size() == 1) {
        int target = bb->successors[0];
        // ...
    }
}
```

### Отладка проходов

Запуск с `--dump-ir-passes` выведет IR после каждого прохода:

```bash
./o7rc input.obr -o out.asm --opt --dump-ir --dump-ir-passes 2>ir_log.txt
```

Формат вывода:

```
; --- after ConstantFolding (changed) on main ---
function main() {
  entry:
    %0 = alloca "x" (4 bytes)
    %1 = copy 14            ← результат свёртки 2 + 3 * 4
    store [%0], %1
    ret void
}
```

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

| Категория | Файлы | Описание |
|---|---|---|
| Лексер | `tests/tokenizer/test_tokenizer_*.cpp` | Токенизация, грамматика, сниппеты |
| Парсер | `tests/parser/test_parser_*.cpp` | Структура AST, выражения, типы, операторы |
| Sema | `tests/sema/test_sema.cpp` | 27 тестов: позитивные + негативные |
| IR Builder | `tests/ir/test_ir_builder.cpp` | Построение IR: модули, переменные, CFG |
| IR Passes | `tests/ir/test_ir_passes.cpp` | ConstantFolding, CopyPropagation, DCE, PassManager |

При сборке с дефолтными опциями тесты запускаются только для Flex + Bison. При включении рукописных реализаций каждый тест запускается для всех скомпилированных комбинаций.

### E2E-тесты (OBNC vs o7rc + RARS)

Требуется Java, libgc-dev и сборка с `-DUSE_RARS=ON`:

```bash
cd build
ctest -L e2e --output-on-failure
```

E2E-тесты регистрируются для каждой включённой комбинации фронтенда в двух вариантах — прямая кодогенерация и IR-пайплайн (например, `e2e_Factorial_flex_bison`, `e2e_Factorial_flex_bison_ir`). Каждый тест:
1. Компилирует `.obr` через OBNC → эталонный вывод
2. Компилирует `.obr` → `.asm` через `o7rc` → запускает в RARS → фактический вывод
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

Workflow тестирует 5 конфигураций фронтенда параллельно:

| Конфигурация | Лексер | Парсер |
|---|---|---|
| Flex + Bison | Flex | Bison |
| Flex + HandParser | Flex | рукописный |
| HandTokenizer + Bison | рукописный | Bison |
| HandTokenizer + HandParser | рукописный | рукописный |
| All | Flex + рукописный | Bison + рукописный |

Каждая конфигурация проходит сборку, юнит-тесты (лексер, парсер, Sema, IR) и E2E-тесты (прямая + IR кодогенерация).

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
│   ├── sema/                            # Семантический анализ
│   │   ├── Sema.h / Sema.cpp           # Visitor по AST — проверки
│   │   └── SemaError.h                 # Типы ошибок
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
│       ├── Value.h                      # IRValue (Temp, Const, Param, Void)
│       ├── Instruction.h                # IROp, IRInstr
│       ├── BasicBlock.h                 # Базовые блоки CFG
│       ├── Function.h                   # IRFunction, IRModule, IRGlobal
│       ├── IRBuilder.{h,cpp}            # AST → IR
│       ├── IRPrinter.{h,cpp}            # Текстовый дамп IR
│       ├── IRDotExporter.{h,cpp}        # Экспорт CFG в Graphviz DOT
│       ├── PassManager.{h,cpp}          # Менеджер оптимизационных проходов
│       ├── RiscVIRCodeGen.{h,cpp}       # IR → RISC-V asm
│       └── passes/
│           ├── IPass.h                  # Интерфейс прохода оптимизации
│           ├── ConstantFolding.{h,cpp}  # Свёртка констант
│           ├── CopyPropagation.{h,cpp}  # Распространение копий
│           └── DeadCodeElim.{h,cpp}     # Удаление мёртвого кода
├── tests/
│   ├── basic.cpp                        # Базовые тесты
│   ├── test_factory.h                   # Фабрика для параметризованных тестов
│   ├── tokenizer/                       # Параметризованные тесты лексера
│   ├── parser/                          # Параметризованные тесты парсера
│   ├── sema/                            # Тесты семантического анализа
│   ├── ir/                              # Тесты IR и оптимизаций
│   └── e2e/
│       ├── run_test.sh                  # Скрипт запуска E2E-теста
│       └── programs/                    # Тестовые .obr программы
├── .github/workflows/ci.yml            # GitHub Actions CI
├── Dockerfile
├── CMakeLists.txt
└── README.md
```

---

## Ограничения и управление памятью

Компилятор реализует основное подмножество Oberon-7, достаточное для учебных целей. Ниже описано текущее состояние поддержки динамической памяти и связанных возможностей.

### Динамическая память (`NEW`)

`NEW(ptr)` транслируется в syscall `sbrk` (RARS ecall 9), который выделяет блок из кучи:

```
li a0, 256      # размер блока — фиксированные 256 байт
li a7, 9        # sbrk
ecall           # a0 ← адрес выделенного блока
```

**Текущие ограничения:**
- Размер блока **фиксирован** — 256 байт, независимо от реального размера типа. Для мелких структур это расточительно, для крупных — недостаточно.
- **Нет освобождения памяти** — `DISPOSE` не поддерживается, нет сборщика мусора. Выделенная память не возвращается. Для коротких учебных программ в RARS это приемлемо.
- Реальный размер типа не вычисляется — `TypeInfo` не используется при вызове `NEW`.

### Указатели (`POINTER TO`)

Указатели поддерживаются на уровне синтаксиса и базовых операций:

| Операция | Поддержка | Как работает |
|---|---|---|
| `NEW(p)` | Частично | sbrk с фиксированным размером 256 байт |
| `p^` (разыменование) | Да | `lw a0, 0(a0)` — загрузка значения по адресу |
| `p := NIL` | Да | Присваивание нулевого адреса |
| `p = NIL` / `p # NIL` | Да | Сравнение с нулём |

### Записи (`RECORD`)

Записи поддерживаются на уровне парсинга и AST, но кодогенерация ограничена:

| Операция | Поддержка | Примечание |
|---|---|---|
| `TYPE R = RECORD ... END` | Парсинг — да | Тип в кодогенерации приводится к `INTEGER` (4 байта) |
| `r.field` | Заглушка | Селектор поля распознаётся, но смещение не вычисляется |
| `RECORD(Base)` | Парсинг — да | Наследование записей не влияет на кодогенерацию |

### Что работает полностью

| Возможность | Описание |
|---|---|
| Целые числа (`INTEGER`) | Арифметика, сравнения, I/O |
| Булевы (`BOOLEAN`) | `TRUE`, `FALSE`, `&`, `OR`, `~` |
| Массивы (`ARRAY N OF T`) | Индексация, `LEN`, вложенные массивы |
| Строковые литералы | Передача в `Out.String` |
| Процедуры | Рекурсия, параметры, `VAR`-параметры, возвращаемые значения |
| Управляющие конструкции | `IF`/`ELSIF`/`ELSE`, `WHILE`, `REPEAT`, `FOR`, `CASE` |
| Встроенные процедуры | `INC`, `DEC`, `ABS`, `ODD`, `ORD`, `CHR`, `LEN`, `NEW` |
| I/O | `Out.Int`, `Out.Ln`, `Out.String`, `Out.Char`, `In.Int` |

### Потенциальные улучшения

- Вычисление реального размера `RECORD`/`POINTER TO` при `NEW`
- Вычисление смещений полей записей через `TypeInfo`
- `DISPOSE` или простейший mark-and-sweep GC
- `REAL` — арифметика с плавающей точкой (RV32F)

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
