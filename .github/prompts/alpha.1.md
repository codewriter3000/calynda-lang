Read [](file:///home/codewriter3000/Coding/calynda-lang/.github/agents/calynda.agent.md#1-1), lines 1 to 80

Read [](file:///home/codewriter3000/Coding/calynda-lang/1.0.0-alpha.1_ROADMAP.md#50-50), lines 50 to 220

Here are the five agent prompts, ready to dispatch in parallel. Each is self-contained.

---

### Agent 1 — Numeric Literal Underscores

```
You are working on the Calynda compiler (C11) at /home/codewriter3000/Coding/calynda-lang.
Build and test with: cd compiler && make clean && make test
All 1463 existing assertions must still pass when you are done.

## Your task

Implement numeric literal underscore separators in the tokenizer.

### Rules
- `_` is legal anywhere inside an integer or float literal EXCEPT:
  - as the first character of the literal
  - as the last character of the literal
  - immediately before or after a `.` (decimal point)
  - two consecutive underscores `__`
- Underscores are stripped before value conversion; they have no semantic effect.
- Valid in decimal, hex, and float literals: `1_000_000`, `0xFF_00`, `3.141_592`, `1_e10`

### Files to modify
- `compiler/src/tokenizer/tokenizer_literals.c` — strip `_` in the integer and float scan loops; add validation for the illegal placements listed above and return an error token for each.

### Tests to add
Add assertions to an appropriate existing `test_tokenizer_*.c` file (or a new `test_tokenizer_6.c` if all existing files are near the 250-line cap). Cover:
1. `1_000` parses to integer 1000
2. `0xFF_00` parses to integer 65280
3. `3.141_592` parses to float 3.141592
4. `1_e10` parses to float 1e10
5. `_100` is a lex error (leading underscore is an identifier, not a number — this is already handled; just confirm)
6. `100_` is a lex error (trailing underscore in numeric literal)
7. `3._14` is a lex error (underscore adjacent to `.`)
8. `1__0` is a lex error (double underscore)

If you add a new test file, register it in `compiler/Makefile` under `TEST_TOKENIZER_OBJS` and `TEST_SRCS`.

Finish with `cd compiler && make clean && make test` — all suites must be green.
```

---

### Agent 2 — Type Aliases

```
You are working on the Calynda compiler (C11) at /home/codewriter3000/Coding/calynda-lang.
Build and test with: cd compiler && make clean && make test
All 1463 existing assertions must still pass when you are done.

## Your task

Implement transparent type aliases as a new top-level declaration.

### Language surface
```
type Predicate  = (int32) -> bool;
type Grid       = float64[][];
export type OptInt = union { Some(int32), None };
```
Aliases are structural equivalents (not newtype wrappers). Modifiers `export`, `public`, `private` apply. Circular aliases (`type A = A`) must be rejected by type resolution with a clear error.

### Agreed shared constant
The `type` keyword token is `TOK_TYPE`. Check `compiler/src/tokenizer/tokenizer.h` — if it does not already exist, add it to the keyword table in `compiler/src/tokenizer/tokenizer_keywords.c` and to the enum in `tokenizer.h`.

### Files to create/modify

**Tokenizer** (`compiler/src/tokenizer/`)
- Ensure `TOK_TYPE` exists in the keyword table.

**AST** (`compiler/src/ast/`)
- `ast_decl_types.h`: add `AstTypeAliasDecl` struct:
  ```c
  typedef struct {
      char          name[64];
      AstType      *aliased_type;
      bool          is_exported;
      AstSourceSpan span;
  } AstTypeAliasDecl;
  ```
- `ast_types.h`: add `AST_TOP_LEVEL_TYPE_ALIAS` to the top-level kind enum.
- `ast_declarations.c`: add alloc/free for `AstTypeAliasDecl`.
- `ast.h` / `ast.c`: add `AstTypeAliasDecl *` to the top-level union and dispatch.

**Parser** (`compiler/src/parser/`)
- In the top-level declaration parser, handle `TOK_TYPE`: parse `["export"] "type" IDENT "=" Type ";"` and produce an `AST_TOP_LEVEL_TYPE_ALIAS` node.

**Symbol table** (`compiler/src/sema/symbol_table/`)
- Add `SYMBOL_KIND_TYPE_ALIAS` to the symbol kind enum.
- In `st_add_top_level_decl`, register the alias name as a `SYMBOL_KIND_TYPE_ALIAS` symbol, storing a pointer to the `AstType` as the alias body.

**Type resolution** (`compiler/src/sema/type_resolution/`)
- When resolving a named type and the symbol is `SYMBOL_KIND_TYPE_ALIAS`, recurse into the alias body.
- Use a visited-set (array of symbol pointers on the stack is fine) to detect cycles; report an error like `"Type alias 'A' is circular."`.

**Semantic dump** (`compiler/src/sema/`)
- Render alias declarations in the scope dump output: `type Predicate = (int32) -> bool`.

**EBNF** (`compiler/calynda_v2.ebnf`)
- Add:
  ```ebnf
  TypeAliasDecl ::= { Modifier } "type" Identifier "=" Type ";" ;
  ```
  and add `TypeAliasDecl` to `TopLevelDecl`.

### Tests to add (use new files near the 250-line cap boundary)

- **Parser**: 5 assertions — basic alias, alias with modifier, alias of function type, alias of array type, alias of union type.
- **Type resolution**: 5 assertions — alias resolves transparently, chained alias (`type A = B; type B = int32;`), cycle rejection error, alias of `ptr<T>`, alias of generic union.
- **Type checker**: 3 assertions — alias used as parameter type, alias used as return type, mismatched alias assignment rejected.
- **Semantic dump**: 1 assertion — alias appears in dump output.
- **Native build (E2E)**: 1 assertion — program using a type alias for a callback parameter compiles and runs correctly.

Register any new test files in `compiler/Makefile`.

Finish with `cd compiler && make clean && make test` — all suites must be green.
```

---

### Agent 3 — Threading: Compiler (tokenizer → HIR)

```
You are working on the Calynda compiler (C11) at /home/codewriter3000/Coding/calynda-lang.
Build and test with: cd compiler && make clean && make test
All 1463 existing assertions must still pass when you are done.

## Your task

Implement the compiler-side of Calynda multithreading: new keywords, types, spawn expression, and HIR desugaring. You do NOT write the runtime (Agent 4 owns that). You rely on agreed helper symbol names listed below.

## Agreed shared constants (do not change these names)

```c
#define CALYNDA_RT_THREAD_SPAWN   "__calynda_rt_thread_spawn"
#define CALYNDA_RT_THREAD_JOIN    "__calynda_rt_thread_join"
#define CALYNDA_RT_MUTEX_NEW      "__calynda_rt_mutex_new"
#define CALYNDA_RT_MUTEX_LOCK     "__calynda_rt_mutex_lock"
#define CALYNDA_RT_MUTEX_UNLOCK   "__calynda_rt_mutex_unlock"
```

`Thread` and `Mutex` are resolved as built-in types from named-type AST nodes — they are NOT new keywords in the token stream (same as how `string` is handled).

`spawn` IS a new keyword: `TOK_SPAWN`.

## Language surface to implement

```
// Spawn a thread
Thread t = spawn () -> {
    // runs concurrently; must be void return
};

// Join
t.join();

// Mutex
Mutex m = Mutex.new();
m.lock();
m.unlock();
```

## Files to create/modify

**Tokenizer** (`compiler/src/tokenizer/`)
- `tokenizer.h`: add `TOK_SPAWN` to the token enum.
- `tokenizer_keywords.c`: add `"spawn"` → `TOK_SPAWN` to the keyword table.

**AST** (`compiler/src/ast/`)
- `ast_types.h`: add `AST_TYPE_THREAD`, `AST_TYPE_MUTEX` to the type kind enum.
- `ast_expressions.c` / `ast_decl_types.h`: add `AstSpawnExpr { AstExpression *callable; AstSourceSpan span; }` and `AST_EXPR_SPAWN` to the expression kind enum.
- `ast.c`: handle `AST_EXPR_SPAWN` in free/dump dispatch.

**Parser** (`compiler/src/parser/`)
- Parse `"Thread"` and `"Mutex"` as named types producing `AST_TYPE_THREAD` / `AST_TYPE_MUTEX` in the type-parsing path (check how `string` and `ptr` are handled and follow the same pattern).
- Parse `"spawn" UnaryExpression` at unary precedence, producing `AST_EXPR_SPAWN`.

**Type resolution** (`compiler/src/sema/type_resolution/`)
- Add trivial cases for `AST_TYPE_THREAD` and `AST_TYPE_MUTEX`: these are zero-arity built-ins, no components to recurse into. Follow the pattern of `AST_TYPE_PTR`.

**Type checker** (`compiler/src/sema/type_checker/`)
- `AST_EXPR_SPAWN`: verify the operand is a zero-argument callable with void return type. Result type is `CHECKED_TYPE_THREAD`. Error if operand is non-callable or has non-void return.
- Member access `.join()` on a `Thread` value: result void. Error if called on non-Thread.
- Member access `Mutex.new()` (where subject type is the `Mutex` type itself, similar to how union constructors work): result `CHECKED_TYPE_MUTEX`.
- Member access `.lock()` and `.unlock()` on a `Mutex` value: result void. Error if called on non-Mutex.

**HIR** (`compiler/src/hir/`)
- Desugar `AstSpawnExpr` into `HirCall` targeting `__calynda_rt_thread_spawn` with the callable as argument.
- Desugar `.join()` on Thread into `HirCall` to `__calynda_rt_thread_join`.
- Desugar `Mutex.new()` into `HirCall` to `__calynda_rt_mutex_new`.
- Desugar `.lock()` / `.unlock()` into `HirCall` to `__calynda_rt_mutex_lock` / `__calynda_rt_mutex_unlock`.
- Pattern: identical to how `stackalloc` desugars to `__calynda_stackalloc` in `compiler/src/hir/`.

**No changes needed** in MIR, LIR, Codegen, Machine, AsmEmit, or Bytecode — helper calls pass through the existing runtime-backed call path.

## Tests to add

- **Tokenizer**: 1 assertion — `spawn` tokenizes as `TOK_SPAWN`.
- **Parser**: 3 assertions — `spawn` expression with lambda, `Thread` as declared type, `Mutex` as declared type.
- **Type resolution**: 2 assertions — `Thread` resolves as a valid type, `Mutex` resolves as a valid type.
- **Type checker**: 6 assertions:
  - `spawn` with non-callable operand is rejected.
  - `spawn` with non-void callable return is rejected.
  - `.join()` on a non-Thread value is rejected.
  - `.lock()` on a non-Mutex value is rejected.
  - Valid spawn expression has type `Thread`.
  - Valid `Mutex.new()` has type `Mutex`.
- **HIR dump**: 3 assertions — spawn desugars to `__calynda_rt_thread_spawn` call; `.join()` desugars to `__calynda_rt_thread_join`; `Mutex.new()` desugars to `__calynda_rt_mutex_new`.

Register any new test files in `compiler/Makefile`.

Finish with `cd compiler && make clean && make test` — all suites must be green.
Note: the runtime helpers do not exist yet (Agent 4 writes them). Your HIR desugaring emits calls to external symbols; the linker will not be invoked by your tests, so this is fine. The E2E link test is owned by the integration step.
```

---

### Agent 4 — Threading: Runtime (pthreads implementation)

```
You are working on the Calynda compiler (C11) at /home/codewriter3000/Coding/calynda-lang.
Build and test with: cd compiler && make clean && make test
All 1463 existing assertions must still pass when you are done.

## Your task

Implement the pthreads runtime backend for Calynda threading. You do NOT touch any compiler stage (tokenizer through HIR). You work entirely within the runtime and build system.

## Agreed helper symbol names (do not change these)

```c
calynda_word calynda_rt_thread_spawn(calynda_word callable);
void         calynda_rt_thread_join(calynda_word thread_handle);
calynda_word calynda_rt_mutex_new(void);
void         calynda_rt_mutex_lock(calynda_word mutex_handle);
void         calynda_rt_mutex_unlock(calynda_word mutex_handle);
```

`calynda_word` is already defined in runtime.h. Study that file and the existing runtime helpers (e.g., runtime_manual.c) before writing code.

## Files to create/modify

**New file: `compiler/src/runtime/runtime_threads.c`**

Implement the 5 helpers:

- `calynda_rt_thread_spawn(callable)`:
  - Heap-allocate a struct holding the callable word and a `joined` flag (initially false).
  - Call `pthread_create` with a trampoline function.
  - The trampoline calls `calynda_rt_callable_dispatch(callable, NULL, 0)` (existing helper — check its signature in `runtime.h`).
  - Return the heap-allocated struct pointer cast to `calynda_word`.

- `calynda_rt_thread_join(thread_handle)`:
  - Cast the handle back to your struct pointer.
  - If `joined` is true, call `abort()` (double-join).
  - Otherwise call `pthread_join`, set `joined = true`, free the struct.

- `calynda_rt_mutex_new(void)`:
  - Heap-allocate a `pthread_mutex_t`.
  - Call `pthread_mutex_init` with `NULL` attrs.
  - Return the pointer cast to `calynda_word`.

- `calynda_rt_mutex_lock(mutex_handle)`:
  - Cast handle to `pthread_mutex_t *`, call `pthread_mutex_lock`.

- `calynda_rt_mutex_unlock(mutex_handle)`:
  - Cast handle to `pthread_mutex_t *`, call `pthread_mutex_unlock`.

Add `#include <pthread.h>` at the top. Do not add a destructor for mutexes in alpha.1.

**runtime.h**
- Declare all 5 helpers.

**runtime_abi.h**
- Add helper-call entries for the 5 new helpers following the existing pattern.

**Makefile**
- Add `$(RUNTIME_DIR)/runtime_threads.c` to `RUNTIME_OBJS` (find the existing pattern for `runtime_manual.c` and mirror it).
- Add `-lpthread` to the linker flags for `TEST_BUILD_NATIVE_BIN` and the `build_native` tool target (search for `-lm` or similar existing flags as a guide).

## Tests to add

Add assertions to `compiler/tests/backend/emit/test_runtime*.c` (use a new `test_runtime_2.c` if the existing file is near the 250-line limit):

1. `calynda_rt_mutex_new()` returns a non-zero handle.
2. `mutex_lock` then `mutex_unlock` on a fresh mutex does not crash.
3. `calynda_rt_thread_spawn` + `calynda_rt_thread_join` with a no-op callable returns without hanging or crashing.
4. Double-join calls `abort()` — test using the existing `run_process`/signal-death pattern already used in the manual-checked tests.

Register any new test files in the Makefile.

Finish with `cd compiler && make clean && make test` — all suites must be green.
```

---

### Agent 5 — EBNF + MCP Server

```
You are working on the Calynda language tooling at /home/codewriter3000/Coding/calynda-lang.
The MCP server is a TypeScript Node.js project in mcp-server/. Build it with: cd mcp-server && npm install && npm run build
All existing MCP server source files must remain ≤250 lines after your changes.

## Your task

Update the V2 grammar and the MCP server to reflect the three new 1.0.0-alpha.1 features:
1. Numeric literal underscores
2. Type aliases
3. Multithreading (`spawn`, `Thread`, `Mutex`)

## Agreed shared constants

Helper symbol names (for documentation only — no code in this track):
- `__calynda_rt_thread_spawn`, `__calynda_rt_thread_join`
- `__calynda_rt_mutex_new`, `__calynda_rt_mutex_lock`, `__calynda_rt_mutex_unlock`

New token: `TOK_SPAWN` for keyword `spawn`.
New type keywords (resolved as built-in types, not token keywords): `Thread`, `Mutex`.

## Files to modify

### `compiler/calynda_v2.ebnf`

Add the following productions (find the right locations in the existing file):

```ebnf
(* Type alias *)
TypeAliasDecl ::= { Modifier } "type" Identifier "=" Type ";" ;
(* Add TypeAliasDecl to TopLevelDecl alternatives *)

(* Spawn expression — at unary precedence *)
SpawnExpr ::= "spawn" UnaryExpression ;
(* Add SpawnExpr to the primary/unary expression alternatives *)

(* Thread and Mutex as built-in types *)
(* Add "Thread" | "Mutex" to the Type production alongside "string", "bool", etc. *)
```

Also add a comment noting that numeric literal underscores are stripped by the tokenizer (no grammar change needed — the literal productions are unchanged).

### knowledge

Locate the keywords and types knowledge files. Add:
- `spawn` to the keywords list with description: `"Spawns a new thread; right-hand side must be a zero-argument void callable. Returns a Thread handle."`
- `Thread` to the built-in types list with description: `"Opaque handle to a running thread. Created by spawn. Supports .join()."`
- `Mutex` to the built-in types list with description: `"Mutual exclusion lock. Created with Mutex.new(). Supports .lock() and .unlock()."`
- `type` alias syntax to the keywords/declarations section.
- Underscore separator note to the numeric literal section.

### resources

Locate the grammar resource file. Update the version string from `0.4.0` to `0.5.0` and add the new EBNF productions from above to the grammar snapshot.

### package.json

Bump `"version"` from `"0.4.0"` to `"0.5.0"`.

### tools or knowledge

Add a short example to the examples knowledge file showing:
```
// Spawn a thread and join
Thread t = spawn () -> {
    // work here
};
t.join();

// Mutex usage
Mutex m = Mutex.new();
m.lock();
m.unlock();

// Type alias
type Callback = (int32) -> void;

// Literal underscores
int32 million = 1_000_000;
```

Ensure no file exceeds 250 lines. If an edit would push a file over 250 lines, split it following the existing `*_helpers` pattern in the same directory.

Finish with `cd mcp-server && npm run build` — must compile without errors.
``` 

