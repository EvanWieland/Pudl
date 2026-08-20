# Known-broken examples

Examples listed here have a recorded golden file that captures their
*current, buggy* output, not correct output. A mismatch against these
examples is reported by the golden-test runner but does not fail the test
run. Once the underlying bug is fixed, remove the entry, re-record the
golden file (`--record`/`-Record`), and confirm the new output is correct.

None currently. (`ex9`'s while-loop segfault and `ex13`'s `&&`/`||`
short-circuit bug were both fixed in Phase 4 of the project hardening
plan — see git history for `examples/ex9.pudl`, `examples/ex13.pudl`, and
`src/Parser/Codegen.h`'s `visit(WhileStatementNode)`/`bilogShortCircuit()`
if you need the detail.)
