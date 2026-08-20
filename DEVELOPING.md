# Developing Pudl

Build/test/run cheat sheet, so building this doesn't require re-deriving
the commands from scratch each time.

## Build

Needs CMake 3.18+, Ninja, and LLVM 18 (dev headers + libraries).

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

`find_package(LLVM)` usually locates an apt-installed LLVM 18 without
help. If you have more than one LLVM version installed, or it can't be
found, point at it explicitly:

```sh
cmake -S . -B build -G Ninja -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm
```

On Windows, force MSVC rather than letting CMake pick up a bundled
`clang++.exe` from the LLVM install (it's usually too old for the
installed MSVC STL):

```powershell
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe
```

The build produces two targets: `pudl_core` (a static library with
everything except the CLI) and `pudl` (the thin CLI executable that links
it). `./build/pudl --help` / `--version` are worth running once after any
build to sanity-check the binary works.

## Test

```sh
cd build && ctest --output-on-failure
```

Five suites: golden-file stdout snapshots for every `examples/*.pudl`,
IR-level snapshots for a representative subset, a no-shell-injection
check, parser-error/crash regression tests, and a compile+link+run
end-to-end check. All of them run in CI (`.github/workflows/ci.yml`) on
both Linux and Windows for every push.

If you change output intentionally (a new example, a fixed bug that
changes what a program prints), re-record the golden files against a
known-good build before committing:

```sh
bash tests/run_golden_tests.sh build/pudl --record
bash tests/run_golden_tests.sh build/pudl --record --ir
```

(`tests/run_golden_tests.ps1 -Bin build/pudl.exe -Record` on Windows.)

## Sanitizer build

Not wired into the default build (it's slower and Debug-only) but runs on
every push in CI's `sanitize` job. To reproduce locally:

```sh
cmake -S . -B build-sanitize -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all -g -O1" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-sanitize
cd build-sanitize && ASAN_OPTIONS=detect_leaks=1 ctest --output-on-failure
```

## Fuzzing

`fuzz/fuzz_pipeline.cpp` is a libFuzzer harness that feeds random bytes
through the lexer -> parser -> AST -> codegen pipeline (everything except
linking/running, which shells out to an external tool). It found three
real bugs the first time it ran for about two minutes: an infinite loop
in the top-level parser, a null-pointer dereference shared by every
binary-operator precedence method, and an uncaught `std::out_of_range`
crash on an oversized integer literal. It runs as a 60s bounded smoke
test in CI's `fuzz` job on every push.

Needs clang (libFuzzer isn't available for GCC/MSVC) plus its runtime:

```sh
sudo apt-get install -y llvm-18-dev clang-18 libpolly-18-dev libclang-rt-18-dev libfuzzer-18-dev
```

Build (a separate build directory -- `PUDL_ENABLE_FUZZING` shouldn't be
part of your regular build):

```sh
cmake -S . -B build-fuzz -G Ninja -DPUDL_ENABLE_FUZZING=ON \
  -DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18
cmake --build build-fuzz --target pudl_fuzz_pipeline
```

Run (a bounded local session -- drop `-max_total_time` to fuzz
indefinitely until you stop it):

```sh
mkdir -p /tmp/pudl-fuzz-scratch
./build-fuzz/pudl_fuzz_pipeline -max_total_time=120 -timeout=5 \
  /tmp/pudl-fuzz-scratch fuzz/corpus
```

`fuzz/corpus` is a curated seed set (copies of `examples/*.pudl`) checked
into git -- pass it as a **read-only additional** seed directory (second
positional arg), never as the first/writable one, or libFuzzer will dump
thousands of generated files into it. The scratch directory is where new
coverage-increasing inputs actually accumulate; it isn't meant to be kept.

If it finds something, it writes `crash-<hash>` in the current directory
and prints a stack trace. To turn a crash into a permanent regression
test: minimize it to a short, readable `.pudl` file under
`tests/regression/`, add a `check`/`Test-NoCrash` call in
`tests/test_parser_error_recovery.sh` and `.ps1`, then delete the raw
`crash-*` artifact (don't commit it -- it's usually full of non-printable
mutation garbage; the point is the readable minimized fixture).

## Versioning

Single source of truth is the repo-root `VERSION` file (just a bare
`X.Y.Z`), configured into `Version.h` at build time and surfaced via
`pudl --version`/`-v`. Bump it there; nothing else needs to change.
