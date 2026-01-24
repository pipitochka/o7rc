(macOS)
```bash
brew install bison flex build-essential
```
(Linux)


(Docker)
```shell
docker build -t o7rc .
docker run -it --rm -v "$(pwd)":/work o7rc
mkdir build && cd build
cmake .. && make
```