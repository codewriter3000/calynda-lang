# Plan: Calynda-to-C Transpiler Fork (calynda-c)

## TL;DR

Fork the Calynda compiler, keep the full frontend (tokenizer → parser → symbol table → type checker → HIR), strip everything below HIR (MIR, LIR, codegen, machine, asm_emit, bytecode), and replace it with a C code emitter. The generated C is then compiled by GCC or devkitPPC's powerpc-eabi-gcc for Wii/GameCube targets. The runtime becomes a portable C library.

## Decisions Made

- Fork project, main Calynda continues separately with native x86_64/ARM64 backends
- Features CUT from fork: statement decomposition (#1), mapped functions (#2), bytecode() -> {} (#10), CAR archives (#3)
- Features KEPT: manual(), boot(), asm() (via GCC inline), C FFI
- manual() CAN call non-manual functions (reversed from earlier restriction)
- No bytecode backend in the fork
- GC implemented in C
- #line directives for Calynda→C source mapping
- `calynda build` shells out to gcc/powerpc-eabi-gcc
- Slower compilation acceptable; runtime performance is the priority
- Primary target: Wii/GameCube via devkitPPC (PPC)
- No standard library shipped; users import what they need; basic printing stdlib included

## Architecture

### What Survives (verbatim from main project)

```
Tokenizer (src/tokenizer/)
    → Parser (src/parser/)
        → Symbol Table (src/sema/symbol_table.*)
            → Type Resolution (src/sema/type_resolution.*)
                → Type Checker (src/sema/type_checker.*)
                    → HIR Builder (src/hir/)
                        → [NEW] C Emitter
```

### What Gets Stripped

- src/mir/ — entire MIR layer
- src/lir/ — entire LIR layer
- src/codegen/ — entire codegen planning layer
- src/backend/machine.* — machine emission
- src/backend/asm_emit.* — assembly emission
- src/bytecode/ — bytecode backend
- Tests for stripped modules (test_mir_dump, test_lir_dump, test_codegen_dump, test_machine_dump, test_asm_emit, test_bytecode_dump, test_build_native)

### What Gets Added

- src/c_emit/ — C code emitter (consumes HirProgram, outputs .c file)
- src/c_runtime/ — portable C runtime library (restructured from src/runtime/)
- Makefile changes for cross-compilation support
- CLI changes: `calynda build` → generate .c → invoke gcc

### Emit from HIR (not MIR)

HIR preserves structured programming: blocks, expressions, lambdas with bodies. MIR breaks everything into basic blocks with gotos, which produces ugly unreadable C. HIR gives us:
- CheckedType on every expression (no re-analysis)
- Structured blocks (HirBlock → sequential C statements)
- Lambda bodies (→ C functions + closure structs)
- Symbol metadata (final, exported, static, internal)
- Source spans (→ #line directives)

## Steps

### Phase 1 — Fork & Strip

1. Fork the repository
2. Remove src/mir/, src/lir/, src/codegen/, src/backend/machine.*, src/backend/asm_emit.*, src/bytecode/
3. Remove corresponding test files
4. Remove CLI commands that depend on stripped modules (calynda asm, calynda bytecode)
5. Update Makefile to remove stripped object targets
6. Verify frontend still compiles and frontend tests pass (test_tokenizer through test_hir_dump — 7+ suites)

### Phase 2 — C Runtime Library

7. Restructure src/runtime/ into src/c_runtime/ with clean public header (calynda_runtime.h)
   - Current runtime.h/runtime.c are already C — minimal restructuring needed
   - Runtime helpers (__calynda_rt_closure_new, __calynda_rt_call_callable, etc.) become normal C library functions
   - Build as libcalynda_runtime.a (static archive)
8. GC abstraction layer: create calynda_gc.h with calynda_gc_alloc(), calynda_gc_retain(), calynda_gc_release(), calynda_gc_shutdown(). Initial implementation: reference counting. All runtime object creation routes through this interface so the strategy can be swapped later without touching the emitter or runtime helpers
9. Strip 64-bit types: remove int64, uint64, float64 and their aliases (long, ulong, double) from tokenizer keyword table, type resolution, type checker, and HIR type mapping. CalyndaRtWord = uint32_t
10. Audit runtime for portability: endianness, pointer↔integer casts, replace malloc with calynda_gc_alloc throughout

*Parallel with Phase 2:*

### Phase 3 — C Emitter Core

10. Create src/c_emit/c_emit.h with public API:
    ```
    bool c_emit_program(FILE *out, const HirProgram *program);
    ```
11. Implement type mapping (CheckedType → C type strings):
    - INT8→int8_t, INT16→int16_t, INT32→int32_t
    - UINT8→uint8_t, UINT16→uint16_t, UINT32→uint32_t
    - FLOAT32→float
    - BOOL→bool (stdbool.h), CHAR→char
    - STRING→CalyndaRtWord (runtime object handle)
    - Arrays→CalyndaRtWord (runtime array object)
    - VOID→void
    - Named types (unions)→generated struct names
12. Implement top-level declaration emission:
    - HirBindingDecl (non-callable) → C global variable
    - HirBindingDecl (callable) → C function definition
    - HirStartDecl → main() wrapper calling calynda_rt_start_process
    - HirUnionDecl → C tagged union struct typedef
13. Implement expression emission (all 16 HIR expression kinds):
    - Literals → C literal syntax
    - Symbol references → C variable names (mangled for uniqueness)
    - Binary/unary ops → C operator equivalents
    - Assignments → C assignment
    - Ternary → C ternary
    - Calls → C function calls (direct or via __calynda_rt_call_callable)
    - Index → __calynda_rt_index_load()
    - Member → __calynda_rt_member_load()
    - Cast → __calynda_rt_cast_value() or C cast for scalar-to-scalar
    - Array literal → __calynda_rt_array_literal()
    - Template → __calynda_rt_template_build()
    - Post-increment/decrement → C ++ / --
14. Implement statement emission (4 HIR statement kinds):
    - Local binding → C local variable declaration + initializer
    - Return → C return
    - Throw → __calynda_rt_throw()
    - Expression statement → C expression statement
15. Implement #line directive emission using HirExpression/HirStatement source spans
16. Implement name mangling:
    - Qualified names (package.name) → underscore-separated C identifiers
    - Lambda names → __calynda_lambda_N naming
    - Avoid C keyword collisions

### Phase 4 — Closures & Lambdas (*depends on Phase 3*)

17. For each lambda in HIR:
    - Emit a C struct for captured variables (one field per capture)
    - Emit a C function taking the struct pointer + parameters
    - At the lambda creation site, emit struct initialization + __calynda_rt_closure_new()
18. Determine captures by walking HirLambdaExpression — the symbol table already knows which symbols are captured (SYMBOL_KIND_CAPTURE in parent scopes, plus HIR preserves HirCallableSignature with parameter lists)
19. Handle nested lambdas (lambda within lambda) — nested capture struct chains

### Phase 5 — CLI & Build Pipeline (*depends on Phase 3*)

20. Add calynda_compile_to_c() function in src/cli/calynda_compile.c:
    - Runs frontend through hir_build_program()
    - Calls c_emit_program() to write .c file
21. Update calynda build command:
    - Generate temp .c file
    - Invoke gcc: `gcc -o <output> <temp.c> -I<runtime_include> -L<runtime_lib> -lcalynda_runtime`
    - Or for devkitPPC: `powerpc-eabi-gcc -o <output.elf> <temp.c> -I<runtime_include> -L<runtime_lib> -lcalynda_runtime $(DEVKITPPC_FLAGS)`
22. Add calynda emit-c command to dump generated C to stdout (like current calynda asm)
23. Add --target flag: `calynda build --target wii`, `calynda build --target gc`, `calynda build --target host`

### Phase 6 — manual(), boot(), asm() (*depends on Phase 4*)

24. manual() modifier:
    - Parser already has TOK_MANUAL and AST_STMT_MANUAL
    - In C emitter: manual functions emit raw C types instead of CalyndaRtWord wrappers
    - Enable pointer declarations (int *ptr) and address-of (&var) in manual blocks
    - Emit malloc/realloc/calloc/free as available C calls
    - manual CAN call non-manual: generated C naturally supports this since both are C functions
25. boot() entry point:
    - New AST/parser support needed (currently only start() exists)
    - Emits bare _start symbol or platform-specific entry
    - No runtime initialization (no calynda_rt_start_process)
    - Compile with -ffreestanding -nostdlib
    - For Wii: emit devkitPPC entry point conventions
26. asm() modifier:
    - Emit GCC __asm__ volatile("...") blocks
    - ISA validation: check --target flag against assembly syntax
    - Separate assembly instructions by ; in Calynda source → newlines in __asm__ output
    - Inherit GCC's clobber/constraint system (design specific syntax TBD)

### Phase 7 — C FFI (*depends on Phase 5*)

27. C FFI via extern declarations with Calynda type annotations:
    - Syntax: `extern int printf = c(string fmt, ...);`
    - New TOK_EXTERN keyword, new AST node AST_TOP_LEVEL_EXTERN
    - Parser: `extern` Type Name `=` `c` `(` ParamList `)` `;`
    - Type checker: validate Calynda types in signature, bridge to C types at boundary
    - C emitter: emit forward declaration with mapped C types
    - Add to fork grammar
28. Type bridging: map Calynda types to C types at FFI boundary
    - Calynda string → const char* (automatic unwrap via runtime helper)
    - Calynda int32 → int32_t (direct)
    - Calynda arrays → pointer + length (or raw pointer in manual context)
29. Callback support: passing Calynda lambdas as C function pointers
    - Requires static trampolines or libffi-style closure generation

### Phase 8 — devkitPPC / Wii Integration (*depends on Phases 5, 6*)

30. Create build profiles:
    - host: default, uses system gcc
    - wii: uses powerpc-eabi-gcc from $DEVKITPPC, links libogc
    - gamecube: same toolchain, different link flags
31. Wii-specific runtime porting:
    - **CRITICAL**: Wii is 32-bit PPC. CalyndaRtWord = uint64_t won't work naturally. Must decide: (a) keep 64-bit words with compiler support, or (b) make CalyndaRtWord platform-width (uint32_t on Wii)
    - Audit all pointer ↔ integer casts in runtime
    - malloc → custom allocator or newlib malloc
    - printf → devkitPPC console or no-op
32. Create example Wii project template:
    - Makefile using devkitPPC conventions  
    - Basic libogc initialization in boot() or start()
    - Video/controller setup via C FFI imports

### Phase 9 — Testing (*continuous, starting from Phase 3*)

33. Frontend tests survive verbatim: test_tokenizer, test_parser, test_ast, test_ast_dump, test_symbol_table, test_type_resolution, test_type_checker, test_hir_dump (7+ suites, ~800+ assertions)
34. New test suite: test_c_emit — verify generated C output for:
    - Scalar programs (arithmetic, variables)
    - Function declarations and calls
    - Closures and captures
    - Templates and arrays
    - Unions
    - manual() blocks
35. New test suite: test_c_compile — verify generated C actually compiles with gcc -c
36. Integration tests: full pipeline calynda build → run → verify output
37. Cross-compilation smoke test: verify generated C compiles under powerpc-eabi-gcc

## Relevant Files

### Reused (no changes)
- `compiler/src/tokenizer/` — full tokenizer
- `compiler/src/parser/` — full parser
- `compiler/src/ast/` — full AST
- `compiler/src/sema/` — symbol table, type resolution, type checker
- `compiler/src/hir/` — HIR builder, all lowering modules
- `compiler/calynda.ebnf` — canonical grammar

### Modified
- `compiler/src/cli/calynda.c` — add emit-c and --target flag, remove asm/bytecode commands
- `compiler/src/cli/calynda_compile.c` — add calynda_compile_to_c() entry point
- `compiler/Makefile` — strip backend objects, add c_emit objects, add cross-compile targets

### New
- `compiler/src/c_emit/c_emit.h` — public API
- `compiler/src/c_emit/c_emit.c` — program-level emission orchestration
- `compiler/src/c_emit/c_emit_expr.c` — expression emission
- `compiler/src/c_emit/c_emit_stmt.c` — statement emission
- `compiler/src/c_emit/c_emit_type.c` — type mapping (CheckedType → C strings)
- `compiler/src/c_emit/c_emit_closure.c` — closure struct generation
- `compiler/src/c_emit/c_emit_names.c` — name mangling
- `compiler/src/c_runtime/calynda_runtime.h` — public runtime header
- `compiler/src/c_runtime/calynda_runtime.c` — runtime implementation (restructured from runtime/)
- `compiler/src/c_runtime/calynda_gc.h` — GC abstraction interface (alloc/retain/release/shutdown)
- `compiler/src/c_runtime/calynda_gc_refcount.c` — reference counting GC implementation
- `compiler/calynda_fork.ebnf` — fork grammar (adds boot(), extern c(), removes 64-bit types)
- `compiler/tests/test_c_emit.c` — C emission tests
- `compiler/tests/test_c_compile.c` — gcc compilation tests

### Stripped
- `compiler/src/mir/` — entire directory
- `compiler/src/lir/` — entire directory
- `compiler/src/codegen/` — entire directory
- `compiler/src/backend/machine.*` — machine emission
- `compiler/src/backend/asm_emit.*` — assembly emission
- `compiler/src/bytecode/` — entire directory

## Verification

1. After Phase 1: `make test` passes all frontend suites (test_tokenizer through test_hir_dump)
2. After Phase 3: `calynda emit-c hello.cal` produces compilable C for a minimal program; `gcc -c output.c` succeeds
3. After Phase 4: programs with lambdas/closures produce correct C; closure capture values are correct at runtime
4. After Phase 5: `calynda build hello.cal -o hello && ./hello` produces correct output
5. After Phase 6: manual() programs with malloc/free compile and run; boot() produces freestanding binary; asm() emits GCC inline assembly
6. After Phase 7: Can call printf from Calynda via C FFI import
7. After Phase 8: `calynda build --target wii hello.cal` produces .elf that runs on Dolphin emulator
8. Continuous: all test suites green after each phase

## Resolved Decisions

1. **Remove 64-bit types from the fork.** No int64, uint64, float64 (and their aliases long, ulong, double). CalyndaRtWord = uint32_t. Solves 32-bit PPC word size cleanly. Requires: strip these from tokenizer keyword table, type resolution, type checker, HIR type mapping. Grammar diverges from main project here.
2. **GC: Reference counting first, behind a swappable abstraction.** All heap object allocation/deallocation goes through a GC interface (calynda_gc_alloc, calynda_gc_retain, calynda_gc_release, calynda_gc_shutdown). Initial impl is refcount. If cycles become a problem, swap to mark-and-sweep or Boehm behind the same interface without changing the emitter.
3. **C FFI: Extern declarations with Calynda type annotations.** Syntax: `extern int printf = c(string fmt, ...);` — explicit per-function, type-safe at the Calynda boundary. Parser needs new `extern` keyword + `c(...)` foreign signature syntax.
4. **asm() uses GCC inline asm syntax verbatim.** No Calynda-native constraint syntax. Emitter wraps user asm in __asm__ volatile("...").
5. **boot() added to the fork's grammar.** New keyword `boot`, new token TOK_BOOT, new AST node AST_TOP_LEVEL_BOOT, grammar rule parallel to start(). Fork grammar diverges from main project; main project does not add boot().
