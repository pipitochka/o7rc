(macOS)
```bash
brew install bison flex build-essential
```
(Linux)


(Docker)
```
apt-get update && \
   DEBIAN_FRONTEND=noninteractive apt-get install -y \
        build-essential \
        cmake \
        flex \
        bison \
        git \
        && rm -rf /var/lib/apt/lists/*
```