#!/bin/bash
# Regression test: malformed programs that trigger a parser error path must
# be reported cleanly and exit normally, not crash the compiler.
#
# tests/regression/undefined_function.pudl and undeclared_assignment.pudl
# each exercise a null-pointer dereference that used to exist in
# Parser::funcall() and Parser::assignment() respectively.
# tests/regression/unary_type_mismatch.pudl exercises Parser::unary()'s
# type-mismatch guard, which used to check the wrong token and so never
# actually fired.
# tests/regression/codegen_error_no_run.pudl exercises a codegen-level
# error (wrong argument count to a function call, which the parser never
# checks) that used to be reported but not actually stop the (broken)
# program from being run anyway -- it must now refuse to run at all.
# tests/regression/malformed_top_level_token.pudl exercises an infinite
# loop (found by fuzzing, see fuzz/fuzz_pipeline.cpp) in
# Parser::parse()'s top-level while(1): a token that isn't `func` used to
# hit the default: case and print "unexpected token" forever, because
# nothing ever advanced the lexer past it. Every check() below runs under
# `timeout` for exactly this reason -- a hang must fail the test loudly
# instead of wedging CI indefinitely.
# tests/regression/binary_operator_missing_rhs.pudl exercises a
# null-pointer dereference (also found by fuzzing) shared by every binary
# precedence method (lor/land/cmpeq/cmp/additive/multiplicative): none of
# them checked whether their own lhs came back NULL (from a deeper parse
# failure) before dereferencing lhs->getType().
# tests/regression/integer_literal_overflow.pudl exercises an uncaught
# std::out_of_range crash (also found by fuzzing): Parser::intgr() called
# std::stoi() with no try/catch.
#
# Usage: test_parser_error_recovery.sh <path-to-pudl-binary>

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <path-to-pudl-binary>" >&2
  exit 2
fi

BIN="$1"
cd "$REPO_ROOT"

fail=0

check() {
  local name="$1" src="$2" expected_text="${3:-}" forbidden_text="${4:-}"
  local output
  # 10s is generous for these tiny programs; it exists purely so a hang
  # (see malformed_top_level_token.pudl above) fails this test instead of
  # blocking the whole CI job forever.
  output="$(timeout 10 "$BIN" "$src" 2>&1)"
  local status=$?
  if [ "$status" -eq 124 ]; then
    echo "FAIL: $name hung (timed out after 10s)"
    fail=1
    return
  fi
  # A process killed by a signal (segfault, abort, ...) reports exit status
  # 128+signal under bash; a clean (even error-reporting) exit never does.
  if [ "$status" -ge 128 ]; then
    echo "FAIL: $name crashed (signal $((status - 128)), exit code $status)"
    fail=1
    return
  fi

  if [ -n "$expected_text" ] && [[ "$output" != *"$expected_text"* ]]; then
    echo "FAIL: $name did not report the expected error ('$expected_text')"
    fail=1
    return
  fi

  if [ -n "$forbidden_text" ] && [[ "$output" == *"$forbidden_text"* ]]; then
    echo "FAIL: $name unexpectedly did '$forbidden_text'"
    fail=1
    return
  fi

  echo "PASS: $name did not crash (exit code $status)"
}

check "undefined function call" "tests/regression/undefined_function.pudl"
check "undeclared variable assignment" "tests/regression/undeclared_assignment.pudl"
check "unary ! type mismatch" "tests/regression/unary_type_mismatch.pudl" "expected boolean but given number"
check "codegen error must not run" "tests/regression/codegen_error_no_run.pudl" \
  "Codegen failed; not compiling, linking, or running." "Executing -----------------------"
check "malformed top-level token must not hang" "tests/regression/malformed_top_level_token.pudl" \
  "unexpected token"
check "binary operator with missing rhs" "tests/regression/binary_operator_missing_rhs.pudl" \
  "expression expected after"
check "integer literal overflow" "tests/regression/integer_literal_overflow.pudl" \
  "is out of range"

exit $fail
