# Calynda 1.0.0-alpha.8

May 24, 2026

## Highlights

- **Compiler warnings, advisories, and size-focused builds.** The type checker now tracks non-fatal warnings and opt-in performance advisories separately from hard errors, and the CLI surfaces them during `calynda asm`, `build`, `run`, and archive compilation with source spans. New flags `--no-performance-warnings`, `--performance-advisories`, and `--size-focus` let you suppress dynamic-dispatch warnings, opt into minor performance advisories, or bias lowering toward smaller, more predictable helper-backed paths.
- **Dynamic callable dispatch and template diagnostics.** Calls made through `var` or other `external`-typed callable values now emit a default performance warning because they route through runtime helper dispatch instead of direct calls. Template literals now emit opt-in advisories, with stronger wording in `boot`, inside `manual` blocks, or under `--size-focus`, where interpolation stays on the runtime-helper path.
- **Omitted array extents preserve static shapes farther into the pipeline.** Declarations such as `int32[] values = [1, 2, 3]`, declared lambda returns, and parameter default values now absorb statically provable extents instead of immediately erasing them. When an omitted extent still depends on runtime flow, the compiler remains permissive but can emit an advisory that the length stays runtime-derived.
- **Typed bindings can now omit initializers.** The parser now accepts `Type name;` for top-level and local bindings. Semantic acceptance is intentionally narrower: explicitly typed, non-`final` bindings get default initialization, while `var` and `final` bindings still require an initializer. HIR then synthesizes concrete defaults such as `0`, `false`, or `null`, so later IR and codegen do not need to carry uninitialized slots.
- **MIR and native assembly now prune unreachable initialization more aggressively.** MIR reachability analysis drops unreachable top-level callable units and pure dead `__mir$module_init` segments, including branching and short-circuit-only initialization that no reachable code reads. Native assembly emission now follows reachable roots as well, skipping dead machine units, dead static-array roots, and their startup-registration glue.
- **Hosted template lowering has a clearer fast path.** Ordinary hosted single-expression templates such as `` `${value}` `` keep the cheap cast-based path when possible, while strong contexts (`boot`, `manual`, or `--size-focus`) intentionally route through the generic template builder. This makes the cost model more explicit and gives size-sensitive builds a predictable lowering choice.
- **Diagnostics knowledge is now shared with tooling.** The compiler tree now carries a warning/advisory catalog, and the MCP server exposes the same families through `calynda://diagnostics`, the `explain_calynda_diagnostic` tool, and structured diagnostic details in analyzer and validator results.
- **Regression and benchmark coverage expanded.** New parser, type-checker, HIR, MIR, asm-emission, CLI, and MCP regressions cover the new diagnostics and pruning behavior, and a recursive Fibonacci benchmark now ships under `benchmarks/recursive-fib/` with Calynda, C, and Rust baselines.

## Backward compatibility

- `var name;` and `final T name;` remain invalid; only explicitly typed, non-`final` bindings can omit the initializer.
- Dynamic callable dispatch through `var` or `external` now warns by default but does not fail the build. Pass `--no-performance-warnings` to silence it.
- `--performance-advisories` is opt-in. Enabling it, especially with `--size-focus`, may surface new notes for template literals and runtime-derived omitted array extents.
- Unused pure top-level initializers may now disappear from MIR and assembly output if no reachable code reads them.

---

# Calynda 1.0.0-alpha.7

May 16, 2026

## Highlights

- **Typed MMIO and cache-control builtins.** `mmio<T>` now joins `ptr<T>` as a first-class low-level memory surface. `mmio<T>.value` type-checks as `T`, stays assignable through `offset(mmio, n).value`, supports compound assignment and postfix operators, and lowers through dedicated volatile helpers instead of the plain pointer path. The embedded/runtime surface also grows `fence()`, `cacheclean(address)`, and `cachefinal()` for ordering and cache maintenance.
- **Statement-level inline assembly.** Block bodies can now contain `asm { ... };` statements in addition to top-level `asm(...) -> { ... };` declarations. HIR lowers each statement by synthesizing a hidden zero-argument asm unit and then emitting an ordinary call, so the surface works in `boot` and hosted code without a separate execution model.
- **Sized-array semantic enforcement.** Declared extents in `T[2]`, `T[2][3]`, and similar forms are now preserved long enough for semantic analysis to reject mismatched initializer sizes, assignments, parameter defaults, lambda returns, call arguments/results, and ternary merges when shape metadata survives.
- **Static final global arrays lower as static objects.** Immutable module-scope arrays no longer have to be rebuilt at runtime during module initialization. Native lowering now materializes them as static objects directly, reducing startup work and making generated output more deterministic.
- **CLI/runtime GC controls.** `calynda build`, `calynda run`, and `calynda asm` now accept `--manual-bounds-check`, `--gc marksweep|legacy`, and `--gc-plugin path.a`. Hosted installs now ship both `calynda_runtime.a` and the default mark-and-sweep archive `calynda_runtime_ms.a`, while `boot` continues to link against `calynda_runtime_boot.a`.
- **Tooling and repository sync.** The compiler tree was split further into focused subdirectories to preserve the 250-line/15-entry rules, and the MCP server, release docs, and Calynda/Calynda QA agents are refreshed to cover the alpha.7 embedded, GC, and array-shape surface.

## Backward compatibility

- `mmio<T>` is additive; existing `ptr<T>` manual-memory programs keep their previous semantics.
- Sized-array checking is stricter where fixed extents were previously ignored, so programs with mismatched declared shapes may now fail during semantic analysis.
- The default hosted GC is now mark-and-sweep. Pass `--gc legacy` to preserve the previous append-only registry behavior.

---

# Calynda 1.0.0-alpha.6

May 4th, 2026

## Highlights

- **Untyped `var` parameters.** Functions and lambdas may now declare a parameter as `var name` (no type) — the parameter accepts a value of any type, opaque at compile time, and is inspected at run time using the existing type-query intrinsics (`typeof`, `isint`, `isstring`, `isarray`, …). `var` parameters cannot be varargs and cannot precede a typed parameter. The grammar (`compiler/calynda.ebnf`) gains a new `Parameter` alternative for the untyped form.
- **`|var` early-return parameters.** A lambda parameter may be prefixed with `|` to mark it as an early-return value: writing through such a parameter performs a non-local return out of the enclosing call. A new runtime helper layer (`runtime_nlr.c`, `__calynda_rt_nlr_push`/`_invoke`/`_check_pop`/`_get_value`) implements the unwinding using a thread-local slot stack.
- **`num` built-in generic numeric type.** `num` is a compile-time placeholder that resolves to whichever numeric primitive (`int8` … `int64`, `uint8` … `uint64`, `float32`, `float64`) the call site requires. A single overload written against `num` participates in numeric widening at each call site.
- **`arr<?>` wildcard array type.** `arr<?>` may be used as a parameter or storage type to accept any primitive-element array (`int32[]`, `string[]`, `float64[]`, …). The element type is opaque inside the body and accessed through indexing plus the runtime type queries.
- **Capture-by-reference lambdas.** Closures now observe writes to enclosing locals: a lambda that captures `x` and is called after `x = …` sees the updated value. Capture analysis (`mir_capture_analysis*`) was extended; previously every captured variable was a value-copy.
- **`car(s)` and `cdr(s)` accept `string`.** `car` returns the first byte as a `char`; `cdr` returns a new `string` with the first byte removed. Both abort at runtime on an empty string. Existing array overloads are unchanged.
- **User-input intrinsics.** New runtime functions wired into the compiler for reading from standard input (line-oriented and typed read variants). They are dispatched through the existing runtime ABI table; see `runtime/runtime.c` and `runtime/runtime_format.c` for details.
- **First release with a standard library.** A small `lib/` of `.cal` modules ships alongside the toolchain (`conditional.cal`, `loop.cal`, `math.cal`, `string_utils.cal`, plus a `structure/` package). Modules are imported through the existing `import` machinery and packaged via `.car`.
- **Expanded bare-metal support.** The runtime is now split into a hosted archive (`calynda_runtime.a`) and a freestanding boot archive (`calynda_runtime_boot.a`). The boot archive is compiled with `-ffreestanding -fno-builtin -fno-stack-protector` and is what `boot -> { ... };` programs link against. Cross-compiled aarch64 and riscv64 boot archives (`calynda_runtime_boot_aarch64.a`, `calynda_runtime_boot_riscv64.a`) are produced by `make runtime-aarch64` / `make runtime-riscv64`. `install.sh` now installs both archives.
- **Improved type-checker / lambda subsystem.** `type_checker_lambda.c`, `type_checker_resolve_binding.c`, `type_checker_types.c`, and `type_checker_ops.c` were extended to integrate `var`, `num`, `arr<?>`, `|var`, and reference captures with overload resolution and operator dispatch.
- **MIR / HIR lowering updates.** `hir_lower_expr*`, `mir_lower*`, `mir_expr_call.c`, `mir_lambda.c`, `mir_lvalue.c`, `mir_store.c`, `mir_capture*`, and `mir_tco.c` were touched to lower the new parameter forms, capture-by-reference, and non-local returns.
- **CLI/build updates.** `cli/build_native*`, `cli/calynda_compile.c`, `cli/calynda_commands.c`, `cli/calynda_car.c`, `cli/calynda_utils.c` updated for the boot-runtime split and improved CAR archive handling.
- **Repository policy.** Top-level entries per directory are kept ≤ 15 and every C source/header file is now ≤ 250 lines. `compiler/src/parser/` was split (`util/` subdirectory holds `parser_lookahead.c`, `parser_utils.c`) and `compiler/tests/ir/` was split into `bytecode/`, `hir/`, `lir/`, `mir/` subdirectories. Files that exceeded the 250-line target were broken up via the `scripts/split_long_file.sh` helper, which cuts a long file at safe boundaries (blank lines, closing braces, struct-entry endings) and chains the parts together with `#include` fragments (`*_p2.inc`, `*_p3.inc`, … for `.c` files; `*_hp2.inc`, `*_hp3.inc`, … for headers, to avoid name collisions). The original symbol of each split file remains a single translation unit; only the textual layout changed.
- **MCP server + agent.** `mcp-server/` knowledge files (`grammar-structure`, `grammar-expressions`, `examples-v3`), parser modules (`lexer-defs`, `parser-blocks`, `parser-declarations`, `parser-statements`), and prompt/tool surfaces were updated to expose `var`, `num`, `arr<?>`, `|var`, capture-by-reference, and the new `car`/`cdr` overloads. The `calynda` agent definition was refreshed accordingly.

## Removed

- `.github/prompts/plan-alpha5AgentFleet.prompt.md` (alpha.5 planning artifact, no longer relevant).

## Backward compatibility

- All grammar additions are opt-in; existing `.cal` programs continue to parse and type-check.
- Programs that do not use `boot` link only against `calynda_runtime.a` as before; the boot archive is consumed only by `boot -> { ... };` programs.