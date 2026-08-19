# Known-broken examples

Examples listed here have a recorded golden file that captures their
*current, buggy* output, not correct output. A mismatch against these
examples is reported by the golden-test runner but does not fail the test
run. Once the underlying bug is fixed, remove the entry, re-record the
golden file (`--record`/`-Record`), and confirm the new output is correct.

- `ex9` — `examples/ex9.pudl` (non-recursive Fibonacci via a `while` loop) is
  marked `# NOT WORKING` in the source itself. **Confirmed (2026-08-19,
  local build against LLVM 18.1.8 on Linux/WSL): the compiler itself
  segfaults** generating/optimizing this program — it crashes after printing
  the optimization-pass banner and before ever reaching the "Executing"
  step, so `tests/golden/ex9.expected.txt` captures only that truncated
  partial output (this is the actual current behavior, not a harness bug).
  Likely cause: `Codegen.h::visit(WhileStatementNode)` attaches its
  `BasicBlock`s to the function only *after* codegen has already branched
  into them (unlike the equivalent `if`-statement path, which attaches
  immediately) — the optimizer then runs on a function LLVM's own basic
  block bookkeeping doesn't consider fully valid, which is a very plausible
  segfault trigger. See the project hardening plan, Phase 4, item 1. Confirm
  the exact mechanism with a debugger/`-p`/`--print-ir` when fixing, then
  re-record this golden file.
