# Top-level lambda-style function bindings are rejected as circular definitions instead of supporting recursion

## Summary

Top-level function-style bindings are currently rejected as circular definitions when they refer to themselves, instead of being treated as recursive functions.

For example, this shape:

```cal
int32 print = manual(string msg) -> {
    ptr<int8> uart = 0x10000000;
    store(uart, msg[0]);
    return msg.length == 0 ? 0 : print(cdr(msg));
};
```

fails with:

```text
Circular definition involving 'print'.
```

## Why This Is Surprising

At the language surface, this looks like a normal named function definition using the existing binding syntax for lambdas.

A user would reasonably expect `print(...)` inside that body to be treated as a recursive function call.

Instead, the compiler treats it as a circular binding initializer.

## Current Compiler Behavior

From the current grammar and type checker, this appears to happen because:

1. `int32 print = ...;` is parsed as a normal `BindingDecl`, not as a distinct named recursive function declaration.
2. The type checker resolves the top-level binding symbol and marks it as "currently resolving".
3. While checking the initializer expression, any reference to `print` re-enters symbol resolution for the same binding.
4. The resolver sees that the symbol is already in progress and emits the circular-definition diagnostic.

Relevant code paths:

- `BindingDecl` grammar: [compiler/calynda.ebnf](compiler/calynda.ebnf)
- circular-definition guard: [compiler/src/sema/type_checker/type_checker_resolve_binding.c](compiler/src/sema/type_checker/type_checker_resolve_binding.c)
- identifier resolution path: [compiler/src/sema/type_checker/expr/type_checker_expr.c](compiler/src/sema/type_checker/expr/type_checker_expr.c)

The key guard is effectively:

```c
if (entry->is_resolving) {
    tc_set_error_at(checker, symbol->declaration_span, NULL,
                    "Circular definition involving '%s'.",
                    symbol->name ? symbol->name : "<anonymous>");
    return NULL;
}
```

## Why This Matters

This makes straightforward recursive helper definitions impossible in the most natural function-style binding form.

That is especially noticeable in bare-metal code, where simple recursive helpers are a practical fallback when loop syntax or richer collection helpers are limited.

Examples include:

- printing a string one character at a time
- walking arrays recursively
- small parser or buffer helpers
- basic functional utilities like recursive transforms

## Expected Behavior

At least one of these should be supported:

1. Top-level lambda-style bindings should be allowed to refer to themselves recursively.
2. The language should provide a distinct named-function declaration form with recursive semantics.
3. If recursion is intentionally unsupported for binding initializers, that should be documented explicitly and diagnosed with a more precise error than "circular definition".

## Suggested Direction

The most user-friendly behavior would be:

- treat a top-level binding whose initializer is a lambda as a recursive callable binding within its own body

or, if that is not desirable:

- introduce a separate named recursive function form and keep ordinary bindings non-recursive

## Environment

- Host: Debian/Linux x86_64
- Compiler: Calynda 1.0.0-alpha.3
- Target used while hitting this: RISC-V 64 bare-metal asm flow