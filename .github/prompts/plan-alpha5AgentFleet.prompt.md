# Plan: alpha5 Agent Fleet

13 items → 8 scoped agents across 3 phases. Agents within a phase are fully parallel.

---

## Phase 1 — 4 agents, all parallel

### Agent A — Boot/Start/Version *(small, frontend only)*
1. `compiler/calynda.ebnf` — drop `"(" ")"` from `BootDecl`
2. `compiler/src/parser/parser_decl.c` `parse_boot_decl()` — remove `TOK_LPAREN`/`TOK_RPAREN` consumption
3. `compiler/src/sema/type_checker/type_checker_lambda.c` lines 145-151 — allow 0-arg `start` (no `string[] args` required) and allow void return from `start`
4. `compiler/src/cli/calynda_metadata.h` line 5 — `"1.0.0-alpha.2"` → `"1.0.0-alpha.5"`
5. Update parser + type checker tests for `boot -> {}` and bare `start -> {}` forms

---

### Agent C — CAR Dependency Archives *(alpha5feature1.md — `--archive lib.car` flag)*
1. `compiler/src/cli/calynda.c` — add repeatable `--archive path.car` / `--archive-path dir/` flags; thread through build/run/asm pipelines
2. `compiler/src/car/car.h` + `car_read.c` — add API to enumerate exported symbols from a CAR by package name
3. `compiler/src/sema/symbol_table/` — `symbol_table_load_archive_deps()` pre-populates external package symbols from dep archives before predeclaration
4. Tests: resolve an import from a dep CAR archive end-to-end

---

### Agent D — Swap Operator `><`
1. `compiler/src/tokenizer/tokenizer.h` + lexer — add `TOK_SWAP`; lex `><` as a 2-char token
2. `compiler/src/parser/parser_stmt.c` — parse `expr >< expr ;` as a swap statement
3. `compiler/src/ast/ast_types.h` + `ast_statements.c` — add `AST_STMT_SWAP` node
4. Type checker — both sides must be assignable lvalues of matching types; result is void
5. HIR lowering — expand to: `temp = lhs; lhs = rhs; rhs = temp`
6. MIR/LIR — handled by generic load/store; no special case
7. Tests: parser, type checker, HIR dump, build_native (array element swap)

---

### Agent E — typeof / isint / issametype builtins
1. `compiler/src/sema/type_checker/expr/type_checker_expr_ext.c` — special-case callee names `typeof`, `isint`, `isfloat`, `isbool`, `isstring`, `isarray`, `issametype` in call handling; return types: `typeof` → `string`, all `is*` → `bool`
2. `compiler/src/mir/mir.c` — map builtin calls to runtime helper calls (`__calynda_typeof`, etc.)
3. `compiler/src/backend/runtime_abi/runtime_abi.h` — register new helper symbols
4. `compiler/src/runtime/runtime.c` + `runtime.h` — implement helpers (base type name, same-type check)
5. `compiler/src/bytecode/` — add bytecode ops for these builtins
6. Tests: type checker + build_native end-to-end

---

## Phase 2 — 3 agents, partially parallel *(Agent B waits for Agent C; F and G are independent)*

### Agent B — Wildcard Import Fix *(depends on Agent C)*

Key issue: `symbol_table_imports.c` lines 124-133 records wildcard imports but never injects names. Fix: after dep archive symbols are loaded, expand wildcard imports.

1. `compiler/src/sema/symbol_table/symbol_table_imports.c` — implement `scope_inject_wildcard_package()` that injects exported package symbols as aliases; call it after dep archives are loaded
2. `compiler/src/sema/type_checker/` — fallback lookup through wildcard-imported packages for unresolved names
3. Tests: `import pkg.*;` + direct unqualified call via parser, type checker, build_native

---

### Agent F — Optional Parameters *(parallel with B and G)*
1. `compiler/calynda.ebnf` — `ParameterDecl`: add `[ "=" Expression ]`
2. `compiler/src/parser/parser_decl.c` — parse default expression into new `AstParameter.default_expr` field
3. `compiler/src/ast/ast_types.h` — add `AstExpression *default_expr` to `AstParameter`
4. `compiler/src/sema/type_checker/` — allow fewer args if trailing params have defaults; validate default expr types
5. `compiler/src/hir/hir.c` — synthesize default-expression HIR nodes at call sites with missing args (defaults are inlined at call site, not at declaration)
6. Tests: parser, type checker, HIR dump, build_native

---

### Agent G — Direct Self-TCO *(parallel with B and F)*

Strategy: MIR rewrite pass — detect `MIR_INSTR_CALL(self) → MIR_TERM_RETURN` pattern; replace with param-reassign + jump to entry block.

1. New `compiler/src/mir/mir_tco.c` — `mir_apply_self_tco(MirUnit*)` pass
2. `compiler/src/mir/mir.h` — expose the pass
3. Invoke pass in MIR pipeline after all units are built
4. Add synthetic entry-landing block if entry block is not already a stable jump target
5. Tests: MIR dump showing tail call → jump rewrite; build_native: 10M self-recursive iterations without stack overflow
6. **Mutual TCO deferred** — needs trampoline design, post-alpha5

---

## Phase 3 — 1 agent *(after Phase 2)*

### Agent H — Function Overloading *(most invasive — goes last)*

Currently duplicate names are rejected in `symbol_table_predecl.c`. Fix: group same-name bindings into overload sets.

1. `compiler/src/sema/symbol_table/symbol_table.h` — introduce `OverloadSet` struct (named group of `Symbol*`)
2. `compiler/src/sema/symbol_table/symbol_table_predecl.c` — instead of rejecting duplicates, accumulate into overload sets
3. `compiler/src/sema/type_checker/expr/type_checker_expr_ext.c` — when callee resolves to an overload set: run resolution by arity → exact type match → widening; error on ambiguity
4. `symbol_table_find_resolution()` — return unresolved-overload-set until call-site selects one `Symbol*`
5. Downstream (HIR/MIR): call sites carry the already-resolved `Symbol*`, no change needed
6. Tests: overload set storage, exact + widening dispatch, ambiguity error, arity selection, HIR/build_native end-to-end

---

## Verification
1. `./compiler/run_tests.sh` — full suite (currently 1405 assertions) passes after each phase
2. Per-agent: `make build/test_<suite>` for fast incremental validation
3. TCO: 10M-iteration self-recursive program must not stack overflow
4. Overloading: two same-name functions with different types dispatch correctly
5. Swap: array element swap confirms values exchanged

---

## Decisions
- **stdlib** shelved — not in alpha5
- **Mutual TCO** deferred post-alpha5 (needs trampoline design)
- **Wildcard imports** scoped to in-source + dep CAR packages only (io.stdlib.* stays qualified)
- **`typeof()`** returns base type name (`"int32"`, `"arr"`, union name, etc.)
- **`><`** is void — statement-level side effect, any assignable lvalue
- **Overload resolution**: widening allowed, error on ambiguity
- **Optional param defaults**: any expression (including calls)
