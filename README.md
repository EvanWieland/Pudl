<p align="center">
<img width="300" src="https://github.com/EvanWieland/Pudl/blob/master/brand/pudl-logo-light.png?raw=true">
</p>

Pudl is a simple, lightweight, and easily customizable programming language.

## About Pudl

Pudl is written using [LLVM](http://llvm.org/), the amazing compiler project. I've built this project off of the
great work done by [Max Balushkin](https://github.com/NoxChimaera). His approach to building a language without
relying on a third party parser/lexer combo - such as Bison/Flex, ANTLR, etc. - intrigued me. Although, I'm changing a
lot of the code to fit my needs, Balushkin's work still gave Pudl a huge jump start.

Expect more changes to Pudl in the future. This project is entirely for fun, so progress may be slow.

## Example

#### Pudl source code

```pudl
func inc( int a ) : int return a + 1

func mast : int {
  print inc( 5 )
  return 0
}
```

#### LLVM IR output (after optimization passes)

```llvm
; ModuleID = 'pudl compiler'
source_filename = "pudl compiler"

@.formati = private constant [4 x i8] c"%d\0A\00"
@.formatf = private constant [4 x i8] c"%f\0A\00"

declare i32 @printf(i8* %0, ...)

define i32 @inc(i32 %a) {
entry:
  %0 = add i32 %a, 1
  ret i32 %0
}

define i32 @mast() {
entry:
  %0 = call i32 @inc(i32 5)
  %1 = call i32 (i8*, ...) @printf(i8* noundef nonnull dereferenceable(1) getelementptr inbounds ([4 x i8], [4 x i8]* @.formati, i64 0, i64 0), i32 %0)
  ret i32 0
}
```

## Getting started

### Docker (Easy)

#### 1. Clone project
```sh
git clone https://github.com/EvanWieland/pudl
```

#### 2. Build Docker image
```sh
docker build . -t pudl
```

#### 3. Run Docker container
```sh
docker run --rm -it pudl
```

#### 4. Test Pudl
```sh
./build/pudl ./examples/main.pudl
```

```
.--------------------------------.
| Pudl Language Compiler v.0.0.1 |
'--------------------------------'
Loading source file ./examples/main.pudl
No optimization level specified, using level -Oall
Optimization: instruction combining, 
         promote allocas to registers, 
         reassociate, 
         dead code elimination, 
         global value numbering, 
         simplify CFG

Executing -----------------------

1
10
```

### Manual (Prolly Hard)

#### 1. Install dependencies

```sh
sudo apt-get update && \
sudo apt-get install -y --no-install-recommends \
    lsb-release \
    wget \
    software-properties-common \
    gnupg \
    cmake \
    build-essential \
    zlib1g-dev \
    libedit-dev \
    llvm-runtime \
    clang-13 \
    git && \
wget https://apt.llvm.org/llvm.sh && \
chmod +x llvm.sh && \
sudo ./llvm.sh 14
```

#### 2. Clone project

```sh
git clone https://github.com/EvanWieland/pudl
```

#### 3. Build project

```sh
./compile.sh
```

#### 4. Test Pudl
```sh
./build/pudl ./examples/main.pudl
```

## Usage

```sh 
./pudl.sh <file> [--help,-h]  [-p,--print-ir <out file>] [-c,--compile <out file>] [-o, --output <out file>] [-O<N>] [-l,--linker <linker>]

Options:
    <file>                The file to run.
                          - Source file
                          - Compiled object file
                          
    -c, --compile         Compile the source file to an object file
    -o, --output          Create an executable output file
    -l, --linker          Linker command to call when linking object files.
                              - Default: clang++-13  
    -h, --help            Get usage and available options
    -p  --print-ir        Print generated LLVM IR to stdout or to a file
    -d, --debug           Print debug information

    -O<N>                 Optimization level
                              - Default: OAll
                              - None: No optimizations
                              - 0: No optimizations
                              - 1: Promote allocas to registers
                              - 2: Reassociate expressions
                              - 3: Dead code elimination
                              - 4: Global value numbering
                              - 5: Simplify control flow graph (CFG)
                              - All: All optimizations
```

### Running Pudl

#### Source file

```sh
./pudl ./examples/main.pudl
```

#### Compiled pudl file

```sh
./pudl ./examples/main.pudl -c # outputs: ./examples/main.o
```

```sh
./debug/pudl main.o 
```

### Compiling object files

```sh
./debug.sh ./examples/main.pudl -c main.o
```

### Linking and Executing object files

#### Linking from source

```sh
./debug/pudl ./examples/main.pudl -o main -l clang++-13
```

#### Linking from pre-compiled object file

```sh
./debug/pudl ./main.o -o main -l clang++-13
```

#### Executing

```sh
./main
```

> Note: The `-l` flag is used to specify the linker to use.

#### Linking compiled object files to other languages other than Pudl, such as C++

```sh
# Compile Pudl source file to object file
./debug.sh ./examples/main.pudl -c main.o
```

```sh
# Link compiled Pudl object file to some C++ source file
clang++-13 ./examples/link.cpp main.o -o main
```

```sh
# Execute the compiled program
./main
```