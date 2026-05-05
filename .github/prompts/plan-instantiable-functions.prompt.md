# Plan: Instantiable Functions (`inst`)

## Feature Summary

An `inst` declaration defines a **named callable type** whose instances carry
construction-time parameters as persistent mutable state. Each construction call
produces a new, independently stateful value whose type is the declared name.
Instances are callable via the associated call signature. The feature fills the
gap between anonymous closure factories (which produce structural/unnamed types)
and full class declarations (which would bring inheritance and `this` complexity
that conflicts with Calynda's simplicity mandate).

---

## Design Questions and Answers

These are the questions that need to be resolved before implementation begins.
Answers are provided here based on the existing language design and philosophy;
review and override any that differ from the original intent.

### Q1: What does "instantiable" mean — persistent state, generic type params, or something else?

**A:** I want to bring OOP-style **stateful instances** to Calynda that combine the

### Q2: Why not closure factories? What gap does this fill?

**A:** The current closure factory pattern works:

```cal
final makeCounter = (int32 init) -> {
    return () -> { init += 1; return init; };
};
final c = makeCounter(0);
```

But `c` has the structural type `int32`. This means:

- You cannot declare a parameter of type "a counter" distinct from "any
  zero-arg int32 callable". `(Counter fn)` is impossible today.
- Union variants cannot carry a named counter type: `union Action { Inc(Counter), Dec(Counter) }` is impossible.
- `Counter[]` cannot be typed as a homogeneous array of counters.

`inst` provides a **nominal type** for stateful callables. The runtime
representation is still a closure; the value-add is entirely at the type-system
level.

### Q3: Why not use `new` for construction?

**A:** Consistency with union variant constructors. `Some(42)` and `None` are
called without `new`; `Counter(0)` follows the same convention. The `inst`
keyword at the declaration site is enough to distinguish a type-producing call
from a regular function call.

### Q4: Does an `inst` type have a structural relationship to the matching lambda type?

**A:** No. `Counter` and `() -> int32` are nominally distinct. Two `inst`
declarations with identical signatures are also distinct types. This avoids
ambiguity in overload resolution and keeps assignment checking simple. If
structural compatibility is needed, the programmer wraps the instance:
`() -> int32 fn = () -> c();`.

### Q5: Can multiple independent instances co-exist?

**A:** Yes — that is the core semantic. `Counter(0)` and `Counter(10)` produce
independent values, each with its own state copy.

### Q6: Is construction-param state mutable across calls?

**A:** Yes, by default. A construction param behaves like a captured mutable
local. Mutations in one call are visible in the next call on the same instance.
There is no `final` qualifier on individual construction params in v1 — if the
call body never mutates a param, it is effectively read-only. Different instances
are always independent.

### Q7: Why should this be a first-class feature rather than a library pattern?

**A:** The named-type benefit cannot be achieved with existing closures, layouts,
or manual memory. `layout` gives named struct types but not callable semantics.
`union` gives nominal tagged types but not callable semantics. Nothing today
gives both. `inst` is the minimum addition that bridges that gap without
introducing class hierarchies, `this` references, virtual dispatch, or any
machinery beyond what closures already provide at the runtime layer.

---

## Resolved Design Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Keyword | `inst` | Parallel to `union` and `layout` |
| Construction syntax | `Counter(args)` — no `new` | Consistent with union constructors |
| State mutability | Mutable by default | Matches existing local/capture semantics |
| Type identity | Nominal | Avoids coercion complexity; consistent with union types |
| Multiple call signatures | Not supported in v1 | Keeps the feature focused; multiple behaviours → multiple inst types |
| Generics (`inst<T>`) | Deferred | Composable later; not needed to deliver the core value |
| Modifier set | Same as `BindingDecl` (`export`, `public`, `private`, `final`, `static`) | No new access rules needed |
| Top-level only in v1 | Yes | Local `inst` declarations add scoping complexity with little benefit |
| Structural coercion | None implicit; wrap with a lambda if needed | Nominal parity with union types |
| Recursive call body | Not permitted in v1 | Circular self-reference through the inst instance would require two-phase capture; defer |

---

## Language Surface

### EBNF Addition

```ebnf
TopLevelDecl
    = StartDecl
    | BootDecl
    | BindingDecl
    | TypeAliasDecl
    | UnionDecl
    | LayoutDecl
    | AsmDecl
    | InstDecl                                    (* NEW *)
    ;

(* Named callable type with construction-time state parameters.      *)
(* The declared Type is the return type of the call body, not the    *)
(* type of the instance (the instance type is the Identifier name).  *)
(* The RHS LambdaExpression is the call body; it closes over the     *)
(* construction parameters, which become per-instance mutable state. *)
(* Each construction call creates an independent instance.           *)
(*                                                                   *)
(* Examples:                                                         *)
(*   inst int32 Counter(int32 init)                                  *)
(*       = () -> { init += 1; return init; };                        *)
(*                                                                   *)
(*   export inst int32 Adder(int32 base)                             *)
(*       = (int32 n) -> base + n;                                    *)
(*                                                                   *)
(*   inst void Printer(string prefix)                                *)
(*       = (string msg) -> io.print(`${prefix}: ${msg}`);           *)
InstDecl
    = { Modifier } "inst" Type Identifier "(" [ ParameterList ] ")" "=" LambdaExpression ";"
    ;
```

### Type System Rules

```
1. INST DECLARATION
   inst RT Name(T1 p1, ...) = (A1 a1, ...) -> body;
   → Declares Name as a named type (SYMBOL_KIND_INST).
   → Name is a callable constructor: Name(T1...) → Name.
   → An instance of type Name is callable: Name(value)(A1...) → RT.
   → RT and Ai must not be void (construction params may be void-typed
     only if there are no construction params, i.e. Name()).

2. CONSTRUCTION EXPRESSION
   Name(e1, ..., en)
   → typeof(Name(e1, ..., en)) = Name
   → Argument types must match the declared construction ParameterList.

3. INVOCATION EXPRESSION
   c(e1, ..., em)  where typeof(c) = Name (SYMBOL_KIND_INST)
   → typeof(c(e1, ..., em)) = RT   (the declared call-body return type)
   → Argument types must match the LambdaExpression's ParameterList.

4. ASSIGNMENT / PARAMETER PASSING
   Nominal: both sides must resolve to the same Name.
   No implicit coercion to a structural lambda type.

5. ARRAY / UNION USAGE
   Name[]  is a valid array type.
   union Foo { Bar(Name) }  is valid; Name is a valid payload type.

6. MODIFIERS
   export / public / private / final / static follow existing BindingDecl
   semantics applied to the inst declaration as a whole.
```

### Example Programs

```cal
(* Stateful counter ─ construction param is mutable state *)
inst int32 Counter(int32 init) = () -> {
    init += 1;
    return init;
};

start -> {
    Counter c1 = Counter(0);
    Counter c2 = Counter(100);
    io.print(`${c1()} ${c1()} ${c2()}`);   (* "1 2 101" *)
    return 0;
};
```

```cal
(* Curried adder ─ no state mutation, read-only construction param *)
inst int32 Adder(int32 base) = (int32 n) -> base + n;

start -> {
    Adder add5 = Adder(5);
    Adder add10 = Adder(10);
    io.print(`${add5(3)} ${add10(3)}`);    (* "8 13" *)
    return 0;
};
```

```cal
(* Union storing inst instances *)
union Transform { Scale(Adder), Shift(Counter) };

start -> {
    Adder a  = Adder(2);
    Counter c = Counter(0);
    Transform t1 = Scale(a);
    Transform t2 = Shift(c);
    return 0;
};
```

```cal
(* Array of same inst type *)
inst int32 Accumulator(int32 sum) = (int32 v) -> {
    sum += v;
    return sum;
};

start -> {
    Accumulator[3] accs = [Accumulator(0), Accumulator(10), Accumulator(100)];
    accs[0](5);
    accs[1](5);
    accs[2](5);
    io.print(`${accs[0](0)} ${accs[1](0)} ${accs[2](0)}`);  (* "5 15 105" *)
    return 0;
};
```

---

## Compiler Pipeline Impact

### Stage 1 — Tokenizer

**Files:** `src/tokenizer/tokenizer.h`, `tokenizer_keywords.c`,
`tokenizer_type_names.c`

- Add `TOK_INST` between existing keyword tokens (alphabetical order near
  `TOK_INTERNAL`).
- Register `"inst"` in the keyword table.
- Add the `"INST"` string name for diagnostics.

No changes to the lexer state machine; `inst` is a plain keyword.

---

### Stage 2 — AST

**Files:** `src/ast/ast_types.h`, `src/ast/ast_decl_types.h`,
`src/ast/ast_declarations.c`, `src/ast/dump/` (AST dump)

**New AST node:**

```c
typedef struct {
    AstSourceSpan       span;
    AstModifiers        modifiers;
    AstType             return_type;    /* call-body return type        */
    char               *name;          /* inst type name               */
    AstParameterList    construction_params;
    AstLambdaExpression call_body;     /* the RHS lambda expression    */
} AstInstDecl;
```

- Add `AST_TOP_LEVEL_INST` to `AstTopLevelDeclKind`.
- Add an `inst_decl` arm to `AstTopLevelDecl.as`.
- Add `ast_inst_decl_free()` to `ast_declarations.c`.
- Add AST dump support: render construction params, return type, and call body.

---

### Stage 3 — Parser

**Files:** `src/parser/parser_decl.c`, `src/parser/parser.c`

- Add `parse_inst_decl()` handling `TOK_INST`:
  1. Parse modifiers (already hoisted before the `inst` token).
  2. Consume `TOK_INST`.
  3. Parse return type (`parse_type()`).
  4. Parse name (identifier).
  5. Parse `(` construction params `)`.
  6. Consume `=`.
  7. Parse `LambdaExpression` (call body).
  8. Consume `;`.
- Wire into `parse_top_level_decl()` with the `TOK_INST` lookahead check.
- Add `looks_like_inst_decl()` lookahead helper that peeks for `inst` (after
  any leading modifiers) to disambiguate from binding decls.

---

### Stage 4 — Symbol Table

**Files:** `src/sema/symbol_table/symbol_table.h`,
`symbol_table_predecl.c`, `symbol_table_imports.c`, `symbol_table.c`

**New symbol kind:**

```c
SYMBOL_KIND_INST   /* after SYMBOL_KIND_LAYOUT */
```

**New scope kind** (for the construction-param scope inside an inst decl):

```c
SCOPE_KIND_INST    /* after SCOPE_KIND_UNION */
```

**Predeclaration** (in `symbol_table_predecl.c`, loop over `AST_TOP_LEVEL_INST`):

1. Create a `SYMBOL_KIND_INST` symbol for the inst name in program scope.
2. Store `generic_param_count = 0` (no generics in v1).
3. Store construction parameter count and the `AstInstDecl` as `declaration`.
4. Store the call-body return type and call-body parameter list on the symbol
   via the existing `has_external_return_type` / `external_parameters` fields
   (reuse without new fields if they fit; otherwise add `has_call_signature`
   and `call_parameters` fields to `Symbol`).
5. Create a child `SCOPE_KIND_INST` scope and predeclare each construction
   parameter as a `SYMBOL_KIND_PARAMETER` symbol in that scope (analogous to
   how lambda parameter scopes are built during type checking today, but done
   at predeclaration time so construction-call sites can validate arg count
   early).

**Overload sets:** Inst names participate in the overload-set deduplication
check. Two inst declarations with the same name are a redeclaration error (same
as duplicate bindings today).

---

### Stage 5 — Type Resolution

**Files:** `src/sema/type_resolution/type_resolution.c` (or whichever
file handles top-level binding type resolution)

- Resolve the declared `return_type` of each `AstInstDecl` (the call-body
  return type).
- Resolve each construction parameter type.
- Resolve each call-body (lambda) parameter type.
- Reject `void` return types and `void`-typed construction params following
  existing void-rejection rules.

---

### Stage 6 — Type Checker

**Files:** `src/sema/type_checker/`, primarily
`type_checker_expr_ext.c` / `type_checker_expr_more.c` and
`type_checker_decl.c` (if it exists) / main program loop

**New checked type kind** (reuse `CHECKED_TYPE_NAMED` — inst instances are
named types):

- The `name` field of `CheckedType` will hold the inst name.
- The type checker must distinguish `SYMBOL_KIND_INST` from `SYMBOL_KIND_UNION`
  when resolving named types (the call semantics differ).

**Type checker changes:**

1. **Program loop** — add a `case AST_TOP_LEVEL_INST:` that type-checks the
   call-body lambda expression in a scope that includes the construction
   parameters. Set the expected return type to the declared `return_type`.

2. **Construction call** — when a call expression's callee resolves to a
   `SYMBOL_KIND_INST` symbol:
   - Check argument count and types against `construction_params`.
   - Return type = `CHECKED_TYPE_NAMED(name)` with a `TypeCheckInfo` that has
     `is_callable = true`, `callable_return_type` = the declared return type,
     and `parameters` = the call-body parameters.
   - Record the checked type on the call expression.

3. **Invocation on an inst instance** — when a call expression's callee type is
   `CHECKED_TYPE_NAMED` and resolves to a `SYMBOL_KIND_INST`:
   - Check argument count and types against the inst's call-body parameter list.
   - Return the declared call-body return type.

4. **Assignment / parameter compatibility** — `tc_checked_type_assignable()`:
   two `CHECKED_TYPE_NAMED` types are assignable only if their `name` fields
   match. No structural coercion.

5. **Inst as array element type** — `CHECKED_TYPE_NAMED` with an inst backing
   symbol is valid in array types (no change needed beyond correct named-type
   resolution).

6. **Semantic dump** — render inst declarations and their construction scopes
   (analogous to union declarations today).

---

### Stage 7 — HIR

**Files:** `src/hir/hir.h`, `src/hir/hir_types.h` (if it exists),
`src/hir/hir.c`, `src/hir/hir_dump.c` / `hir_dump_helpers.c`

**New HIR node:**

```c
typedef struct {
    const Symbol       *symbol;           /* SYMBOL_KIND_INST              */
    char               *name;
    CheckedType         return_type;      /* call-body return type          */
    HirCallableSignature construction_sig;/* construction param signature   */
    HirCallableSignature call_sig;        /* call-body param signature      */
    HirLambdaExpression *call_body;       /* owned; body closes over ctors  */
} HirInstDecl;
```

- Add `HIR_TOP_LEVEL_INST` to the HIR top-level kind enum.
- Add `inst_decl` arm to the HIR top-level union.
- HIR lowering from `AstInstDecl`:
  1. Build `HirInstDecl.construction_sig` from `AstInstDecl.construction_params`.
  2. Build `HirInstDecl.call_sig` from `AstLambdaExpression.parameters`.
  3. Lower the call-body lambda into a `HirLambdaExpression` whose parameter
     scope is the call-body params and whose capture scope includes the
     construction params (mark each construction param as a capturable local in
     the HIR's symbol resolution context).
  4. Preserve the inst `Symbol*`.

- **Construction call sites** in HIR call expressions: the checked type on a
  construction call already carries `CHECKED_TYPE_NAMED(Counter)` with callable
  metadata; no extra HIR node kind is needed. HIR call expressions are lowered
  uniformly — MIR lowering will special-case based on callee symbol kind.

- **HIR dump**: render `inst Name(ctor_sig) -> call_sig { body }`.

---

### Stage 8 — MIR

**Files:** `src/mir/mir.h`, `src/mir/lower/mir_decl.c` (or equivalent),
`src/mir/lower/mir_lambda.c`, `src/mir/lower/mir_expr.c`

An `HirInstDecl` lowers into **two `MirUnit`s**. No new `MirUnitKind` values
are needed — both reuse existing kinds:

```
MIR_UNIT_LAMBDA   — the call body unit (captures = construction params)
MIR_UNIT_BINDING  — the constructor unit (creates closure; returns inst value)
```

**Constructor unit** (`calynda_unit_<Name>_new`):

```
parameters: [T1 p1, T2 p2, ...]   (* construction params *)
body:
  t0 = closure(calynda_unit_<Name>_call, [p1, p2, ...])
  return t0
```

**Call body unit** (`calynda_unit_<Name>_call`):

```
captures: [T1 p1, T2 p2, ...]     (* construction params as captures *)
parameters: [A1 a1, A2 a2, ...]   (* call-body params *)
body:
  < lowered from HirInstDecl.call_body >
```

**Construction call sites** in `mr_lower_call_expression()`:

- Detect that the callee symbol is `SYMBOL_KIND_INST`.
- Emit a `MIR_INSTR_CALL` targeting the constructor unit name
  (`calynda_unit_<Name>_new`).
- The result temp has type `CHECKED_TYPE_NAMED(Name)`.

**Invocation call sites** — already handled by existing callable dispatch
(`MIR_INSTR_DISPATCH` or the direct-call path), because the inst value is
a closure at runtime. No changes needed beyond ensuring the callee temp's
type carries the callable metadata correctly.

**Module init** — inst declarations do not emit global-init instructions (the
constructor function itself is the factory; there is no static instance to
initialize).

---

### Stage 9 — LIR / Codegen / Machine / Bytecode

No new LIR instruction kinds, codegen patterns, machine instructions, or
bytecode opcodes are needed. Inst instances are closures at every layer below
MIR. The existing closure lowering chain handles them transparently once MIR
emits `MIR_INSTR_CLOSURE` and `MIR_INSTR_DISPATCH` (or direct callable call)
correctly.

Verify through end-to-end `test_build_native` and `test_bytecode_dump` tests
rather than adding new cases at every layer.

---

## Implementation Checklist

### Phase 1 — Tokenizer, AST, Parser

- [ ] `tokenizer.h` — add `TOK_INST` (alphabetical position after `TOK_INTERNAL`)
- [ ] `tokenizer_keywords.c` — register `"inst"` → `TOK_INST`
- [ ] `tokenizer_type_names.c` — add `"INST"` case
- [ ] `ast_decl_types.h` — add `AstInstDecl` struct
- [ ] `ast_types.h` — add `AST_TOP_LEVEL_INST` to `AstTopLevelDeclKind`; add `inst_decl` arm to `AstTopLevelDecl.as`
- [ ] `ast_declarations.c` — implement `ast_inst_decl_free()`
- [ ] `ast/dump/` — render `AstInstDecl` in the AST dump
- [ ] `parser_decl.c` — implement `parse_inst_decl()` and `looks_like_inst_decl()` lookahead
- [ ] `parser.c` — wire `TOK_INST` lookahead into `parse_top_level_decl()`
- [ ] `tests/frontend/test_tokenizer.c` — `inst` keyword token
- [ ] `tests/frontend/test_parser.c` — basic inst decl, zero-param ctor, both-param case
- [ ] `tests/frontend/test_ast_dump.c` — inst decl dump format

### Phase 2 — Semantics

- [ ] `symbol_table.h` — add `SYMBOL_KIND_INST`, `SCOPE_KIND_INST`; add call-signature storage fields to `Symbol` if the existing external fields are insufficient
- [ ] `symbol_table_predecl.c` — predeclare inst symbols; open `SCOPE_KIND_INST` scopes; predeclare construction params
- [ ] `type_resolution.c` — resolve construction param types, call return type, call-body param types for inst decls
- [ ] Type checker program loop — `case AST_TOP_LEVEL_INST:` entry
- [ ] `type_checker_expr_ext.c` — construction call handling (callee is `SYMBOL_KIND_INST`)
- [ ] `type_checker_expr_more.c` — invocation on inst-typed expression; nominal type-assignability for `CHECKED_TYPE_NAMED` with inst backing
- [ ] `semantic_dump.c` — render inst declarations and `SCOPE_KIND_INST` scopes
- [ ] `tests/sema/test_symbol_table.c` — inst predeclaration, scope kind
- [ ] `tests/sema/test_type_resolution.c` — construction param and return type resolution
- [ ] `tests/sema/test_type_checker.c` — construction call, invocation, nominal assignment, cross-type rejection, wrong-arg-count errors
- [ ] `tests/sema/test_semantic_dump.c` — inst in dump output

### Phase 3 — HIR

- [ ] `hir.h` — add `HIR_TOP_LEVEL_INST`, `HirInstDecl`
- [ ] `hir.c` — lower `AstInstDecl` to `HirInstDecl`; lower construction params as capturable in the call-body scope
- [ ] `hir_dump.c` / `hir_dump_helpers.c` — render `HirInstDecl`
- [ ] `tests/ir/test_hir_dump.c` — inst HIR structure; construction-param capture

### Phase 4 — MIR

- [ ] `mir_decl.c` (or wherever top-level HIR decls are lowered) — lower `HirInstDecl` into constructor `MIR_UNIT_BINDING` + call `MIR_UNIT_LAMBDA`
- [ ] `mir_expr.c` — in `mr_lower_call_expression()`, detect `SYMBOL_KIND_INST` callee and route to constructor unit name
- [ ] `tests/ir/test_mir_dump.c` — constructor unit shape, call unit capture list, construction call site

### Phase 5 — End-to-End Validation

- [ ] `tests/backend/emit/test_build_native.c` — counter (stateful mutation across calls), adder (read-only construction param), two independent instances, `Counter[]` array
- [ ] `tests/backend/test_bytecode_dump.c` (optional) — inst constructor + call unit in bytecode form
- [ ] `./compiler/run_tests.sh` — all suites pass (target: current count + inst suite additions)

### Phase 6 — MCP Server and VS Code Extension (post-compiler)

- [ ] `mcp-server/src/parser/parser.ts` — parse `inst` declarations
- [ ] `mcp-server/src/analyzer/types.ts` — add `InstCalyndaType`; `typesCompatible` for inst nominal matching
- [ ] `mcp-server/src/knowledge/` — document the inst feature
- [ ] `vscode-calynda/syntaxes/calynda.tmLanguage.json` — highlight `inst` as a keyword; highlight inst type names as `entity.name.type.inst.calynda`

---

## Out of Scope (Deferred)

- **Generic inst declarations** (`inst<T> T Wrapper(T value) = (T fn) -> fn(value)`) — needs type-param inference or explicit instantiation syntax; composable after generic-functions work lands.
- **Multiple call signatures on one inst** — would require a dispatch table; closer to a class than a callable. Use a union of inst types instead.
- **Recursive call body** — the call body calling back through the inst instance requires two-phase capture or a `self` reference; deferred until the recursive-lambda pattern is cleaner.
- **Local inst declarations** — `inst` inside a block scope; adds scoping complexity with limited benefit. Closures satisfy the same local use case.
- **Structural coercion** — assigning an inst instance to a lambda variable without an explicit wrap; would complicate overload resolution.
