## Plan: Calynda V2 Agreed Feature Track

Define Calynda V2 as a compatibility-oriented evolution aimed at developers coming from Java 8 or C, while preserving V1 as the canonical shipped language until each V2 slice is parser-complete, semantically specified, and mirrored across compiler and tooling. The recommended delivery order is: first lock the written spec, then implement syntax/light semantic features, then tackle runtime-heavy features such as reified generics, heterogeneous arrays, tagged unions, and manual memory.

**Agreed V2 surface**
1. Philosophy: prioritize Java and C compatibility, and make the language easy to pick up for developers coming from Java 8 or C.
2. Primitive aliases: support both current Calynda primitive names and Java-style aliases.
3. Reified generics: Java-style generic syntax including `<T>`, `<K, V>`, and wildcard `<?>`.
4. Heterogeneous arrays: use `arr<?>` instead of the earlier struct concept; array literal layout is computed at compile time and each index has a fixed shape after creation.
5. Tagged unions: keep unions, but only as tagged unions.
6. Internal nested helpers: allow `internal` nested functions for loop-library extensibility, such as local `break` and `continue` helpers.
7. Manual memory: `manual()` is an unsafe boundary with C-style pointer/reference semantics and allocation helpers.
8. Static/private initialization: keep a V2 static/private top-level initialization model.
9. Prefix/postfix operators: add `++` and `--`.
10. Varargs: support Java/C-style variadic parameters.
11. Discard expression: `_` is a discard expression.
12. Import model: support plain imports, module aliases, wildcard imports, selective imports, explicit exports, and compile errors on ambiguous bare names.

**Deferred or rejected items**
1. Rejected: structs, enums, cryptographic verification for asm imports, and implicit filename/directory fallback for import conflicts.
2. Deferred: asm() remains out of the first implementation slice unless it solves a real problem that external adapters cannot solve.
3. Deferred pending explicit decision: the one-statement callback shorthand should not be implemented until its auto-lifting semantics are frozen.
4. Deferred: transitive wildcard imports are out of the first import design; wildcard imports should bind only directly exported names from the target module.

**Recommended import spec for V2**
1. Keep the existing direct import form: `import foo.bar;`
2. Add module aliases: `import foo.bar as baz;`
3. Add wildcard imports for direct exported names only: `import foo.bar.*;`
4. Add selective imports: `import foo.bar.{a, b, c};`
5. Add explicit exports on top-level declarations so importable names are intentional.
6. Make ambiguous bare names a compile error; require explicit qualification or aliasing instead of implicit filename/directory fallback.

**Steps**
1. Phase 0: Rewrite `/home/codewriter3000/Coding/calynda-lang/version_wishlist.md` into a proper V2 spec. Replace the old philosophy wording, remove rejected features, add the agreed import model, and split content into agreed, deferred, and rejected sections. This blocks all implementation work.
2. Phase 0: Keep `/home/codewriter3000/Coding/calynda-lang/compiler/calynda_v2.ebnf` as the only active V2 grammar sandbox. Do not update the canonical V1 grammar until a V2 slice is complete and validated.
3. Phase 0: Convert the import system from a single `import QualifiedName;` rule into a module system spec with direct import, alias import, wildcard import, selective import, and explicit export syntax. Freeze the grammar and name-resolution rules before coding.
4. Phase 1: Implement the low-risk front-end slice in the compiler: primitive aliases, `++`/`--`, varargs, `_` discard expression, and the revised static/private initialization surface. This depends on steps 1 and 2.
5. Phase 1: Implement the first V2 import/export slice in the compiler front end: parse `as`, `*`, and `{a, b, c}` import forms; add `export` for top-level declarations; and make ambiguous bare-name resolution a diagnostic. This depends on step 3 and can run in parallel with late testing for step 4.
6. Phase 2: Add internal nested-function visibility as a semantic rule rather than only syntax. The implementation should enforce that internal helper functions are callable only inside the intended nested callable contexts used by loop-library patterns. This depends on step 4.
7. Phase 2: Decide whether the one-statement callback shorthand survives. If it is accepted later, implement it as context-sensitive callable auto-lifting based on expected parameter type rather than ordinary expression parsing. This depends on step 4 and is intentionally not part of the first execution slice.
8. Phase 3: Design the runtime type model for reified generics. This must define AST representation, type resolution, checked-type metadata, wildcard handling, runtime descriptors, and backend-visible lowering rules before any generic implementation begins.
9. Phase 3: Implement `arr<?>` as the replacement for the old struct idea. Define exact rules for literal layout, indexing, mutation, casts, equality, dump rendering, backend lowering, and GC scanning. This depends on step 8.
10. Phase 3: Implement tagged unions on top of the same runtime metadata model used by reified generics and heterogeneous arrays, so Calynda does not end up with multiple incompatible tagged-layout systems. This depends on steps 8 and 9.
11. Phase 4: Add `manual()` as an explicitly unsafe boundary. This includes parser support for pointer forms, allocation helpers, unsafe operations, runtime failure behavior, and backend lowering. This depends on steps 8 and 9 because manual pointers interact with heap layout and runtime object boundaries.
12. Phase 4: Re-evaluate `asm()` only after the `manual()` model exists and only if a clear language-level justification remains.
13. Phase 5: After each accepted V2 compiler slice lands, synchronize duplicated language surfaces in the MCP server and VS Code extension so the tools do not advertise unsupported V2 syntax.
14. Phase 5: Add regression coverage for each accepted V2 slice across tokenizer, parser, AST dump, semantic dump, type resolution, type checking, IR lowering, and runtime/backend behavior as needed.

**Relevant files**
- `/home/codewriter3000/Coding/calynda-lang/version_wishlist.md` — turn the current wishlist into the authoritative V2 planning/spec document.
- `/home/codewriter3000/Coding/calynda-lang/compiler/calynda_v2.ebnf` — evolve the V2 grammar in isolation before promoting any change into the canonical grammar.
- `/home/codewriter3000/Coding/calynda-lang/compiler/calynda.ebnf` — keep unchanged until a V2 slice is intentionally promoted.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/tokenizer.h` — add tokens for new keywords and operators such as `export`, `as`, `internal`, `static`, `++`, `--`, and any primitive aliases.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/tokenizer.c` — update keyword and operator recognition for V2 syntax.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/parser.c` — extend grammar handling for imports/exports, aliases, selective imports, wildcard imports, generic parameters, arr<?> types, varargs, discard expression, internal nested helpers, and manual constructs.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/ast.h` — add AST support for import/export forms, alias names, generic parameters, wildcard types, arr<?> metadata, tagged unions, discard expressions, internal visibility, static/private initialization, and manual constructs.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/symbol_table.c` — implement name binding for imported/exported symbols, ambiguous import diagnostics, and internal nested-helper visibility rules.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/type_resolution.c` — resolve aliases, generic arguments, wildcard forms, import/export-visible types, and restricted V2 declaration shapes.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/type_checker.c` — enforce import ambiguity rules, internal visibility, varargs behavior, `++`/`--`, discard semantics, and later the generic/heterogeneous-array/manual semantics.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/hir.c` — lower V2 sugar and preserve enough metadata for imports, generics, arr<?>, unions, and manual boundaries.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/mir.c` — lower V2 runtime-visible operations including varargs packing, heterogeneous array access, tagged unions, and manual operations.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/runtime.h` — extend runtime metadata for reified generics, heterogeneous arrays, and tagged unions.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/runtime.c` — implement runtime helpers and fatal-error behavior for unsafe/manual misuse.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/bytecode.h` — reflect any new runtime-visible metadata or opcodes needed by V2 features.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/bytecode.c` — lower V2 runtime-heavy features into bytecode where supported.
- `/home/codewriter3000/Coding/calynda-lang/compiler/src/machine.c` — handle native lowering for new operators, imports-visible calls, varargs packing, and runtime-backed V2 operations.
- `/home/codewriter3000/Coding/calynda-lang/compiler/tests` — add regression coverage per V2 slice, especially parser/type/runtime/import tests.
- `/home/codewriter3000/Coding/calynda-lang/mcp-server/src/knowledge/grammar.ts` — update the embedded grammar only after a V2 grammar slice becomes real.
- `/home/codewriter3000/Coding/calynda-lang/mcp-server/src/knowledge/keywords.ts` — mirror accepted V2 keywords and aliases.
- `/home/codewriter3000/Coding/calynda-lang/mcp-server/src/parser/lexer.ts` — keep MCP lexing in sync with the compiler token surface.
- `/home/codewriter3000/Coding/calynda-lang/mcp-server/src/analyzer/types.ts` — expand analyzer support for V2 types and import/export awareness.
- `/home/codewriter3000/Coding/calynda-lang/mcp-server/src/knowledge/examples.ts` — add examples for the import system and accepted V2 features once semantics are frozen.
- `/home/codewriter3000/Coding/calynda-lang/vscode-calynda/syntaxes/calynda.tmLanguage.json` — update syntax highlighting for import/export syntax, aliases, keywords, operators, and type aliases.
- `/home/codewriter3000/Coding/calynda-lang/README.md` — clarify V1 versus V2 status once implementation begins.

**Verification**
1. Add parser golden tests for direct imports, aliased imports, wildcard imports, selective imports, explicit exports, ambiguous import diagnostics, primitive aliases, `++`/`--`, varargs, `_`, internal nested helpers, generic parameter lists, wildcard `<?>`, tagged unions, and manual statements.
2. Add semantic tests for invalid alias use, ambiguous imported names, invalid export forms, invalid internal calls, wildcard misuse, heterogeneous array index writes after shape lock, union tag mismatches, and manual misuse.
3. Validate that bytecode and native backends both either support each new runtime-visible feature or reject it with explicit diagnostics.
4. Synchronize MCP and VS Code only after compiler support lands, then verify grammar resources, keyword sets, analyzer type names, examples, and syntax highlighting all reflect the same implemented V2 slice.

**Decisions**
- Philosophy has shifted from simplicity-first toward Java/C compatibility and ease of pickup for Java 8 or C users.
- Both primitive naming systems remain because Java-style aliases are a transition aid.
- Generics are runtime-reified and use Java-style generic syntax including wildcard `<?>`.
- The old structure idea is replaced by `arr<?>`; there is no separate struct feature in the agreed V2 set.
- `arr<?>` currently assumes compile-time layout calculation for literals and fixed per-index shape after creation.
- Unions remain in scope only as tagged unions.
- Enums are removed from the agreed V2 set.
- Cryptographic verification for asm has been removed.
- `manual()` is intentionally unsafe and currently tolerates runtime-fatal misuse instead of ownership/lifetime rules.
- `_` is a discard expression.
- The agreed import model is: plain import, module alias, wildcard import, selective import, explicit export, and compile-time ambiguity errors.
- Implicit filename/directory fallback for import conflicts is rejected.
- Transitive wildcard imports are deferred out of the initial import design.

**Further considerations**
1. The one-statement callback shorthand should remain deferred unless its auto-lift rule is made completely explicit; otherwise it will look like immediate evaluation rather than deferred callback creation.
2. Allowing GC references inside manual memory with only runtime-fatal protection remains the weakest semantic decision in the set. If that becomes a blocker during implementation, narrow `manual()` to plain scalars and raw addresses first.
3. “Generics everywhere from day one” is still wider than the current implementation architecture wants. A practical execution agent may keep the syntax broad but stage actual support incrementally, starting with declarations and `arr<?>` containers.
4. If the import model grows again later, the next candidates worth considering are duplicate/unused import diagnostics and optional re-export syntax, not transitive wildcard imports.
