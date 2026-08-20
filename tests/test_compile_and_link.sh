#!/bin/bash
# Regression test: the -o (compile-to-object, link, produce an executable)
# path. Until this fix it was completely broken on any modern PIE-by-default
# Linux toolchain (relocation errors from a non-PIC object) and, separately,
# relied on a hardcoded "clang++-13" linker name that doesn't exist once the
# installed LLVM/Clang version differs -- neither of which the golden-file
# suite exercises, since that only runs the default `pudl file.pudl`
# (interpret-via-lli) path.
#
# Usage: test_compile_and_link.sh <path-to-pudl-binary>

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <path-to-pudl-binary>" >&2
  exit 2
fi

BIN="$1"
cd "$REPO_ROOT"

OUT_EXE="./pudl_test_compile_and_link_exe"
rm -f "$OUT_EXE" temp.o TempLinker.cpp

"$BIN" examples/main.pudl -o "$OUT_EXE" >/dev/null 2>&1

if [ ! -x "$OUT_EXE" ]; then
  echo "FAIL: -o did not produce a runnable executable"
  rm -f "$OUT_EXE" temp.o TempLinker.cpp
  exit 1
fi

actual="$("$OUT_EXE")"
# The linker's generated wrapper (see Linker.h) is `int main() { std::cout
# << mast() << std::endl; }` -- mast()'s own return value (0) is always
# printed as a trailing line on top of whatever mast() itself printed.
expected="1
10
0"

rm -f "$OUT_EXE" temp.o TempLinker.cpp

if [ "$actual" != "$expected" ]; then
  echo "FAIL: compiled+linked executable produced wrong output"
  echo "--- expected ---"
  echo "$expected"
  echo "--- actual ---"
  echo "$actual"
  exit 1
fi

echo "PASS: -o compile+link+run produced the correct output"
exit 0
