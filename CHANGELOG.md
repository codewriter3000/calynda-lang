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

# Calynda 1.0.0-alpha.5

April 17, 2026

## Highlights

- **`start` is now argument-optional and return-optional.** The entry point no longer requires `(string[] args)` parameters or an `int32` return. All four forms are now valid: `start -> expr;`, `start -> { ... };`, `start(string[] args) -> expr;`, and `start(string[] args) -> { ... };`. A void `start` body that falls through or uses bare `return;` exits with code 0.
- **`boot` no longer takes arguments or parentheses.** The bare-metal entry point is now written as `boot -> { ... };` instead of `boot() -> { ... };`, reflecting that it bypasses the runtime entirely and therefore cannot receive any parameters.
- **Operator overloading.** Top-level bindings may now share the same name with different parameter types. The type checker selects the correct overload at each call site using exact-type matching first, then numeric widening. Ambiguous call sites (where two overloads match equally well) are diagnosed as errors.
- **Default parameter values.** Parameters may now carry a default expression: `Type name = expr`. Callers may omit trailing defaulted arguments; the HIR inlining pass substitutes the default expression at each omitted call site. A default expression may only reference parameters declared earlier in the same list.
- **Swap operator (`><`).** `x >< y;` atomically exchanges the values of two writable expressions. The HIR expansion pass lowers the swap into a standard read/modify/write triple so no new runtime helpers are needed.
- **Self tail-recursion elimination.** The MIR pass now detects when the sole recursive call in a lambda is in tail position and rewrites the unit into an explicit loop, preventing stack growth for deeply recursive programs.
- **Type-query builtins.** Four new intrinsics are available anywhere in Calynda code: `typeof(value)` returns a `string` naming the runtime type; `isint(value)`, `isfloat(value)`, `isbool(value)`, `isstring(value)`, and `isarray(value)` return `bool`; and `issametype(x, y)` returns `bool`. These are dispatched through the runtime rather than resolved as compile-time constants.
- **Wildcard imports fixed.** `import pkg.*;` now correctly injects all exported symbols from `pkg` into the current scope. Previously wildcard imports parsed but symbols were not merged, causing unresolved-identifier errors on direct calls like `print("…")`.
- **External `.car` dependency archives.** `calynda build`, `run`, and `asm` now accept `--archive path.car` (repeatable) and `--archive-path dir` (loads all `.car` files from a directory) to resolve imports against prepackaged library archives without repacking the application source.
- **Version string corrected.** `calynda --version` now reports the correct `1.0.0-alpha.5` string. Previously all releases reported `1.0.0-alpha.2`.

---

# Calynda 1.0.0-alpha.4

April 17, 2026

## Highlights

- Top-level lambda-style bindings can now call themselves recursively. Previously, any reference to a binding name from within its own lambda initializer was rejected with "Circular definition involving 'X'". The type checker now seeds the callable signature before checking the lambda body, so recursive self-calls observe the correct type without triggering the circular-definition guard. Non-lambda initializers still reject genuine cycles.

---

# Calynda 1.0.0-alpha.3

April 16, 2026

## Highlights

- `calynda asm` now accepts `.car` archives in addition to `.cal` source files. `calynda build` and `calynda run` already supported archives; `asm` now matches that behavior so multi-file projects can inspect generated assembly without an intermediate step.
- `manual(args) -> { ... }` is a new whole-function manual shorthand. It is syntactic sugar for a lambda whose entire body is wrapped in a `manual { ... }` block. The existing `manual { ... };` statement form is unchanged.
- `car(arr)` and `cdr(arr)` are new built-in functions for arrays. `car` returns the first element of a non-empty array; `cdr` returns a new array containing every element except the first. Both abort at runtime on an empty array.
- String indexing is now supported. `str[i]` returns the `char` byte at position `i`. Out-of-bounds access aborts at runtime. The index expression must be integral.
- Returns inside `manual { ... }` sub-blocks now correctly propagate into the enclosing lambda or `start` return analysis. Previously, a `return` nested inside a manual block was not counted toward the whole-function return obligation, causing spurious type errors.

---

# Calynda 1.0.0-alpha.2

April 16, 2026

## Highlights

- The CLI now exposes version metadata via `calynda --version`.
- The strict race-checker flag is frozen as `--strict-race-check`.
- `spawn` now selects its result type by callable return: zero-argument `void` callables return `Thread`, and zero-argument non-`void` callables return `Future<T>`.
- The shipped alpha.2 member surface is `Thread.join()` / `Thread.cancel()`, `Future<T>.get()` / `Future<T>.cancel()`, and `Atomic<T>.new(value)` / `.load()` / `.store(value)` / `.exchange(value)`.
- `thread_local` storage exists for cross-thread identity, but it is not a general stack-local mechanism.
- `Atomic<T>` is currently limited to first-class single-word runtime values.
- `boot()` remains a freestanding entry contract; the RISC-V `_start` path no longer advertises Linux-only exit behavior.

## Concurrency Notes

- Cancellation uses pthread cancellation semantics.
- Forced cancellation does **not** yet guarantee Calynda-level cleanup execution for `manual { ... };` or `manual checked { ... };` scopes.
- Alpha.2 documentation, bytecode notes, roadmap closeout text, and MCP/help surfaces are now aligned to the landed concurrency contract.

---

# Calynda 0.4.0

April 16, 2026

## Highlights

- Shared runtime descriptor metadata is now frozen across heterogeneous arrays, tagged unions, bytecode constants, and native lowering.
- `arr<?>` now has explicit 0.4.0 semantics: shape-locked literal construction, indexed reads as `<external>`, indexed writes rejected, and identity-based array equality.
- Tagged unions now expose read-only `.tag` and `.payload` access as first-class language operations with dedicated MIR, LIR, bytecode, and native helper lowering.
- `manual { ... };`, `manual checked { ... };`, and `ptr<T>` are now the stable unsafe manual-memory surface for 0.4.0.
- `stackalloc(size)` is defined as manual-scope scratch storage reclaimed at scope exit, and `manual checked` now uses a growable bounds registry instead of the old provisional fixed-cap table.
- Layout declarations remain intentionally limited to scalar primitive fields in 0.4.0.

## Tooling And Docs

- The README, MCP server metadata, and VS Code syntax extension are synced to the 0.4.0 language surface.
- The CLI help and installation docs now describe the shipped runtime archive and the `pack` workflow for CAR archives.

---

# Calynda 0.3.0

April 15, 2026

## New Features

### ARM64 support

Linux AArch64 is now a supported compilation target. Pass `--target aarch64-linux`
to `calynda build` to produce AArch64 assembly. The default target remains Linux
x86-64 SysV.

### Inline assembly declarations (`asm()`)

A new top-level declaration form lets you write platform-specific routines directly
in a Calynda source file:

```
int32 my_add = asm(int32 a, int32 b) -> {
    mov eax, edi
    add eax, esi
    ret
};
```

The assembly body is passed through to the assembler unchanged. Parameters follow
normal Calynda type syntax and map to ABI registers. The `export` and `static`
modifiers are supported.

### Bare-metal entry point (`boot()`)

`boot()` is an alternative entry point that bypasses the Calynda runtime entirely
and emits a raw `_start` symbol. It is intended for freestanding and embedded
targets where no OS runtime is available.

```
boot() -> 0;
```

`boot()` and `start()` cannot coexist in the same compilation unit.

### CAR source archives

The CAR (Calynda Archive) format bundles multiple `.cal` source files into a single
portable archive. Two new CLI subcommands are available:

- `calynda car create archive.car src/` — creates an archive from a directory.
- `calynda car extract archive.car` — extracts files from an archive.

### RISC-V 64 support

Linux RISC-V 64 (RV64GC) is now a supported compilation target. Pass
`--target riscv64-linux` to `calynda build` to produce RISC-V assembly.

### Manual memory boundary (`manual { ... };`) *(experimental in 0.3.x; stable in 0.4.0)*

`manual { ... };` opens an unsafe scope for direct memory management.
`manual checked { ... };` routes those operations through bounds-checked runtime
helpers. The following built-in expressions are available inside a manual block:

| Expression | Returns | Description |
|---|---|---|
| `malloc(size)` | `int64` | Allocates `size` bytes. |
| `calloc(count, size)` | `int64` | Allocates zeroed memory. |
| `realloc(addr, size)` | `int64` | Resizes an allocation. |
| `free(addr)` | `void` | Releases an allocation. |
| `stackalloc(size)` | `int64` | Allocates manual scratch space and auto-registers a deferred free. |
| `deref(addr)` | element type | Reads the value at `addr`. |
| `store(addr, value)` | `void` | Writes `value` to `addr`. |
| `offset(addr, count)` | `int64` | Advances `addr` by `count` elements. |
| `addr(expr)` | `int64` | Takes the address of a value. |
| `cleanup(value, fn)` | same as `value` | Registers `fn(value)` for manual-scope exit. |

`cleanup()` callbacks also run before a `return` or `throw` leaves an enclosing
manual block.

This surface shipped experimentally in 0.3.x. For 0.4.0, the contract is now
frozen: `manual { ... };`, `manual checked { ... };`, and `ptr<T>` are stable
unsafe features, `stackalloc(size)` provides scope-local scratch storage
reclaimed on manual-scope exit rather than guaranteed machine-stack residency,
and `manual checked` uses growable runtime bounds tracking instead of a fixed
live-pointer ceiling.

### Typed pointers (`ptr<T>`) *(experimental in 0.3.x; stable in 0.4.0)*

Variables inside a `manual` block can be declared with `ptr<T>` to get typed
pointer semantics. When a `ptr<T>` variable is used with `deref`, `store`, or
`offset`, the compiler automatically sizes memory operations to `sizeof(T)`.

```cal
manual {
    ptr<int32> p = malloc(40);
    store(p, 7);
    int32 v = deref(p);
    ptr<int32> q = offset(p, 2);
    free(p);
};
```

All arguments must be integral types. The return value of `malloc`, `calloc`, and
`realloc` is a raw `int64` address with no type safety — use with care.

For 0.4.0, this pointer surface is considered stable, but it remains explicitly
unsafe: the raw address representation is unchanged, and the compiler only adds
typed sizing for `deref`, `store`, and `offset`.

## Small Changes

- Codebase refactor

---

# Calynda 0.2.1

April 4, 2026

Calynda 0.2.1 is a refactoring and cleanup release. No new language features are
introduced.

## Highlights

- The obsolete V1 grammar has been removed. The canonical grammar now lives at
  `compiler/calynda.ebnf` and reflects the full V2 language surface.
- `manual()` and `asm()` references have been removed from the grammar. Both are
  deferred to V3 and will not ship until their full design cycle is complete.
- Every compiler source file in `compiler/src/` has been refactored to stay under
  250 lines by splitting large modules into focused compilation units organized
  by logical concern (e.g., `parser_expr.c`, `parser_decl.c`).
- The Makefile has been updated to build all new split files.

## Grammar Cleanup

- Deleted the obsolete V1 grammar (`compiler/calynda.ebnf`).
- Removed `ManualStatement`, `ManualBody`, `MallocStatement`, `CallocStatement`,
  `ReallocStatement`, `FreeStatement`, and pointer operators `*`/`&` from the
  grammar — all deferred to V3.
- Renamed `compiler/calynda_v2.ebnf` to `compiler/calynda.ebnf` as the sole
  canonical grammar.

## Refactoring

- Split 26 source files that exceeded 250 lines into ~65 focused compilation
  units. Each split follows the module's existing header and public API — no
  external interface changes.
- All 1147 tests continue to pass.

---

# Calynda 0.2.0

April 4, 2026

Calynda 0.2.0 lands the V2 language surface on top of the 0.1.0 compiled
toolchain.

## Highlights

- Reified generics with Java-style `<T>`, `<K, V>`, and wildcard `<?>` syntax.
- Heterogeneous arrays via `arr<?>`, replacing the old struct concept.
- Tagged unions with `union Name<T> { Variant(T), Empty }` declarations.
- Expanded import model: module aliases (`as`), wildcard imports (`.*`),
  selective imports (`{a, b, c}`), and explicit `export` control.
- Primitive aliases: `byte`, `sbyte`, `short`, `int`, `long`, `ulong`, `uint`,
  `float`, `double` alongside Calynda-native type names.
- Prefix and postfix `++`/`--` operators.
- Varargs parameters (`Type... name`).
- Discard expression (`_`).
- `internal` modifier for nested helper visibility.
- `static` and `export` modifiers for top-level declarations.

## Deferred To V3

- `manual()` — manual memory boundary with malloc/calloc/realloc/free.
- `asm()` — inline assembly. Depends on `manual()` landing first.
- One-statement callback shorthand — pending explicit auto-lift rule decision.

---

# Calynda 0.1.0

April 2, 2026

This is the first public edition of Calynda.

Calynda 0.1.0 ships the first end-to-end compiled toolchain for the language: parsing, semantic analysis, lowering through multiple IR stages, native code generation for Linux x86_64, a portable bytecode emitter, a runtime layer for dynamic language features, and a command-line compiler that can build and run `.cal` programs.

## Highlights

- Native compilation now works end to end on Linux x86_64 SysV ELF. `calynda build program.cal` emits native assembly, links it with the runtime, and produces a runnable executable.
- The CLI also supports `calynda run`, `calynda asm`, and `calynda bytecode`, so the first edition is usable both as a compiler and as an inspection tool.
- A portable backend now exists alongside the native backend. Calynda can emit `portable-v1` bytecode from MIR as a compiled backend target.
- The repository now includes `install.sh`, which builds Calynda and installs the CLI together with the required `runtime.o` payload for new users.

## Language And Compiler Surface

This first edition includes the front-end and semantic pipeline needed to compile non-trivial Calynda programs.

- Recursive-descent parsing from the Calynda grammar in [calynda.ebnf](compiler/calynda.ebnf).
- Source-aware diagnostics for parse, symbol, and type errors.
- A required single `start(string[] args)` program entry point.
- Lambda-based callable model with first-class lambdas and closure lowering.
- Arrays, casts, member access, index access, function calls, assignments, ternaries, template literals, and `throw`.
- Multiple lowering stages: AST, HIR, MIR, LIR, codegen planning, machine emission, native assembly, and portable bytecode.

## Runtime And Backend Model

Calynda 0.1.0 is a compiled language release, not an interpreter release.

- Primary backend: native x86_64 SysV ELF.
- Secondary backend: `portable-v1` bytecode emission.
- Rejected path: interpreting AST, HIR, MIR, or later IR directly.

The runtime currently provides the helper surface needed for closures, callable dispatch, array literals and indexing, template construction, member access, dynamic casts, indexed and member stores, and thrown values. Native startup also boxes CLI arguments into Calynda `string[]` values before entering `start`.

## Tooling

- CLI binary: `build/calynda`.
- Install script: [install.sh](/home/codewriter3000/Coding/calynda-lang/install.sh).
- VS Code syntax support is included in [vscode-calynda/README.md](/home/codewriter3000/Coding/calynda-lang/vscode-calynda/README.md) and packaged locally as `calynda-syntax-0.1.0.vsix`.

## Installation

For a local user install:

```sh
sh ./install.sh
```

For a custom prefix:

```sh
sh ./install.sh --prefix /usr/local
```

The installer places the launcher in `bin/` and the real CLI plus `runtime.o` under `lib/calynda/` so the compiler can link generated executables correctly.

## Known Limits In 0.1.0

- Native compilation currently targets Linux x86_64 SysV ELF and uses `gcc` for final linking.
- The bytecode backend currently emits the `portable-v1` program form; this release does not yet ship a bytecode executor.
- Runtime-backed language features are present, but the runtime surface is intentionally narrow and focused on the helpers the compiler already lowers to.
- The bundled editor tooling is syntax highlighting and language configuration, not a full language server.

## First Edition Summary

Calynda 0.1.0 establishes the project's core direction: a compiled functional systems language with a real native backend, a defined runtime boundary, and a second compiled backend path for portability. This release is the foundation release for future work on backend depth, runtime capabilities, and broader tooling.
