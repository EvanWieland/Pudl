FROM debian:bookworm-slim

RUN set -xe && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    lsb-release \
    wget \
    software-properties-common \
    gnupg \
    cmake \
    ninja-build \
    build-essential \
    zlib1g-dev \
    libedit-dev

# Debian bookworm's own repos don't carry LLVM 18, so add apt.llvm.org's
# repo for it via llvm.sh, then install exactly the packages verified to
# work in CI (.github/workflows/ci.yml's Linux leg installs the same set):
# llvm-18-dev/clang-18 for the compiler itself, libpolly-18-dev because
# CMakeLists.txt links the Passes/ipo components, which pull in libPolly.
RUN set -xe && \
    wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 18 && \
    apt-get install -y --no-install-recommends \
    llvm-18-dev \
    clang-18 \
    libpolly-18-dev

WORKDIR /usr/src/pudl

COPY . .

RUN set -xe && \
    mkdir build && \
    cd build && \
    cmake -G Ninja -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm .. && \
    cmake --build .
