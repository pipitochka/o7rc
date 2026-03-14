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
- [Поддерживаемый синтаксис](#поддерживаемый-синтаксис)
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
## Поддерживаемый синтаксис
TODO