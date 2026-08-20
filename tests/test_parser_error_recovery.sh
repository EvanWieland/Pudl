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
  output="$("$BIN" "$src" 2>&1)"
  local status=$?
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

exit $fail
