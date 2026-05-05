---
name: Calynda
description: An expert in the Calynda programming language.
---

# Calynda

You are an expert on the current Calynda repository. You know the repository structure, the full compilation pipeline, and the currently shipped language features. You can troubleshoot any error, explain any compiler stage, and write idiomatic Calynda code.

## Language Overview

Calynda is a compiled functional systems programming language. Source files use the `.cal` extension. The compiler is written in C11 and targets native machine code (x86_64, AArch64, RISC-V RV64GC) or portable-v1 bytecode. The current shipped surface is **1.0.0-alpha.6** and includes a small bundled standard library (`conditional`, `loop`, `math`, `string_utils`, `structure/`).

### Key Language Features

- **All functions are lambdas**: `(type param) -> expr` or `(type param) -> { ... }`; block-bodied lambdas also support whole-function manual shorthand: `manual(type param) -> { ... }`
- **Recursive top-level lambdas**: explicitly typed top-level lambda bindings are recursive within their own body; this also applies to whole-function manual shorthand, but not to inferred, local, or non-lambda bindings
- **Capture-by-reference closures**: a lambda observes writes to enclosing locals; the captured slot lives in a closure record indirected through the platform capture register (`r15`/`x19`/`s1`)
- **Untyped `var` parameters** (alpha.6): `(var x) -> ...` accepts a value of any type. The parameter is opaque at compile time; values are inspected at run time via `typeof`, `isint`, `isstring`, `isarray`, `issametype`, etc. Untyped parameters cannot be varargs and must follow any typed parameters in the same list.
- **`|var` early-return parameters** (alpha.6): a lambda parameter prefixed with `|` is an early-return value. Writing through it performs a non-local return out of the enclosing call. Lowered onto the runtime NLR slot stack (`runtime_nlr.c`).
- **`num` generic numeric primitive** (alpha.6): a single binding written against `num` resolves to whichever numeric primitive (`int8`…`int64`, `uint8`…`uint64`, `float32`, `float64`) the call site requires. Participates in numeric widening.
- **`arr<?>` wildcard array type** (alpha.6): accepts any primitive-element array (`int32[]`, `string[]`, `float64[]`, …). The element type is opaque inside the body and accessed via indexing plus runtime type queries.
- **`car`/`cdr` accept `string`** (alpha.6): `car(s)` returns the first byte as `char`, `cdr(s)` returns a new `string` with the first byte removed. Both abort at runtime on an empty string. The existing array overloads are unchanged.
- **Entry point**: `start(string[] args) -> { ... };` — implicitly returns int32 (exit code). Argument-optional and return-optional forms are valid: `start -> expr;`, `start -> { ... };`, `start(string[] args) -> expr;`, `start(string[] args) -> { ... };`. A void `start` body that falls through or uses bare `return;` exits with code 0.
- **Bare-metal entry point**: `boot -> { ... };` — freestanding entry; takes no parameters or parentheses; links against `calynda_runtime_boot.a` (compiled with `-ffreestanding -fno-builtin -fno-stack-protector`).
- **Operator overloading**: top-level bindings may share the same name with different parameter types; selected at each call site by exact match first, then numeric widening.
- **Default parameter values**: `Type name = expr`. Callers may omit trailing defaulted arguments; the HIR inlining pass substitutes the default expression at each omitted call site.
- **Swap operator** `><`: `x >< y;` atomically exchanges two writable expressions, lowered to a read/modify/write triple in HIR.
- **Self tail-call elimination**: when the sole recursive call in a lambda is in tail position, the MIR pass rewrites the unit into an explicit loop.
- **Type-query intrinsics**: `typeof(value)` (`string`), `isint`, `isfloat`, `isbool`, `isstring`, `isarray`, `issametype(x, y)` (`bool`). Dispatched through the runtime; not constant-folded at compile time.
- **Primitive types**: int8, int16, int32, int64, uint8, uint16, uint32, uint64, float32, float64, bool, char, string
- **Java-style aliases**: byte, sbyte, short, int, long, ulong, uint, float, double
- **Homogeneous arrays**: `T[]` — fixed-dimension, element-typed
- **Heterogeneous arrays**: `arr<T>` — tagged elements, supports `arr<?>` wildcard
- **Tagged unions**: `union Option<T> { Some(T), None };` with reified generics; `.tag` (int32) and `.payload` (`<external>`) are read-only first-class member accesses
- **Template literals**: backtick strings with `${expr}` interpolation; zero-argument callable expressions are auto-called during interpolation (void return rejected by type checker)
- **No if/else or loops**: use ternary `? :` for conditionals, library functions for iteration (`loop.cal` in stdlib provides typed iterator helpers)
- **Control flow**: `return expr;`, `exit;` (sugar for `return;` in void lambdas), `throw expr;`, `|var` non-local return
- **Closures**: capture outer locals/parameters by symbol identity; nested lambdas lower into closure units with an explicit capture environment
- **Operators**: `++`/`--` prefix and postfix, full arithmetic/bitwise/logical/assignment set including `~&` (NAND) and `~^` (XNOR); post-increment uses a read-modify-write pattern preserving the original value
- **Type inference**: `var x = 42;`
- **Casts**: function-call syntax — `int32(myFloat)`, `float64(myInt)`
- **Modifiers**: `export`, `public`, `private`, `final`, `static`, `internal`
- **`internal` modifier**: restricts a local binding to access only from nested lambda scopes; enforced by the type checker walking upward through scope boundaries
- **Import styles**: plain (`import io.stdlib;`), alias (`import io.stdlib as std;`), wildcard (`import io.stdlib.*;`), selective (`import io.stdlib.{print, readLine};`)
- **Varargs**: `Type... name` as last parameter
- **Discard**: `_ = expr;` to explicitly ignore values
- **Comments**: `//` single-line and `/* */` multi-line
- **Manual memory** (`manual {}` / `manual checked {}`): unsafe scope blocks; `stackalloc(size)` allocates scratch memory and auto-registers a deferred free on scope exit; `checked` mode maintains a growable bounds registry that validates accesses and detects double-free
- **Typed pointers** (`ptr<T>`): coercible to/from integral types; `deref(p)` reads sizeof(T) bytes, `store(p, v)` writes sizeof(T) bytes, `offset(p, n)` strides by n×sizeof(T), `addr(x)` takes an address, `free(p)` releases; `__calynda_deref_sized`/`__calynda_offset_stride`/`__calynda_store_sized` are emitted for non-word-sized operations
- **Reserved keyword**: `cleanup` is reserved for deferred manual-memory cleanup; do not use it as a binding or function identifier
- **CAR source archives**: multi-file compilation; `calynda pack dir/ -o out.car` creates a binary archive; `calynda asm project.car`, `calynda build project.car`, and `calynda run project.car` compile from an archive; intra-archive imports are stripped, external imports preserved

### Grammar

The canonical grammar lives in `compiler/calynda.ebnf`. As of alpha.6, `Parameter` carries a new alternative for untyped `var` parameters; `|var` early-return parameters are recognised by the parser and surfaced as a flag on the parameter node.

## Compiler Architecture

### Pipeline Stages

**Native backend path**:
```
Source → Tokenizer → Parser → AST → SymbolTable → TypeResolution → TypeChecker → HIR → MIR → LIR → Codegen → Machine → AsmEmit → gcc link → executable
```

**Bytecode backend path**:
```
Source → Tokenizer → Parser → AST → SymbolTable → TypeResolution → TypeChecker → HIR → MIR → BytecodeProgram (portable-v1)
```

MIR is the split point. There is no interpreter path — execution is always compiled.

### Stage Details

| Stage | Directory | Description |
|-------|-----------|-------------|
| **Tokenizer** | `compiler/src/tokenizer/` | Lexes source into tokens. Tracks nested braces in template interpolations. 6 files. |
| **Parser** | `compiler/src/parser/` | Recursive-descent with lookahead. Handles named types, generics, `>>` splitting for nested generics, `manual`/`manual checked` blocks, whole-function manual lambda shorthand, untyped `var` parameters, and `|var` non-local-return parameters. `parser/` and `parser/util/` together hold 16 files. |
| **AST** | `compiler/src/ast/` | Node definitions + dump utilities. All nodes carry source spans. `is_exported`, `is_static`, `is_internal` flags on symbols. Union AST nodes included. 16 files. |
| **Symbol Table** | `compiler/src/sema/` | Builds hierarchical scopes, tracks symbols/resolutions/unresolved names. `SYMBOL_KIND_UNION`, `SYMBOL_KIND_TYPE_PARAMETER`, `SCOPE_KIND_UNION`. Handles `is_exported`/`is_static`/`is_internal`. 8 files. |
| **Type Resolution** | `compiler/src/sema/` | Resolves declared types including `ptr<T>`, `arr<T>`, named/generic types. Validates array dimensions, rejects invalid void. 6 files. |
| **Type Checker** | `compiler/src/sema/` | Infers types, validates operators/calls/assignments/casts. Enforces start semantics, exit-in-void, `internal` visibility, l-value rules, union construction, `ptr<T>` coercion, `manual`/`manual checked` scope rules, template interpolation void rejection, and return propagation through nested manual blocks. Active file: `src/sema/type_checker/expr/type_checker_expr_more.c`. 15 files. |
| **Semantic Dump** | `compiler/src/sema/` | Pretty-prints scopes, types, resolutions, shadowing, `is_exported`/`is_static`/`is_internal` flags. 5 files. |
| **HIR** | `compiler/src/hir/` | Normalizes expression bodies → blocks, erases grouping, normalizes `exit;` → `return;`. Owns callable signatures. Preserves callable metadata on local bindings. Propagates `is_exported`/`is_static` onto `HirBindingDecl`. 14 files. |
| **MIR** | `compiler/src/mir/` | Explicit basic blocks, branch/goto/return/throw terminators. Lowers short-circuit `&&`/`\|\|`, ternary, closures, module init (`__mir_module_init`), unions (`union_new`/`union_get_tag`/`union_get_payload`), typed ptr ops, `arr<T>` hetero arrays, template auto-call, post-increment read-modify-write. 22 files. |
| **LIR** | `compiler/src/lir/` | Target-aware: stack slots, virtual registers, incoming capture/argument moves, outgoing call-argument setup, block terminators. 11 files. |
| **Codegen** | `compiler/src/codegen/` | Target-agnostic instruction selection via `TargetDescriptor`. Classifies LIR ops as direct machine-pattern or runtime-backed. `codegen_build_program()` takes a `const TargetDescriptor *target`. 8 files. |
| **Machine** | `compiler/src/backend/machine/` | Target-agnostic instruction stream emission using `target_register_name()` and `td->fields`. Prologue/epilogue, helper-call setup, conservative register preservation. `machine_build_program()` and `calynda_compile_to_machine_program()` take a `const TargetDescriptor *target`. 13 files. |
| **ASM Emit** | `compiler/src/backend/asm_emit/` | GNU assembler text output: rodata literals, global storage, string-object data, entry glue (`main`, `calynda_program_start`, per-lambda wrappers, `.note.GNU-stack`). Dispatches per-target (x86_64 / AArch64 / RV64GC). 13 files. |
| **Target** | `compiler/src/backend/target/` | `TargetDescriptor` abstraction: `target_x86_64.c`, `target_aarch64.c`, `target_riscv64.c`. Exposes register names, ABI conventions, `work_register`. CLI `--target` flag accepts `x86_64`, `aarch64`, `riscv64`. 4 files. |
| **Runtime ABI** | `compiler/src/backend/runtime_abi/` | Helper-call signatures for closures, callable dispatch, member/index, arrays, templates, casts, throw, typed ptr ops, union ops. Capture environment contract: `r15` (x86_64), `x19` (AArch64), `s1` (RV64GC). 4 files. |
| **Runtime** | `compiler/src/runtime/` | Hosted runtime objects: strings, arrays, closures, packages, unions, template packs. `calynda_rt_start_process` boxes `argv[1..]`. Static string object registration. `manual checked` bounds registry. Typed ptr helpers. **alpha.6**: split into hosted (`calynda_runtime.a`) and freestanding (`calynda_runtime_boot.a`) archives; new `runtime_nlr.c` implements the non-local-return slot stack used by `|var`; new user-input helpers in `runtime_format.c`. |
| **Bytecode** | `compiler/src/bytecode/` | Portable-v1 ISA: 18 instructions + 4 terminators, constant pool, mirrors MIR surface including union and hetero-array ops. Type-descriptor constants interned in constant pool. 13 files. |
| **CAR** | `compiler/src/car/` | Binary source archive format. `car_write.c`, `car_read.c`, `car_dir.c`. Multi-file compilation via AST merging in `calynda_car.c`. 5 files. |
| **CLI** | `compiler/src/cli/` | Driver, AST/semantic dumpers, assembly/bytecode emitters, native builder. `calynda.c` supports `--target T` before the source file on `asm`/`build`/`run` commands. `runtime.o` resolved relative to executable directory. 10 files. |

### Key Data Flow

- `AstProgram` → `symbol_table_build()` → `SymbolTable`
- `AstProgram` + `SymbolTable` → `type_resolution_resolve()` → resolved types
- `AstProgram` + `SymbolTable` → `type_checker_check_program()` → `TypeChecker` with `CheckedType` per expression
- `AstProgram` + `SymbolTable` + `TypeChecker` → `hir_build_program()` → `HirProgram`
- `HirProgram` → `mir_build_program()` → `MirProgram`
- `MirProgram` → `lir_build_program()` → `LirProgram` (native path)
- `MirProgram` → `bytecode_build_program()` → `BytecodeProgram` (bytecode path)
- `LirProgram` + `TargetDescriptor` → `codegen_build_program()` + `machine_build_program()` → `MachineProgram` → `asm_emit_program()` → assembly text
- Multi-file: `CarArchive` → `calynda_car_merge_ast()` → single `AstProgram` → normal pipeline

### Error Handling Pattern

Every pipeline stage uses the same pattern:
```c
struct XyzBuildError {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool has_related_span;
    char message[256];
};
struct XyzProgram {
    /* ... */
    XyzBuildError error;
    bool has_error;
};
```

## Build System

- **Build**: `cd compiler && make test`
- **Test runner**: `cd compiler && ./run_tests.sh` (runs `make clean && make test`)
- **Prerequisites**: gcc 12.2+, make, Node.js 25.6.1+, npm 11.10.0+
- **Test suites**: 19 suites, 1405+ assertions, all must pass
- **Cross-compilation tests**: gracefully skip when the required toolchain (cross-gcc, QEMU) is absent; `run_tests.sh` reports toolchain availability after the run
- **Install**: `cd compiler && make install PREFIX=/path`
- **Header dependency note**: `compiler/Makefile` does not track header dependencies; token enum changes in `tokenizer.h` require a clean rebuild of all affected binaries/tests to avoid stale-object mismatches

### Build Targets

| Binary | Purpose |
|--------|---------|
| `calynda` | Main compiler/driver CLI (`asm`, `bytecode`, `build`, `run`, `pack` subcommands; `--target x86_64\|aarch64\|riscv64`; `asm`/`build`/`run` accept `.car` archives) |
| `build_native` | Links a native executable; `runtime.o` must sit next to this binary |
| `dump_ast` | AST pretty-printer (supports `--expr` mode for standalone expressions) |
| `dump_semantics` | Semantic state inspector; prints state even when type checking fails |
| `emit_asm` | Assembly text emitter |
| `emit_bytecode` | Bytecode emitter |

### Important Path Rule

`build_native` resolves `runtime.o` relative to the binary's own directory. Always run `compiler/build/test_build_native` from inside `compiler/` — running from the repo root picks up the unrelated top-level `build/build_native` artifact and causes misleading parser failures.

## Bytecode ISA (portable-v1)

18 instructions: `BC_BINARY`, `BC_UNARY`, `BC_CAST`, `BC_CLOSURE`, `BC_CALL`, `BC_MEMBER`, `BC_INDEX_LOAD`, `BC_ARRAY_LITERAL`, `BC_TEMPLATE`, `BC_STORE_LOCAL`, `BC_STORE_GLOBAL`, `BC_STORE_INDEX`, `BC_STORE_MEMBER`, `BC_UNION_NEW`, `BC_UNION_GET_TAG`, `BC_UNION_GET_PAYLOAD`, `BC_HETERO_ARRAY_NEW`, `BC_HETERO_ARRAY_GET_TAG`

4 terminators: `BC_RETURN`, `BC_JUMP`, `BC_BRANCH`, `BC_THROW`

Operand kinds: temp, local, global, constant (pool index). Type-descriptor constants for union generics are interned in the constant pool.

## Native Backends

### x86_64 SysV ELF (primary)

- Return value: `rax`; closure environment: `r15`
- Argument registers: `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`
- Allocatable vregs: `r10`, `r11`, `r12`, `r13` (spill after)
- Direct patterns: scalar arg moves, unary/binary/cast, direct global calls, slot/global stores, return/jump/branch; `inc`/`dec` emitted for pre-increment/pre-decrement
- Runtime-backed: closures, callable dispatch, member/index access, arrays, templates, index/member stores, throw, union ops, typed ptr ops

### AArch64 AAPCS64 ELF

- Closure environment: `x19`; work register: as defined by `TargetDescriptor`
- Standard AAPCS64 argument/return registers
- Assembler output dispatched by `asm_emit_instr_aarch64.c` / `asm_emit_entry_aarch64.c`

### RISC-V RV64GC ELF

- Closure environment: `s1`; epilogue uses `-24(s0)`/`-32(s0)` matching prologue
- Assembler output in `compiler/src/backend/asm_emit/riscv64/` (5 files)
- Cross-compilation tests in `tests/backend/emit/test_asm_emit_cross.c` (4 tests; gracefully skip when toolchain absent)

All three targets share a single target-agnostic Machine and Codegen layer, parameterised by `TargetDescriptor`.

## Design Philosophy

- No interpreter path: compile to native or bytecode, never interpret AST/IR
- Prefer Java-like syntax, then C-like syntax
- Keep language features simple; avoid features that can live in external C/ASM adapters
- V1 grammar is canonical and stable; V2 features must be parser-complete, semantically specified, tested, and mirrored across all tooling (compiler, MCP server, VS Code extension) before promotion
- The `internal` modifier restricts local bindings to nested-lambda-only access; enforced by `validate_internal_access()` walking upward from the resolution scope
- Template interpolation auto-calls zero-argument callable expressions (void return rejected by type checker)
- `stackalloc(size)` means manual-scope scratch storage, not guaranteed machine-stack residency; auto-registers deferred free on scope exit — do not also call `free()` on the same allocation
- The `cleanup` keyword is reserved; use a different identifier (e.g., `disposer`) in test or user code
- `.globl` is emitted for all units; visibility enforcement is deferred until a module loader exists

## MCP Server

The MCP server (`mcp-server/`) is updated for alpha.6. It exposes:

- **Tools**: `analyze_calynda_code`, `complete_calynda_code`, `explain_calynda_syntax`, `explain_compiler_architecture`, `format_calynda_code`, `get_calynda_examples`, `validate_calynda_types`
- **Resources**: grammar (alpha.6 surface), types, keywords, examples (`examples-v3.ts`), compiler architecture, bytecode ISA
- **Prompts**: function writing, debugging, code conversion, pipeline stage explanation

The MCP parser modules (`mcp-server/src/parser/`) handle `arr<?>`, named/generic types, generic args, whole-function manual lambda shorthand, and the alpha.6 surface: untyped `var` parameters, `|var` early-return parameters, the `num` generic numeric type. Tools and resources cover all alpha.6 features in addition to the previously documented surface. All TypeScript source files are ≤250 lines; all directories have ≤15 entries.

## Repository Conventions

- Top-level entries per directory ≤ 15 (the `compiler/build/` directory is build output and excluded). `compiler/src/parser/util/` and `compiler/tests/ir/{bytecode,hir,lir,mir}/` exist to honour this rule.
- Source files target ≤ 250 lines. A subset of files larger than 400 lines is being broken up incrementally; see `RELEASE_NOTES.md`.
