#!/bin/bash
# Regression test for the command-injection fix in Linker.h/Codegen.h.
#
# Before the fix, the linker/run steps built a shell command string via
# naive concatenation of user-controlled CLI values (-o, -l, the input
# file) and handed it to std::system(). Any shell metacharacter in one of
# those values was arbitrary command execution. Now every invocation goes
# straight to the child process's argv with no shell involved, so a value
# like "foo; touch marker" is just a single (nonsensical, harmless)
# argument/filename -- never split and interpreted.
#
# Usage: test_no_shell_injection.sh <path-to-pudl-binary>

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <path-to-pudl-binary>" >&2
  exit 2
fi

BIN="$1"
MARKER="$SCRIPT_DIR/injection_marker_$$"
rm -f "$MARKER"

cd "$REPO_ROOT"

# A shell-interpreting implementation would run `touch $MARKER` as a
# separate command here (once via the -o value, once via -l); an
# argv-based one just fails to find/execute an oddly-named program/file
# and moves on.
"$BIN" examples/main.pudl -o "pwned_out; touch $MARKER" -l "clang++-13; touch $MARKER" >/dev/null 2>&1

status=0
if [ -f "$MARKER" ]; then
  echo "FAIL: a shell metacharacter in a CLI value executed an injected command"
  status=1
else
  echo "PASS: no shell injection via -o/-l values"
fi

rm -f "$MARKER" "pwned_out; touch $MARKER" TempLinker.cpp temp.o
exit $status
