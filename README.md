# o7rc

**o7rc** — кросс-компилятор языка Oberon-7 в машинно-ориентированный ассемблер архитектуры RISC-V.

## Оглавление
- [Функционал](#функционал)
- [Начало работы](#начало-работы)
    - [Быстрый старт](#быстрый-старт)
    - [Полное руководство](#полное-руководство)
        - [1. Зависимости](#1-зависимости)
        - [2. Сборка](#2-сборка)
        - [3. Запуск](#3-запуск)
        - [4. Тестирование](#4-тестирование)
- [Синтаксис oberon7](#синтаксис-oberon7)
---

## Функционал
TODO
---

## Начало работы
### Быстрый-старт
```shell
docker build -t o7rc .
docker run -it --rm -v "$(pwd)":/work o7rc
mkdir build && cd build
cmake .. && make
./o7rc
```

### Полное руководство
#### 1. Зависимости

(macOS)
```bash
xcode-select --install
brew install bison flex cmake
```

(Linux)
```shell
apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    flex \
    bison \
    pkg-config \
    gdb \
    procps 
```

(Docker)
```shell
docker build -t o7rc .
```

#### 2. Сборка

(Локально)
```shell
mkdir build && cd build
cmake .. && make    
```

(Docker)
```shell
docker run -it --rm -v "$(pwd)":/work o7rc
mkdir build && cd build
cmake .. && make
```

#### 3. Запуск

```shell
./o7rc
```

#### 4. Тестирование

```shell
./o7rc_tests
```

---
## Синтаксис oberon7

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