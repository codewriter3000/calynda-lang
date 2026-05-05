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