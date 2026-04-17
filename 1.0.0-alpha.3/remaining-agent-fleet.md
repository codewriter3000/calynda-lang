# Alpha.3 Remaining Agent Fleet Plan

This file turns the remaining alpha.3 work into separate work packets that can be handed to multiple agents.

The goal is to parallelize the unfinished items without wasting time redoing slices that are already landed on the current branch.

## Current HEAD Status

Already landed on the current branch:

- `bug2` is satisfied via `.car` support in the `asm` CLI path.
- `bug7` is satisfied via `manual(args) -> { ... }` lambda shorthand.
- Manual-block return propagation was also fixed so returns inside `manual {}` contribute to the enclosing lambda or `start` block.

Completed-file anchors on the current branch:

- `compiler/src/cli/calynda_commands.c`
- `compiler/src/parser/parser_expr.c`
- `compiler/src/parser/parser_lookahead.c`
- `compiler/src/sema/type_checker/type_checker_block.c`
- `compiler/src/sema/type_checker/type_checker_internal.h`
- `compiler/src/sema/type_checker/type_checker_lambda.c`
- `compiler/tests/backend/tools/test_calynda_cli.c`
- `compiler/tests/backend/tools/test_calynda_cli_2.c`
- `compiler/tests/frontend/parser/test_parser.c`
- `compiler/tests/frontend/parser/test_parser_9.c`
- `compiler/tests/sema/checker/test_type_checker.c`
- `compiler/tests/sema/checker/test_type_checker_10.c`

Validated on the current branch:

- `cd compiler && make build/test_calynda_cli && ./build/test_calynda_cli`
- `cd compiler && make build/test_parser && ./build/test_parser`
- `cd compiler && make build/test_type_checker && ./build/test_type_checker`

## Remaining Alpha.3 Scope

Still open or intentionally deferred:

- `bug1`: freestanding string byte access
- `bug3`: freestanding imported/package-member lowering
- `bug4`: byte-buffer ergonomics
- `bug5`: string as `char[]`-style data path
- `bug6`: arrays-only `car()` / `cdr()` builtins

## Global Rules For Every Agent

- Treat `1.0.0-alpha.3/bug1.md` through `1.0.0-alpha.3/bug7.md` as desired outcomes, not authoritative proof of current breakage.
- Audit current HEAD for your assigned item before editing anything.
- Do not redo the completed `.car` asm work or the completed manual shorthand work unless your change must touch the same area for a real reason.
- Prefer the smallest change that fits the existing compiler architecture.
- Do not add an interpreter-style path or bypass the native-versus-bytecode split.
- Use targeted test binaries first. Run `./compiler/run_tests.sh` only after the narrow suite for your slice is green.
- `compiler/Makefile` does not track header dependencies well. If you change tokenizer or shared headers, use a clean rebuild for affected binaries.

Mandatory stop-and-ask semantics:

- What type `string[i]` should produce.
- Whether `string -> char[]` is implicit, explicit, or view-based.
- Whether `.length` should exist on all arrays or only specific lowered/runtime-backed forms.
- Whether `cdr()` returns a copied tail or a view/slice.
- Whether bug4's default-parameter note is actually in scope or should stay deferred.

## Recommended Split

Recommended agent packets:

1. Agent A: audit coordinator for remaining alpha.3 items.
2. Agent B: freestanding imported/package-member lowering (`bug3`).
3. Agent C: string and byte-buffer track (`bug1`, `bug4`, `bug5`).
4. Agent D: arrays-only `car()` / `cdr()` (`bug6`).

Concurrency notes:

- Agent B can run in parallel with Agent C.
- Agent D can run on a separate branch, but it is likely to conflict with Agent C in parser, type-checker, and lowering files.
- If you want low-conflict merging, run Agent D after Agent C settles the low-level array/string semantics.

## Agent A Prompt

```text
Work in /home/codewriter3000/Coding/calynda-lang.

Audit the remaining alpha.3 items against current HEAD: bug1, bug3, bug4, bug5, and bug6.

Completed items already landed on this branch:
- bug2: asm now accepts .car archives
- bug7: manual(args) -> { ... } shorthand is implemented

Your job:
1. Re-check current behavior for each remaining bug file.
2. Classify each item as one of:
   - still missing
   - partially solved
   - solved in a different form
3. For each item, name the current controlling files and one narrow test binary to use.
4. Produce a concise markdown status note for the rest of the agents.

Do not make broad code changes. This is an audit pass.

Key anchors:
- compiler/src/sema/type_checker/expr/type_checker_expr_more.c
- compiler/src/sema/type_checker/expr/type_checker_expr_memory.c
- compiler/src/sema/type_checker/expr/type_checker_expr_array.c
- compiler/src/hir/lower/hir_lower_expr.c
- compiler/src/mir/lower/mir_expr_helpers.c
- compiler/src/codegen/codegen_helpers.c
- compiler/src/runtime/runtime_ops.c
- compiler/tests/sema/checker/
- compiler/tests/backend/emit/

Suggested validation style:
- use targeted test binaries only
- no full-suite run unless you unexpectedly need code changes
```

## Agent B Prompt

```text
Work in /home/codewriter3000/Coding/calynda-lang.

Address the remaining freestanding imported/package-member lowering gap described by 1.0.0-alpha.3/bug3.md.

Current branch status:
- asm .car support is already done
- manual lambda shorthand is already done
- do not redo those slices

Goal:
When imported/package-member calls are statically known in freestanding-style code, lower them without hosted runtime member/call dispatch. If a case cannot be lowered statically yet, emit a targeted compile-time error instead of silently producing hosted-runtime symbols.

Start by auditing current HEAD behavior with a small imported helper call and confirm whether assembly still references:
- __calynda_pkg_*
- __calynda_rt_member_load
- __calynda_rt_call_callable

Most likely implementation anchors:
- compiler/src/sema/type_checker/expr/type_checker_expr_more.c
- compiler/src/hir/lower/hir_lower_expr.c
- compiler/src/mir/lower/mir_expr_helpers.c
- compiler/src/codegen/codegen_helpers.c
- compiler/src/codegen/codegen_names.c
- compiler/src/backend/runtime_abi/runtime_abi.c
- compiler/src/runtime/runtime_ops.c

Most likely regression anchors:
- compiler/tests/backend/emit/test_asm_emit.c
- compiler/tests/backend/emit/test_asm_emit_cross.c
- compiler/tests/backend/codegen/

Constraints:
- Prefer static lowering for compile-time-known callees.
- Do not introduce a new freestanding CLI flag unless that is clearly unavoidable.
- If you cannot lower a case safely, fail early with a precise diagnostic.

Validation:
1. cd compiler && make build/test_asm_emit build/test_asm_emit_cross
2. run the touched test binaries
3. if cross toolchains are missing, confirm the cross tests skip cleanly rather than fail
4. only after narrow tests pass, consider ./run_tests.sh
```

## Agent C Prompt

```text
Work in /home/codewriter3000/Coding/calynda-lang.

Handle the low-level string and byte-buffer track for bug1, bug4, and bug5.

Current branch status:
- asm .car support is already done
- manual lambda shorthand is already done
- do not touch those slices unless required for compile integration

Goal:
Establish one supported low-level text/byte path for freestanding code by auditing and then improving the current behavior around:
- string indexing
- addr(string)
- array .length ergonomics
- minimal byte-buffer usability
- any string-to-char[]-style bridge, if that is already implied by current design

Start with an audit against HEAD. Treat 1.0.0-alpha.3/bug1.md, bug4.md, and bug5.md as desired outcomes, not guaranteed current failures.

Use these active files, not shadow copies:
- compiler/src/sema/type_checker/expr/type_checker_expr_more.c
- compiler/src/sema/type_checker/expr/type_checker_expr_memory.c
- compiler/src/sema/type_checker/expr/type_checker_expr_array.c
- compiler/src/sema/type_checker/type_checker_types.c

You may also need to touch lowering/runtime only if the chosen semantics are already implied and checker-only changes are not enough.

Likely tests to extend:
- compiler/tests/sema/checker/test_type_checker.c
- compiler/tests/sema/checker/test_type_checker_2.c
- compiler/tests/frontend/parser/
- compiler/tests/backend/emit/
- compiler/tests/backend/emit/test_build_native*.c if you need end-to-end proof

Mandatory stop-and-ask points:
- what type string indexing returns
- whether string-to-char[] is implicit, explicit, or a view
- whether .length is universal on arrays or restricted to specific representations
- whether bug4's default-parameter note should be deferred instead of implemented

Strong preference:
- pick the smallest semantics already suggested by current code/tests
- do not widen language policy just to satisfy the old alpha.3 prose

Validation:
1. cd compiler && make build/test_type_checker build/test_parser
2. run the touched test binaries
3. if lowering/runtime changed, also run build/test_asm_emit or the narrowest end-to-end native test you touched
4. only after narrow tests pass, consider ./run_tests.sh
```

## Agent D Prompt

```text
Work in /home/codewriter3000/Coding/calynda-lang.

Implement the arrays-only car()/cdr() feature described in 1.0.0-alpha.3/bug6.md.

Current branch status:
- asm .car support is already done
- manual lambda shorthand is already done

Goal:
Add a minimal built-in arrays-only surface for:
- car(xs) -> first element
- cdr(xs) -> tail

Before editing, audit how current builtins and call handling are implemented so this can fit the existing language without adding unnecessary syntax.

Preferred design direction:
- treat car/cdr as built-in function names rather than new keywords unless there is a strong reason not to
- keep the feature arrays-only
- keep the implementation minimal and explicit

Likely anchors:
- compiler/src/sema/type_checker/expr/
- compiler/src/hir/lower/
- compiler/src/mir/lower/
- compiler/src/runtime/ if cdr needs runtime help
- compiler/src/parser/ only if syntax changes are truly necessary

Likely test anchors:
- compiler/tests/sema/checker/
- compiler/tests/ir/
- compiler/tests/backend/emit/

Mandatory stop-and-ask points:
- empty-array behavior
- whether cdr returns a copied tail or a view/slice

Merge warning:
- this packet is likely to overlap with the string/byte-buffer packet in parser, type-checker, and lowering code
- if low-conflict merging matters more than speed, wait until the string/byte-buffer packet settles core array/string semantics

Validation:
1. run the narrowest parser/type-checker tests for the new surface
2. run the narrowest IR/backend test needed for the chosen lowering path
3. only after narrow tests pass, consider ./run_tests.sh
```

## Suggested Merge Order

If you are merging multiple agent branches back together, use this order:

1. Agent A audit notes
2. Agent B freestanding lowering
3. Agent C string and byte-buffer work
4. Agent D car/cdr builtins

That order minimizes the chance that string/array semantics get reworked twice.
