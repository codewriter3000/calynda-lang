# Calynda Warnings And Advisories

This document describes every warning and advisory currently emitted by the compiler's type-checking diagnostics.

## Diagnostic Classes

- Warning: enabled by default unless the same condition is promoted to an error by a stricter mode.
- Performance warning: emitted only when performance warnings are enabled with `type_checker_set_global_performance_warnings(true)`.
- Performance advisory: emitted only when performance advisories are enabled with `type_checker_set_global_performance_advisories(true)`.

## Warning: Possible Data Race On `spawn`

Message family:

```text
Possible data race: spawned callable captures mutable symbol '<name>'. Use final, thread_local, or Atomic<T> to share it safely.
```

When it appears:

- A spawned callable captures a mutable symbol that is not marked `final`, not `thread_local`, and not wrapped in `Atomic<T>`.

Why it is warned against:

- The spawned callable may execute concurrently with the enclosing code.
- A mutable shared capture creates a credible race on reads or writes.
- Races are correctness bugs first, but they also make performance less predictable because synchronization is implicit or missing.

Preferred alternatives:

- Mark immutable shared state as `final`.
- Use `thread_local` when the value should not be shared.
- Use `Atomic<T>` when shared mutation is intentional.

Notes:

- When strict race checking is enabled with `type_checker_set_global_strict_race_check(true)`, this condition is upgraded from a warning to an error.

## Performance Warning: Dynamic Callable Dispatch Through `external`

Message family:

```text
Dynamic callable dispatch through external-typed value ... cannot use a direct call and will route through runtime helper dispatch. Prefer a statically typed callable when performance matters.
```

When it appears:

- A call goes through a `var` or other `external`-typed value instead of a statically typed callable.

Why it is warned against:

- The compiler cannot lower the call as a direct, typed call.
- Dispatch must go through runtime helper machinery.
- That blocks straightforward inlining and other call-site optimizations.
- On bare-metal and size-sensitive targets, helper-based dispatch adds avoidable code and runtime overhead.

Preferred alternatives:

- Use a statically typed callable parameter or binding.
- Narrow `external` values to a concrete callable type before invoking them.

## Performance Advisory: Template Literals

Message family:

```text
Complex template literals may still build strings through runtime helpers and may allocate. Prefer plain strings or simple `${value}` forms in hot code.

Template literals in boot, manual, or size-focused code build strings through runtime helpers and may allocate. Prefer plain strings, explicit casts, or cached values on this path.
```

When it appears:

- A complex hosted template literal such as `` `hello ${name}` `` is type-checked while performance advisories are enabled.
- An interpolated template literal appears in `boot`, inside a `manual` block, or while `--size-focus` is active.

Why it is advised against:

- Template literals are convenient, but they are not free.
- Complex hosted templates and strong-context templates still lower through runtime string-building helpers.
- Interpolation may allocate or copy data.
- In hot code, tight loops, or bare-metal/size-focused builds, that cost is often worse than reusing an existing string or avoiding dynamic formatting entirely.

Preferred alternatives:

- Reuse cached strings.
- Keep already-plain string values plain instead of rebuilding them.
- Prefer simple `` `${value}` `` forms on ordinary hosted paths when you still want interpolation syntax.
- Use plain strings or explicit casts in `boot`, `manual`, or size-focused code when predictability or code size matters most.

## Performance Advisory: Omitted Array Extents That Stay Runtime-Derived

Message family:

```text
... omits an array length that cannot be inferred statically; length will be determined at runtime. Prefer an explicit extent or a statically sized source when size-focused optimizations matter.
```

This advisory can now appear at three kinds of boundaries:

- Binding declarations such as `int32[] values = some_expression;`
- Declared return types such as `int32[] make_values = () -> ...;`
- Parameter default values such as `(int32[] values = some_expression) -> ...`

When it appears:

- The declared type omits at least one array extent.
- The compiler tries to fill omitted extents from the source expression.
- After that merge, at least one omitted extent still has no static size.

Why it is advised against:

- The array length remains a runtime property instead of a compile-time fact.
- The compiler loses opportunities to preserve tighter shape information in later phases.
- Size-focused optimizations become harder, especially for native and bare-metal builds where explicit shapes can help dead-code pruning and tighter lowering choices.
- The code is still valid, but it carries less optimization information than an explicit or statically inferable extent.

Preferred alternatives:

- Write the extent explicitly when it is part of the API or storage contract.
- Feed the declaration, return, or default from a statically sized source when possible.
- Avoid shape-erasing callable boundaries when exact lengths matter for code size or downstream optimization.

Important detail:

- If the omitted extent can be inferred statically, the compiler stays silent. The advisory exists only for cases that remain runtime-derived after type checking.

## Summary

As of this release, the compiler emits one general warning family, one performance warning family, and two performance advisory families. This file should be updated whenever a new warning or advisory is added, removed, promoted to an error, or materially reworded.