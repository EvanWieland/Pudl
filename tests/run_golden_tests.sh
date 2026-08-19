#!/bin/bash
# Golden-file regression tests for Pudl example programs.
#
# Usage:
#   run_golden_tests.sh <path-to-pudl-binary> [--record] [--ir]
#
# Default mode: for each examples/*.pudl, runs the binary and diffs its
# combined stdout+stderr against tests/golden/<name>.expected.txt, failing
# (nonzero exit) on any mismatch that isn't listed in KNOWN_BROKEN.md.
#
# --record: (re)writes the current output as the new golden file for every
#           example instead of comparing. Run this once against a known-good
#           build to establish/refresh the baseline, then commit the results.
#
# --ir: instead of the full example set, runs a small representative subset
#       with `-p` (print IR) and snapshots the emitted LLVM IR to
#       tests/golden-ir/<name>.ir.txt, to catch codegen/optimization
#       regressions that don't show up in program stdout.
#
# A mismatch for a name listed in KNOWN_BROKEN.md is reported but does not
# fail the run — those examples are tracked bugs, not regressions, until
# fixed (at which point remove them from KNOWN_BROKEN.md and re-record).

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
EXAMPLES_DIR="$REPO_ROOT/examples"
GOLDEN_DIR="$SCRIPT_DIR/golden"
GOLDEN_IR_DIR="$SCRIPT_DIR/golden-ir"
KNOWN_BROKEN_FILE="$SCRIPT_DIR/KNOWN_BROKEN.md"

# Representative subset for --ir snapshotting (kept small; IR output is
# verbose and sensitive to every codegen/optimization-pipeline change).
IR_SUBSET="main ex1 ex5"

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <path-to-pudl-binary> [--record] [--ir]" >&2
  exit 2
fi

BIN="$1"
shift
RECORD=0
IR_MODE=0
for arg in "$@"; do
  case "$arg" in
    --record) RECORD=1 ;;
    --ir) IR_MODE=1 ;;
    *) echo "unknown argument: $arg" >&2; exit 2 ;;
  esac
done

if [ ! -x "$BIN" ] && [ ! -f "$BIN" ]; then
  echo "error: pudl binary not found: $BIN" >&2
  exit 2
fi

is_known_broken() {
  local name="$1"
  [ -f "$KNOWN_BROKEN_FILE" ] && grep -qi "\`$name" "$KNOWN_BROKEN_FILE"
}

check_or_record() {
  local name="$1" expected="$2" actual="$3"

  if [ "$RECORD" -eq 1 ]; then
    printf '%s' "$actual" > "$expected"
    echo "recorded: $name"
    return 0
  fi

  if [ ! -f "$expected" ]; then
    echo "FAIL: $name (no golden file recorded — run with --record first)"
    return 1
  fi

  local expected_content
  expected_content="$(cat "$expected")"
  if [ "$actual" = "$expected_content" ]; then
    echo "PASS: $name"
    return 0
  fi

  if is_known_broken "$name"; then
    echo "KNOWN-BROKEN (mismatch, not failing): $name"
    return 0
  fi

  echo "FAIL: $name"
  echo "--- expected ---"
  echo "$expected_content"
  echo "--- actual ---"
  echo "$actual"
  return 1
}

fail=0

if [ "$IR_MODE" -eq 1 ]; then
  mkdir -p "$GOLDEN_IR_DIR"
  for name in $IR_SUBSET; do
    src="$EXAMPLES_DIR/$name.pudl"
    if [ ! -f "$src" ]; then
      echo "FAIL: $name (source not found: $src)"
      fail=1
      continue
    fi
    actual="$("$BIN" "$src" -p 2>&1)"
    check_or_record "$name" "$GOLDEN_IR_DIR/$name.ir.txt" "$actual" || fail=1
  done
else
  mkdir -p "$GOLDEN_DIR"
  for src in "$EXAMPLES_DIR"/*.pudl; do
    [ -e "$src" ] || continue
    name="$(basename "$src" .pudl)"
    actual="$("$BIN" "$src" 2>&1)"
    check_or_record "$name" "$GOLDEN_DIR/$name.expected.txt" "$actual" || fail=1
  done
fi

exit $fail
