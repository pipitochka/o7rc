# o7rc

**o7rc** — учебный кросс-компилятор языка Oberon-7 в ассемблер RISC-V (RV32IM), совместимый с симулятором [RARS](https://github.com/TheThirdOne/rars).

Компилятор спроектирован для демонстрации основных этапов компиляции: лексический анализ, синтаксический анализ, семантический анализ, построение промежуточного представления (IR) с графом потока управления (CFG), оптимизации и кодогенерация.

## Оглавление
- [Архитектура](#архитектура)
- [Docker](#docker)
- [Зависимости](#зависимости)
- [Сборка](#сборка)
- [Запуск](#запуск)
- [Модульная система](#модульная-система)
- [Добавление runtime-модулей stdlib](#добавление-runtime-модулей-stdlib)
- [Семантический анализ](#семантический-анализ)
- [IR и оптимизации](#ir-и-оптимизации)
- [Написание своих оптимизаций](#написание-своих-оптимизаций)
- [Тестирование](#тестирование)
- [CI/CD](#cicd)
- [Структура проекта](#структура-проекта)
- [Динамическая память и записи](#динамическая-память-и-записи)
- [Реализованные возможности](#реализованные-возможности)
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
                                 │
                   ┌─────────────┴─────────────┐
                   │         Парсер             │
                   │  (Bison или рукописный)    │
                   └─────────────┬─────────────┘
                                 │
                   ┌─────────────┴─────────────┐
                   │      ModuleLoader          │
                   │  (загрузка IMPORT модулей) │
                   └─────────────┬─────────────┘
                                 │
                   ┌─────────────┴─────────────┐
                   │    Семантический анализ    │
                   │         (Sema)             │
                   └─────────────┬─────────────┘
                                 │
                     ┌───────────┴───────────┐
                     │                       │
              ┌──────┴──────┐         ┌──────┴──────┐
              │  Прямая     │         │  IRBuilder  │
              │  кодоген.   │         │  AST → IR   │
              │  (AST→asm)  │         └──────┬──────┘
              └──────┬──────┘                │
                     │                ┌──────┴──────┐
                     │                │ PassManager  │
                     │                │ (оптимизации)│
                     │                └──────┬──────┘
                     │                       │
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
| **ModuleLoader** | Поиск, парсинг и кэширование импортированных модулей |
| **Sema** | Семантический анализ: проверка областей видимости, типов, арности |
| **IR / CFG** | Трёхадресный код в базовых блоках с графом потока управления |
| **PassManager** | Управление проходами оптимизации с fluent API |
| **Кодогенерация (прямая)** | Генерация RISC-V ассемблера напрямую из AST (`RiscVCodeGen`) |
| **Кодогенерация (IR)** | Генерация RISC-V ассемблера из оптимизированного IR (`RiscVIRCodeGen`) |
| **Runtime (Out / In)** | Каталог процедур и генерация syscall’ов RARS: `src/runtime/` (`StdlibProc`, `RiscVStdlibAsm`, `IRStdlib`) |
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

### Опциональные

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
| `USE_CODEGEN` | `ON` | Включить прямую кодогенерацию RISC-V (AST → asm) и файл `runtime/RiscVStdlibAsm.cpp` |
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

Типичная программа с `IMPORT Out`, `IMPORT In` или `IMPORT Math` **обязана** видеть соответствующие `.obr` на пути модулей (`-M`), иначе загрузка импортов завершится ошибкой. В репозитории это каталог **`stdlib/`**.

```bash
# Прямая кодогенерация (AST → asm), с модулями из stdlib/
./o7rc input.obr -o output.asm -M stdlib

# Тот же исходник через IR-пайплайн
./o7rc input.obr -o output.asm -M stdlib --ir

# IR + оптимизации
./o7rc input.obr -o output.asm -M stdlib --opt

# Запуск в RARS (путь к jar зависит от сборки, см. build/)
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
| `-M <dir>`, `--module-path <dir>` | Добавить каталог поиска модулей (можно указать несколько раз) |

Флаги `--opt`, `--dump-ir`, `--dump-ir-passes` и `--dump-dot` автоматически включают IR-пайплайн. Флаги `--tokenizer` и `--parser` имеют смысл только когда собрано несколько реализаций одновременно; по умолчанию используются Flex / Bison.

---

## Модульная система

Компилятор поддерживает импорт пользовательских модулей. Модуль — это отдельный `.obr` файл с экспортированными символами (помеченными `*`).

### Принцип работы

Компиляция происходит **в один файл** (single-file inlining): при обнаружении `IMPORT` компилятор находит, парсит и встраивает объявления импортированного модуля в итоговый `.asm`. Для констант, типов и процедур во встраивание попадают только **экспортированные** (`*`) символы; **модульные переменные** (`VAR` на уровне модуля) из импортов сейчас обрабатываются без фильтра по экспорту (в ассемблере могут появиться лишние глобальные). Раздельной компиляции и линковки нет — весь код оказывается в одном ассемблерном файле.

### Архитектура

1. **`ModuleLoader`** — ищет `.obr`/`.obn` файлы по списку каталогов (`-M` / `--module-path`), парсит их тем же фронтендом, что и основной модуль, кэширует AST и рекурсивно загружает транзитивные импорты.
2. Модуль **`SYSTEM`** не загружается с диска — его процедуры обрабатываются компилятором напрямую. Модули **`Out`** и **`In`** загружаются как обычные: в репозитории заданы **заглушки** `stdlib/Out.obr` и `stdlib/In.obr` (сигнатуры экспорта, пустые тела). Их **реализация для RISC-V/RARS** сосредоточена в каталоге `src/runtime/` (`StdlibProc` — каталог имён и классификация; `RiscVStdlibAsm`, `IRStdlib` — генерация asm и IR), а не размазана по всему кодогенератору. Для программ с `IMPORT Out` / `IMPORT In` нужен каталог интерфейсов, например `-M stdlib`.
3. При кодогенерации (как прямой, так и IR) экспортированные объявления импортированных модулей обрабатываются первыми, после чего генерируется код основного модуля.

### Экспорт символов

В Oberon-7 символ экспортируется добавлением `*` после имени:

```oberon
PROCEDURE Abs*(x: INTEGER): INTEGER;   (* экспортирована *)
PROCEDURE helper(x: INTEGER): INTEGER; (* приватная *)
```

### Создание своего модуля

**1. Создайте файл модуля** (например, `stdlib/Math.obr`):

```oberon
MODULE Math;

PROCEDURE Abs*(x: INTEGER): INTEGER;
VAR r: INTEGER;
BEGIN
    IF x < 0 THEN r := -x ELSE r := x END
RETURN r
END Abs;

PROCEDURE Power*(base, exp: INTEGER): INTEGER;
VAR r, i: INTEGER;
BEGIN
    r := 1;
    FOR i := 1 TO exp DO
        r := r * base
    END
RETURN r
END Power;

END Math.
```

**2. Используйте его в программе**:

```oberon
MODULE UseMath;
IMPORT Out, Math;

BEGIN
    Out.Int(Math.Abs(-42), 0); Out.Ln;
    Out.Int(Math.Power(2, 10), 0); Out.Ln
END UseMath.
```

**3. Скомпилируйте**, указав путь к каталогу с модулями:

```bash
./o7rc UseMath.obr -o out.asm -M stdlib
java -jar rars.jar nc sm out.asm
```

Вывод:

```
42
1024
```

### Стандартная библиотека

В каталоге `stdlib/` поставляются модуль **`Math`** и интерфейсы **`Out`** и **`In`**.

**`Math`** — экспортированные функции:

| Процедура | Описание |
|---|---|
| `Abs(x)` | Абсолютное значение |
| `Min(a, b)` | Минимум из двух чисел |
| `Max(a, b)` | Максимум из двух чисел |
| `Clamp(val, lo, hi)` | Ограничение значения диапазоном |
| `Power(base, exp)` | Возведение в степень |
| `GCD(a, b)` | Наибольший общий делитель |
| `Factorial(n)` | Факториал |

**`Out.obr`** — только интерфейс (пустые тела). Реализация для RARS — в **`src/runtime/`** (`RiscVStdlibAsm` при прямой кодогенерации, `IRStdlib` при `--ir`).

| Процедура | Описание |
|---|---|
| `Int(x, w: INTEGER)` | Печать целого (`x`; ширина `w` пока не используется) |
| `Ln` | Перевод строки |
| `String(s: ARRAY OF CHAR)` | Печать ASCIIZ-строки |
| `Char(ch: CHAR)` | Печать одного символа |
| `Real(x: REAL)` | Печать вещественного (RARS syscall 2) |

**`In.obr`** — только интерфейс; машинный код там же, в **`src/runtime/`**.

| Процедура | Описание |
|---|---|
| `Open` | Инициализация ввода (no-op) |
| `Int(VAR x: INTEGER)` | Чтение целого с stdin |
| `Char(VAR ch: CHAR)` | Чтение одного символа |
| `Line(VAR s: ARRAY OF CHAR)` | Чтение строки в массив символов |

Для строк и `ARRAY n OF CHAR` элементы считаются **однобайтовыми** (`CHAR` размером 1 в массиве), что согласовано с `In.Line`, `Out.String` и окружением RARS.

Каталог `stdlib/` автоматически подключается при E2E-тестировании (`-M stdlib`). В скрипте `tests/e2e/run_test.sh` файлы **`Out.obr`** и **`In.obr`** **не копируются** в сборку OBNC: эталонный вывод строится системными модулями **Out** и **In** из OBNC, а `o7rc` использует свою реализацию тех же вызовов.

### Добавление runtime-модулей stdlib

Наряду с обычными модулями вроде **`Math`** (полный Oberon-код компилируется в ассемблер как часть программы), можно завести **runtime-модуль**: в `stdlib/` лежит только **контракт** (экспортируемые сигнатуры с пустыми телами), а **семантика на машине** задаётся вручную в C++ — через syscall’ы RARS, специальные последовательности инструкций и т.п. Так сделаны **`Out`** и **`In`**.

**Зачем так:** в языке нет прямого доступа к `ecall`/системным вызовам симулятора; проще и предсказуемее один раз описать отображение «вызов Oberon → asm/IR», чем пытаться выразить это чистым Oberon в заглушке.

**Чем отличается от `Math`:**

| | Обычный модуль (`Math`) | Runtime-модуль (`Out`, `In`, ваш новый) |
|---|---|---|
| Файл в `stdlib/` | Полные процедуры с телами | Только объявления `PROCEDURE …*; BEGIN END` |
| Код в `.asm` | Генерируется из AST процедур | Вызовы `ИмяМодуля.Proc` перехватываются до `jal proc_…` |
| Где логика | В `.obr` | В `src/runtime/` (`RiscVStdlibAsm`, `IRStdlib`) |

**Цепочка вызова:** при разборе выражения или оператора с квалификатором `Модуль.процедура` сначала вызываются `riscvEmitStdlibCall` / `irEmitStdlibCall` (есть скобки с аргументами) или `riscvEmitStdlibStmt` / `irEmitStdlibStmt` (редкий случай оператора **без** скобок, как `Out.Ln` или `In.Open`). Если `classifyStdlibProc` узнаёт пару модуль/процедура — генерируется нативный код; иначе управление уходит к обычным процедурам из AST (в т.ч. к пустым заглушкам из `.obr`).

#### Как добавить новый runtime-модуль

1. **`stdlib/Имя.obr`** — модуль с нужными `PROCEDURE …*;` и пустыми `BEGIN END` (как у `Out`/`In`). Имена модуля и процедур должны совпасть с тем, что вы заведёте в runtime.

2. **`ModuleLoader`** (`src/module/ModuleLoader.cpp`) — модуль **не** должен попадать в `kBuiltins` (там только `SYSTEM`). Тогда он загружается с диска как любой другой импорт, и Sema видит `IMPORT`.

3. **`src/runtime/StdlibProc.h`** — добавьте константы в перечисление `StdlibProcKind` (например `MyMod_Foo`).

4. **`src/runtime/StdlibProc.cpp`** — в `classifyStdlibProc` сопоставьте `module == "MyMod"` и `proc == "Foo"` с новым видом; в `stdlibQualifiedProcNames()` добавьте строку `"MyMod.Foo"` (и все остальные экспортируемые вызовы, которые Sema должен считать «известными» без разбора тела).

5. **`src/runtime/RiscVStdlibAsm.cpp`** — ветка `switch` для каждого `StdlibProcKind`: генерируйте инструкции через `cg.emit_`, выражения — `args->args[i]->accept(cg)` и т.д. Класс `RiscVCodeGen` объявляет эти функции как `friend`, чтобы был доступ к `emit_`, `emitAddress`, `typeAfterDesignator`. Сборка: только при **`USE_CODEGEN=ON`**.

6. **`src/runtime/IRStdlib.cpp`** — те же процедуры для IR-пайплайна: `Syscall`, `Store`, адреса через `emitAddress` / `emitExpr`. Нужен всегда, если используется `--ir`.

7. **Оператор без скобок** — если в языке процедура допускается как `MyMod.Bar` без `()` (как `Out.Ln`), добавьте обработку в `riscvEmitStdlibStmt` и `irEmitStdlibStmt`.

8. **E2E** — если у OBNC уже есть одноимённый системный модуль и вы не хотите подменять его пустой заглушкой, в `tests/e2e/run_test.sh` при копировании из `-M` добавьте исключение по имени модуля (по аналогии с `In` и `Out`).

**Ограничения:** нет плагинов или JSON-описания — каждый новый вызов добавляется в enum и в два `switch`. Префикс в исходнике должен быть **реальным** именем модуля (`MyMod.Proc`), не псевдонимом из `IMPORT M := MyMod`. Функции с результатом как часть runtime-модуля сейчас не развивались (в `Out`/`In` — процедуры).

### Ограничения модульной системы

- **Нет раздельной компиляции** — все импортированные модули встраиваются (inlining) в один `.asm` файл.
- **Циклические импорты** — явного запрета в компиляторе нет (загрузчик кэширует уже открытые модули), но такая схема не тестируется и может привести к дублированию AST или к некорректной сборке; лучше избегать.
- **Переименование при импорте** — конструкция `IMPORT M := RealName` разбирается и попадает в Sema как псевдоним, но **кодогенерация** ожидает в исходнике префикс **реального** имени модуля (`RealName.Proc`), а не псевдонима (`M.Proc`), поэтому на практике не поддерживается.

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

Список известных **неквалифицированных** встроек (`INC`, `DEC`, `ABS`, …) задан в `Sema`. Имена модулей **`Out.*`** и **`In.*`** подмешиваются из **`runtime/StdlibProc`** (`stdlibQualifiedProcNames`), чтобы не дублировать строки. Типы `INTEGER`, `BOOLEAN`, `REAL`, `CHAR` и др. тоже известны анализатору. Для вызовов вида `ИмяМодуля.идентификатор` проверяется, что **первый компонент** — объявленный импорт; полная проверка того, что целевой символ существует в том модуле (например, что у `Math` действительно есть `Abs`), **не доведена** — часть ошибок вылезет уже на этапе кодогенерации.

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

    bool isVoid() const;
    bool isConst() const;
    bool isTemp() const;
    bool isParam() const;
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
| | `Index` | `dst = index src1, src2` | Адрес элемента массива (шаг 4 байта) |
| | `Index1` | `dst = index1 src1, src2` | Адрес элемента `ARRAY … OF CHAR` (шаг 1 байт) |
| | `Load8` / `Store8` | `dst = load [addr]` / `store [addr], val` | Один байт (CHAR в массиве) |
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

**4. Регистрация в `main.cpp`**:

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
| Sema | `tests/sema/test_sema.cpp` | 29 тестовых сценариев (параметризация по фронтенду) |
| IR Builder | `tests/ir/test_ir_builder.cpp` | Построение IR: модули, переменные, CFG |
| IR Passes | `tests/ir/test_ir_passes.cpp` | ConstantFolding, CopyPropagation, DCE, PassManager |
| IR Memory | `tests/ir/test_ir_memory.cpp` | RECORD, POINTER TO, NEW, доступ к полям, вложенные записи |

При сборке с дефолтными опциями тесты запускаются только для Flex + Bison. При включении рукописных реализаций каждый тест запускается для всех скомпилированных комбинаций.

### E2E-тесты (OBNC vs o7rc + RARS)

Требуется Java, libgc-dev и сборка с `-DUSE_RARS=ON`:

```bash
cd build
ctest -L e2e --output-on-failure
```

E2E-тесты регистрируются для каждой включённой комбинации фронтенда в двух вариантах — прямая кодогенерация и IR-пайплайн (например, `e2e_Factorial_flex_bison`, `e2e_Factorial_flex_bison_ir`). Если в сборке доступны все четыре пары (flex+bison, flex+hand, hand+bison, hand+hand), получается **8** e2e-прогонов на каждую из **45** программ (всего **360** тестов с меткой `e2e`). Каждый тест:
1. Компилирует `.obr` через OBNC → эталонный вывод
2. Компилирует `.obr` → `.asm` через `o7rc` (с `-M stdlib`) → запускает в RARS → фактический вывод
3. Сравнивает выводы

**45 тестовых программ** в `tests/e2e/programs/`:

| Категория | Программы |
|---|---|
| Базовые | `Hello`, `Arith`, `Assign`, `Negate`, `ConstTest` |
| Управление | `IfElse`, `NestedIf`, `CaseStmt`, `WhileLoop`, `RepeatTest`, `ForLoop`, `NestedLoop` |
| Логика | `BoolLogic`, `EvenOdd` |
| Массивы | `ArraySum`, `ArrayReverse`, `BubbleSort` |
| Процедуры | `Factorial`, `Fibonacci`, `MultiProc`, `Scope`, `MutualCall`, `RecSum` |
| Записи/указатели | `Records`, `RecordProc`, `LinkedList` |
| Встроенные | `IncDec`, `AbsOdd`, `DivMod` |
| Алгоритмы | `GCD`, `Power`, `Primes`, `SumDigits`, `Collatz` |
| Строки | `StringHello`, `StringTwoParts` (`Out.String`, литералы) |
| С входными данными | `ReadSum`, `ReadMax`, `ReadFact`, `ReadGCD`, `ReadPower`, `ReadArray`, `ReadClassify`, `ReadLineEcho` (`In.Line`; см. `.in`) |
| Модули | `UseMath` (импорт `Math`) |

**Тесты с входными данными** — рядом с `.obr` лежит `.in` с числами или текстом для stdin. Скрипт `run_test.sh` нормализует содержимое и подаёт его и OBNC, и RARS (используется для `In.Int`, `ReadLineEcho` с `In.Line` и т.п.).

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
│   ├── runtime/                         # Out/In под RARS (не Oberon-текст)
│   │   ├── StdlibProc.{h,cpp}           # Квалифицированные имена для Sema; classifyStdlibProc
│   │   ├── RiscVStdlibAsm.{h,cpp}       # AST → asm syscalls (только при USE_CODEGEN)
│   │   └── IRStdlib.{h,cpp}             # AST → IR syscall (всегда в core)
│   ├── module/                          # Модульная система
│   │   ├── ModuleLoader.h              # Интерфейс загрузчика модулей
│   │   └── ModuleLoader.cpp            # Поиск, парсинг, кэширование модулей
│   ├── sema/                            # Семантический анализ
│   │   ├── Sema.h / Sema.cpp           # Visitor по AST — проверки
│   │   └── SemaError.h                 # Типы ошибок
│   ├── util/
│   │   ├── Token.h                      # Определение токенов
│   │   └── ast/                         # AST-узлы (Expr, Stmt, Type, Decl, Module)
│   ├── codegen/                         # Прямая кодогенерация AST → RISC-V
│   │   ├── ICodeGen.h
│   │   ├── RiscVCodeGen.{h,cpp}         # AST → asm; вызовы Out/In делегируются в runtime/
│   │   ├── Emitter.cpp
│   │   ├── SymbolTable.cpp
│   │   └── TypeInfo.cpp
│   └── ir/                              # IR-пайплайн
│       ├── Value.h                      # IRValue (Temp, Const, Param, Void)
│       ├── Instruction.h                # IROp, IRInstr
│       ├── BasicBlock.h                 # Базовые блоки CFG
│       ├── Function.h                   # IRFunction, IRModule, IRGlobal
│       ├── IRBuilder.{h,cpp}            # AST → IR; Out/In через runtime/IRStdlib
│       ├── IRPrinter.{h,cpp}            # Текстовый дамп IR
│       ├── IRDotExporter.{h,cpp}        # Экспорт CFG в Graphviz DOT
│       ├── PassManager.{h,cpp}          # Менеджер оптимизационных проходов
│       ├── RiscVIRCodeGen.{h,cpp}       # IR → RISC-V asm
│       └── passes/
│           ├── IPass.h                  # Интерфейс прохода оптимизации
│           ├── ConstantFolding.{h,cpp}  # Свёртка констант
│           ├── CopyPropagation.{h,cpp}  # Распространение копий
│           └── DeadCodeElim.{h,cpp}     # Удаление мёртвого кода
├── stdlib/                              # Стандартная библиотека модулей
│   ├── Math.obr                         # Math (Abs, Min, Max, Power, GCD, ...)
│   ├── Out.obr                          # Интерфейс Out (заглушка; codegen даёт реализацию)
│   └── In.obr                           # Интерфейс In (заглушка; codegen даёт реализацию)
├── tests/
│   ├── basic.cpp                        # Базовые тесты
│   ├── test_factory.h                   # Фабрика для параметризованных тестов
│   ├── tokenizer/                       # Параметризованные тесты лексера
│   ├── parser/                          # Параметризованные тесты парсера
│   ├── sema/                            # Тесты семантического анализа
│   ├── ir/                              # Тесты IR, оптимизаций и памяти
│   └── e2e/
│       ├── run_test.sh                  # Скрипт запуска E2E-теста
│       └── programs/                    # 45 тестовых .obr (+ .in где нужен stdin)
├── .github/workflows/ci.yml            # GitHub Actions CI
├── Dockerfile
├── CMakeLists.txt
└── README.md
```

---

## Динамическая память и записи

Компилятор поддерживает динамическую память через `POINTER TO` и `NEW`, а также записи (`RECORD`) с вычислением смещений полей.

### Пример

```oberon
MODULE Records;
IMPORT Out;

TYPE
    Point = RECORD
        x, y: INTEGER
    END;

VAR
    p: POINTER TO Point;
    r: Point;

BEGIN
    r.x := 10; r.y := 20;
    Out.Int(r.x, 0); Out.Ln;  (* 10 *)

    NEW(p);
    p.x := 30; p.y := 40;
    Out.Int(p.x, 0); Out.Ln;  (* 30 *)
END Records.
```

### Как это работает

**Записи (`RECORD`)** — поля размещаются последовательно; для скалярных типов в этом компиляторе размер поля совпадает с размером типа (для `INTEGER`, `BOOLEAN`, `REAL`, `CHAR`, `SET` это **4 байта** RV32). `TypeDecl` регистрирует тип в реестре с вычислением смещений:

```
TYPE Point = RECORD x, y: INTEGER END;
→ x: offset=0, size=4
  y: offset=4, size=4
  total: 8 байт
```

Переменная типа запись на **уровне модуля** — `.space` в `.data`. Запись как **локальная переменная процедуры** — выделяется в её стековом кадре (смещения от `s0`), а не в `.data`.

**Доступ к полям** (`r.x`, `p.y`) — при кодогенерации вычисляется адрес базы + смещение поля:

```
la a0, _Records_r     # адрес записи
addi a0, a0, 4        # + offset поля y
lw a0, 0(a0)          # значение поля
```

**Автоматическое разыменование указателей** — при `p.x` компилятор автоматически разыменовывает указатель перед доступом к полю:

```
la a0, _Records_p     # адрес переменной-указателя
lw a0, 0(a0)          # разыменование: a0 = адрес записи в куче
addi a0, a0, 0        # + offset поля x (= 0 для первого поля)
```

**`NEW(p)`** — выделяет в куче ровно столько байт, сколько занимает тип, на который указывает `p` (в RARS это syscall **9** — `sbrk`, как в [документации RARS](https://github.com/TheThirdOne/rars/wiki/Environment-Calls)):

```
li a0, 8        # sizeof(Point) = 8
li a7, 9        # sbrk
ecall           # a0 ← адрес выделенного блока
```

### Поддержка

| Операция | Статус | Описание |
|---|---|---|
| `TYPE R = RECORD ... END` | Полная | Вычисление размера и смещений полей |
| `r.field` | Полная | Доступ к полям стековых и глобальных записей |
| `p.field` (POINTER TO) | Полная | Авто-разыменование + доступ к полю |
| `p^` (явное разыменование) | Полная | `lw a0, 0(a0)` |
| `NEW(p)` | Полная | sbrk с точным размером типа |
| `p := NIL` / `p = NIL` | Полная | NIL = 0 |
| `RECORD(Base)` | Частично | Разбор и разметка памяти учитывают базовый тип (`makeRecord` смещает поля после `Base`); поиск поля идёт через цепочку `baseRecord`. Нет полноценной поддержки на уровне языка (приведения типов, расширенной семантики Оберона для расширенных записей). |

### Ограничения

- **Нет освобождения памяти** — `DISPOSE` не реализован, нет GC.
- **Нет проверки на NIL** — разыменование нулевого указателя приведёт к ошибке RARS в runtime.
- **Расширение записей** — синтаксис `RECORD(Base)` поддержан для раскладки полей и доступа к полям через `findField`; тонкие случаи Оберона (строгая типизация расширений, теги) не реализованы.

---

## Реализованные возможности

| Возможность | Описание |
|---|---|
| Целые числа (`INTEGER`) | Арифметика, сравнения, I/O |
| Булевы (`BOOLEAN`) | `TRUE`, `FALSE`, `&`, `OR`, `~` |
| Массивы (`ARRAY N OF T`) | Индексация, `LEN`, вложенные массивы |
| Записи (`RECORD`) | Объявление типов, доступ к полям, размещение на стеке/в .data |
| Указатели (`POINTER TO`) | `NEW`, разыменование, авто-разыменование, `NIL` |
| Строковые литералы | Аргументы `Out.String`; несколько вызовов подряд |
| Процедуры | Рекурсия, параметры, `VAR`-параметры, возвращаемые значения |
| Управляющие конструкции | `IF`/`ELSIF`/`ELSE`, `WHILE`, `REPEAT`, `FOR`, `CASE` |
| Встроенные процедуры | `INC`, `DEC`, `ABS`, `ODD`, `ORD`, `CHR`, `LEN`, `NEW` |
| I/O | `Out.Int`, `Out.Ln`, `Out.String`, `Out.Char`, `Out.Real`; `In.Open`, `In.Int`, `In.Char`, `In.Line` (интерфейсы `stdlib/Out.obr`, `stdlib/In.obr` + `-M`) |
| Импорт модулей | `IMPORT M`, квалифицированный доступ `M.x`, экспорт `*`, поиск по `-M` путям |
| Стандартная библиотека | `Math` в `stdlib/`; интерфейсы `Out`, `In` в `stdlib/`, реализация syscall’ов в `src/runtime/` |

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
