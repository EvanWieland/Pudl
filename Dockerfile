FROM debian:bookworm-slim

RUN set -xe && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    lsb-release \
    wget \
    software-properties-common \
    gnupg \
    cmake \
    build-essential \
    zlib1g-dev \
    libedit-dev \
    llvm-runtime \
    clang-13

RUN set -xe && \
    wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 14

WORKDIR /usr/src/pudl

COPY . .

RUN set -xe && \
    mkdir build && \
    cd build && \
    cmake .. && \
    cmake --build .