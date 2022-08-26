<p align="center">
<img width="300" src="https://github.com/EvanWieland/Pudl/blob/master/brand/pudl-logo-light.png?raw=true">
</p>

Pudl is a friendly programming language that aims to allow low-level operations in an intuitive way.

## About Pudl

Pudl is written using [LLVM](http://llvm.org/), the amazing compiler project.

## Getting started

### 1. Install dependencies

```sh
sudo apt-get update && \
sudo apt-get install -y --no-install-recommends \
    ca-certificates \
    lsb-release \
    wget \
    software-properties-common \
    gpg-agent \
    libedit-dev \
    uuid-dev \
    zlib1g-dev \
    build-essential \
    pkg-config \
    cmake \
    git && \
wget https://apt.llvm.org/llvm.sh && \
chmod +x llvm.sh && \
sudo ./llvm.sh 12
```

### 2. Clone project

```sh
git clone https://github.com/EvanWieland/pudl
```

### 3. Build project

```sh
./compile.sh
```

## Usage

```
./run.sh <file> [--help,-h] [--print-llvm] [--compile,-c [-o <path (default: output.o)>]]

Options:
    -h, --help              Get usage and available options
    -c, --compile           Outputs to an object file. By default the program is using the JIT
    -o                      Filename used along with --compile option. Default is output.o
        --print-llvm        Print generated LLVM IR bytecode
```

### Example usage

```sh
./run.sh hello_world.pudl --print-llvm
```
