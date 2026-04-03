# Calynda V2 Language Specification

> This document is the authoritative specification for Calynda V2.
> It supersedes all prior wishlist content and defines the agreed language surface,
> delivery plan, and outstanding decisions.

---

## 1. Philosophy

Calynda V2 is a compatibility-oriented evolution aimed at developers coming from
Java 8 or C, while preserving V1 as the canonical shipped language until each V2
slice is parser-complete, semantically specified, and mirrored across compiler and
tooling.

1. **Prioritize Java and C compatibility.** Make the language look like Java first,
   or C when Java conventions do not apply.
2. **Make the language easy to pick up** for developers coming from Java 8 or C.
3. **If it can be built without needing external C or Assembly adapters, it is
   unnecessary.** The language should not introduce surface area that exists only
   to expose lower-level primitives already reachable through external tooling.

---

## 2. V2 Grammar Sandbox

The working V2 grammar lives at `compiler/calynda_v2.ebnf`. All experimental
grammar changes must happen in this file.

The canonical V1 grammar at `compiler/calynda.ebnf` **must not be modified** until
a V2 slice is fully validated — parser-complete, semantically specified, tested,
and mirrored across the compiler and tooling.

---

## 3. Agreed V2 Surface

### 3.1 Primitive Aliases

Support both Calynda primitive names **and** Java-style aliases:

| Calynda Name | Java-Style Alias |
|--------------|------------------|
| `uint8`      | `byte`           |
| `int8`       | `sbyte`          |
| `uint16`     | `char`           |
| `int16`      | `short`          |
| `uint32`     | `uint`           |
| `int32`      | `int`            |
| `uint64`     | `ulong`          |
| `int64`      | `long`           |
| `bool`       | `bool`           |
| `float32`    | `float`          |
| `float64`    | `double`         |

Both notation systems remain in the language. Java-style aliases are primarily a
transition aid for developers coming from Java; C developers may already be
familiar with names like `char` and `double` that overlap with Calynda-native
types. The Calynda-native names are the canonical form.

### 3.2 Reified Generics

Java-style generic syntax: `<T>`, `<K, V>`, and wildcard `<?>`.

Generics are **runtime-reified** — type information is preserved and accessible at
runtime, unlike Java's type erasure.

### 3.3 Heterogeneous Arrays

`arr<?>` replaces the old struct concept. Heterogeneous arrays allow each index to
hold a value of a different type.

- Array literal layout is computed at **compile time**.
- Each index has a **fixed shape after creation** — the per-index type cannot
  change after the array is constructed.

### 3.4 Tagged Unions

Unions remain in the V2 surface but only as **tagged unions**. Every union value
carries a discriminant tag identifying the active variant at runtime.

### 3.5 Internal Nested Helpers

The `internal` modifier on nested functions enables loop-library extensibility
patterns such as local `break` and `continue` helpers.

- `internal` nested functions are callable **only** inside the intended nested
  callable contexts used by the enclosing function's argument functions.
- This is enforced as a **semantic rule**, not merely syntax. The compiler must
  verify call-site validity during semantic analysis.

### 3.6 Manual Memory

`manual()` is an explicitly **unsafe boundary** with C-style pointer/reference
semantics and allocation helpers:

- `*` and `&` — pointer dereference and address-of, as in C.
- `malloc` — allocate memory for a given type.
- `calloc` — allocate and zero-initialize contiguous memory.
- `realloc` — resize a previously allocated block.
- `free` — release memory associated with a pointer.

`manual()` is **intentionally unsafe**. It tolerates runtime-fatal misuse (e.g.,
use-after-free, double-free) instead of imposing ownership or lifetime rules. This
is a deliberate design choice to stay close to C semantics.

### 3.7 Static/Private Initialization

Calynda V2 retains a static/private top-level initialization model. Static
variables are stored in the data segment. Static function forms and private
initialization blocks follow C-inspired conventions.

### 3.8 Prefix/Postfix Operators

`++` and `--` are supported as both prefix and postfix operators.

### 3.9 Varargs

Java/C-style variadic parameters are supported:

```calynda
int add = (int... nums) -> {};
```

The variadic parameter is treated as an array within the function body.

### 3.10 Discard Expression

`_` is a **discard expression**. It signals that a value is intentionally unused.

### 3.11 Import Model

The V2 import system supports five forms with explicit export control and
compile-time ambiguity checking.

#### Plain Import

```calynda
import foo.bar;
```

Imports the module and makes it available via its fully qualified name.

#### Module Alias

```calynda
import foo.bar as baz;
```

Imports the module under an alias for shorter references.

#### Wildcard Import (Direct Exports Only)

```calynda
import foo.bar.*;
```

Imports all directly exported names from the target module. Wildcard imports bind
**only** the names explicitly exported by the target module — they do not
transitively re-export names that the target itself imported.

#### Selective Import

```calynda
import foo.bar.{a, b, c};
```

Imports specific named exports from a module.

#### Explicit Exports

Top-level declarations must be explicitly marked for export. Only exported names
are visible to importers. Importable names are intentional, not accidental.

#### Ambiguity Rules

Ambiguous bare names (e.g., two wildcard imports that both export a name `x`) are
a **compile error**. The developer must resolve the ambiguity through explicit
qualification or aliasing. There is no implicit filename/directory fallback.

---

## 4. Deferred Items

| Item | Reason |
|------|--------|
| `asm()` | Deferred unless it solves a real problem that external adapters cannot solve. Will be re-evaluated only after the `manual()` model exists and only if a clear language-level justification remains (see Phase 4, step 12). |
| One-statement callback shorthand | Deferred pending explicit decision. Auto-lifting semantics must be frozen before implementation. Without a precise rule, the shorthand risks looking like immediate evaluation rather than deferred callback creation. |
| Transitive wildcard imports | Out of the first import design. Wildcard imports bind only directly exported names from the target module. |

---

## 5. Rejected Items

| Item | Reason |
|------|--------|
| Structs | Replaced by `arr<?>`. There is no separate struct feature in the agreed V2 set. |
| Enums | Removed from the agreed V2 set. |
| Cryptographic verification for asm imports | Removed. |
| Implicit filename/directory fallback for import conflicts | Rejected. Ambiguous names must be resolved explicitly by the developer. |

---

## 6. Phased Delivery Plan

### Phase 0 — Spec & Grammar Foundation

> *Blocks all implementation work.*

1. **Rewrite the spec.** Replace the old wishlist (`version_wishlist.md`) with this
   authoritative V2 spec document. Replace old philosophy wording, remove rejected
   features, add the agreed import model, and split content into agreed, deferred,
   and rejected sections.

2. **Preserve the grammar sandbox.** Keep `compiler/calynda_v2.ebnf` as the only
   active V2 grammar sandbox. Do not update the canonical V1 grammar until a V2
   slice is complete and validated.

3. **Spec the import system.** Convert the import system from a single
   `import QualifiedName;` rule into a full module system spec with direct import,
   alias import, wildcard import, selective import, and explicit export syntax.
   Freeze the grammar and name-resolution rules before coding.

### Phase 1 — Low-Risk Front-End

> *Depends on Phase 0.*

4. **Implement syntax features.** Implement the low-risk front-end slice in the
   compiler: primitive aliases, `++`/`--`, varargs, `_` discard expression, and the
   revised static/private initialization surface.

5. **Implement import/export.** Parse `as`, `*`, and `{a, b, c}` import forms; add
   `export` for top-level declarations; make ambiguous bare-name resolution a
   diagnostic. Can run in parallel with late testing for step 4.

### Phase 2 — Semantic Features

> *Depends on Phase 1.*

6. **Internal nested-function visibility.** Add `internal` nested-function
   visibility as a semantic rule rather than only syntax. The implementation must
   enforce that internal helper functions are callable only inside the intended
   nested callable contexts used by loop-library patterns.

7. **Callback shorthand decision.** Decide whether the one-statement callback
   shorthand survives. If accepted, implement it as context-sensitive callable
   auto-lifting based on expected parameter type rather than ordinary expression
   parsing. Intentionally not part of the first execution slice.

### Phase 3 — Runtime Type Model

> *Depends on Phase 1.*

8. **Reified generics runtime model.** Design the runtime type model for reified
   generics. Must define AST representation, type resolution, checked-type
   metadata, wildcard handling, runtime descriptors, and backend-visible lowering
   rules before any generic implementation begins.

9. **Heterogeneous arrays.** Implement `arr<?>` as the replacement for the old
   struct idea. Define exact rules for literal layout, indexing, mutation, casts,
   equality, dump rendering, backend lowering, and GC scanning. Depends on step 8.

10. **Tagged unions.** Implement tagged unions on top of the same runtime metadata
    model used by reified generics and heterogeneous arrays, so Calynda does not
    end up with multiple incompatible tagged-layout systems. Depends on steps 8
    and 9.

### Phase 4 — Unsafe Boundaries

> *Depends on Phase 3.*

11. **Manual memory.** Add `manual()` as an explicitly unsafe boundary. Includes
    parser support for pointer forms, allocation helpers, unsafe operations,
    runtime failure behavior, and backend lowering. Depends on steps 8 and 9
    because manual pointers interact with heap layout and runtime object
    boundaries.

12. **Re-evaluate asm().** Re-evaluate `asm()` only after the `manual()` model
    exists and only if a clear language-level justification remains.

### Phase 5 — Tooling Sync & Regression

> *After each accepted V2 slice lands.*

13. **Synchronize tooling.** After each accepted V2 compiler slice lands,
    synchronize duplicated language surfaces in the MCP server and VS Code
    extension so the tools do not advertise unsupported V2 syntax.

14. **Regression coverage.** Add regression coverage for each accepted V2 slice
    across tokenizer, parser, AST dump, semantic dump, type resolution, type
    checking, IR lowering, and runtime/backend behavior as needed.

---

## 7. Decisions

The following decisions have been explicitly made for the V2 design:

- **Philosophy** has shifted from simplicity-first toward Java/C compatibility and
  ease of pickup for Java 8 or C users.
- **Both primitive naming systems remain** because Java-style aliases are a
  transition aid.
- **Generics are runtime-reified** and use Java-style generic syntax including
  wildcard `<?>`.
- **The old struct idea is replaced by `arr<?>`**; there is no separate struct
  feature in the agreed V2 set.
- **`arr<?>` assumes compile-time layout calculation** for literals and fixed
  per-index shape after creation.
- **Unions remain only as tagged unions.**
- **Enums are removed** from the agreed V2 set.
- **Cryptographic verification for asm** has been removed.
- **`manual()` is intentionally unsafe** and currently tolerates runtime-fatal
  misuse instead of ownership/lifetime rules.
- **`_` is a discard expression.**
- **Import model:** plain import, module alias, wildcard import, selective import,
  explicit export, and compile-time ambiguity errors.
- **Implicit filename/directory fallback** for import conflicts is rejected.
- **Transitive wildcard imports** are deferred out of the initial import design.

---

## 8. Further Considerations

1. **Callback shorthand.** The one-statement callback shorthand should remain
   deferred unless its auto-lift rule is made completely explicit; otherwise it
   will look like immediate evaluation rather than deferred callback creation.

2. **GC references in manual memory.** Allowing GC references inside manual memory
   with only runtime-fatal protection remains the weakest semantic decision in the
   set. If this becomes a blocker during implementation, narrow `manual()` to
   plain scalars and raw addresses first.

3. **Incremental generics.** "Generics everywhere from day one" is still wider than
   the current implementation architecture wants. A practical execution approach
   may keep the syntax broad but stage actual support incrementally, starting with
   declarations and `arr<?>` containers.

4. **Future import candidates.** If the import model grows again later, the next
   candidates worth considering are duplicate/unused import diagnostics and
   optional re-export syntax — not transitive wildcard imports.

---

## 9. Relevant Files

| File | Purpose |
|------|---------|
| `version_wishlist.md` | This document — the authoritative V2 planning and spec document. |
| `compiler/calynda_v2.ebnf` | V2 grammar sandbox. Evolve in isolation before promoting changes into the canonical grammar. |
| `compiler/calynda.ebnf` | Canonical V1 grammar. Keep unchanged until a V2 slice is intentionally promoted. |
| `compiler/src/tokenizer.h` | Add tokens for new keywords and operators: `export`, `as`, `internal`, `static`, `++`, `--`, and primitive aliases. |
| `compiler/src/tokenizer.c` | Update keyword and operator recognition for V2 syntax. |
| `compiler/src/parser.c` | Extend grammar handling for imports/exports, aliases, selective/wildcard imports, generic parameters, `arr<?>` types, varargs, discard expression, internal nested helpers, and manual constructs. |
| `compiler/src/ast.h` | Add AST support for import/export forms, alias names, generic parameters, wildcard types, `arr<?>` metadata, tagged unions, discard expressions, internal visibility, static/private initialization, and manual constructs. |
| `compiler/src/symbol_table.c` | Implement name binding for imported/exported symbols, ambiguous import diagnostics, and internal nested-helper visibility rules. |
| `compiler/src/type_resolution.c` | Resolve aliases, generic arguments, wildcard forms, import/export-visible types, and restricted V2 declaration shapes. |
| `compiler/src/type_checker.c` | Enforce import ambiguity rules, internal visibility, varargs behavior, `++`/`--`, discard semantics, and later the generic/heterogeneous-array/manual semantics. |
| `compiler/src/hir.c` | Lower V2 sugar and preserve enough metadata for imports, generics, `arr<?>`, unions, and manual boundaries. |
| `compiler/src/mir.c` | Lower V2 runtime-visible operations including varargs packing, heterogeneous array access, tagged unions, and manual operations. |
| `compiler/src/runtime.h` | Extend runtime metadata for reified generics, heterogeneous arrays, and tagged unions. |
| `compiler/src/runtime.c` | Implement runtime helpers and fatal-error behavior for unsafe/manual misuse. |
| `compiler/src/bytecode.h` | Reflect any new runtime-visible metadata or opcodes needed by V2 features. |
| `compiler/src/bytecode.c` | Lower V2 runtime-heavy features into bytecode where supported. |
| `compiler/src/machine.c` | Handle native lowering for new operators, imports-visible calls, varargs packing, and runtime-backed V2 operations. |
| `compiler/tests/` | Add regression coverage per V2 slice: parser, type, runtime, and import tests. |
| `mcp-server/src/knowledge/grammar.ts` | Update embedded grammar only after a V2 grammar slice becomes real. |
| `mcp-server/src/knowledge/keywords.ts` | Mirror accepted V2 keywords and aliases. |
| `mcp-server/src/parser/lexer.ts` | Keep MCP lexing in sync with the compiler token surface. |
| `mcp-server/src/analyzer/types.ts` | Expand analyzer support for V2 types and import/export awareness. |
| `mcp-server/src/knowledge/examples.ts` | Add examples for the import system and accepted V2 features once semantics are frozen. |
| `vscode-calynda/syntaxes/calynda.tmLanguage.json` | Update syntax highlighting for import/export syntax, aliases, keywords, operators, and type aliases. |
| `README.md` | Clarify V1 versus V2 status once implementation begins. |
