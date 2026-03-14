### Запуск

(macOS)
```bash
brew install bison flex build-essential
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
    
mkdir build && cd build
cmake .. && make    
```

(Docker)
```shell
docker build -t o7rc .
docker run -it --rm -v "$(pwd)":/work o7rc
mkdir build && cd build
cmake .. && make
```

### Тестирование 
(macOS)
```bash
brew install bison flex build-essential
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
    
mkdir tests && cd tests
cmake --build . --target o7rc_tests
ctest --output-on-failure
```

(Docker)
```shell
docker build -t o7rc .
docker run -it --rm -v "$(pwd)":/work o7rc
mkdir build && cd build
cmake .. && make
```