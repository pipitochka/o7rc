FROM ubuntu:24.04

LABEL authors="artemakulov"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    flex \
    bison \
    git \
    pkg-config \
    gdb \
    procps \
    ca-certificates \
    libfl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work

CMD ["bash"]
