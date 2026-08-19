# Known-broken examples

Examples listed here have a recorded golden file that captures their
*current, buggy* output, not correct output. A mismatch against these
examples is reported by the golden-test runner but does not fail the test
run. Once the underlying bug is fixed, remove the entry, re-record the
golden file (`--record`/`-Record`), and confirm the new output is correct.

- `ex9` — `examples/ex9.pudl` (non-recursive Fibonacci via a `while` loop) is
  marked `# NOT WORKING` in the source itself. Root cause is believed to be
  in `Codegen.h::visit(WhileStatementNode)` (basic blocks are attached to the
  function after codegen runs into them, unlike the equivalent `if`-statement
  path) — see the project hardening plan, Phase 4, item 1. Confirm the exact
  divergence with `-p`/`--print-ir` once building against LLVM 18 works, fix,
  then re-record this golden file.
