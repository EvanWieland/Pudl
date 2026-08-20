// libFuzzer harness for the lexer -> parser -> AST -> codegen pipeline.
//
// Deliberately stops short of linking/running: those steps shell out to an
// external linker/lli, which is slow, environment-dependent, and not code
// this harness is trying to stress -- everything interesting for a
// fuzzer to find (crashes in the hand-written lexer/parser, or in
// Codegen's IR construction) happens before that point.
//
// Build (needs clang -- libFuzzer isn't available for GCC/MSVC; the
// -fsanitize=fuzzer,... flags live on the pudl_fuzz_pipeline CMake target
// itself, not on the command line -- see CMakeLists.txt):
//   cmake -S . -B build-fuzz -G Ninja -DLLVM_DIR=<...> \
//     -DPUDL_ENABLE_FUZZING=ON -DCMAKE_CXX_COMPILER=clang++-18
//   cmake --build build-fuzz --target pudl_fuzz_pipeline
//
// Run (60s bounded smoke run, matching what CI does on every push).
// fuzz/corpus is checked into git as a curated seed set (the
// examples/*.pudl programs) and should stay small and readable -- point
// libFuzzer at a throwaway scratch directory as the first (writable) arg
// so the inputs it discovers while mutating don't turn into thousands of
// generated files under version control; fuzz/corpus after that is a
// read-only additional seed directory:
//   mkdir -p /tmp/pudl-fuzz-scratch
//   ./build-fuzz/pudl_fuzz_pipeline -max_total_time=60 \
//     /tmp/pudl-fuzz-scratch fuzz/corpus
//
// See DEVELOPING.md for the full explanation of what this is and isn't
// meant to catch.

#include <cstdint>
#include <cstdio>

#include "Parser/Codegen.h"
#include "Parser/Parser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *aData, size_t aSize) {
    // fmemopen wraps the fuzzer-owned buffer as a FILE* with no disk I/O --
    // Parser::parse() takes ownership of it via its Lexer member and
    // fclose()s it when the Parser (a local here) is destroyed.
    FILE *file = fmemopen(const_cast<uint8_t *>(aData), aSize, "r");
    if (file == nullptr) {
        return 0;
    }

    Parser parser;
    Node *root = parser.parse(file);

    if (parser.isFailed() || root == nullptr) {
        return 0;
    }

    Codegen codegen;
    root->accept(codegen);

    return 0;
}
