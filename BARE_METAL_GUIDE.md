# Bare Metal Guide

This is a guide I made to help you write code in Bare Metal.

## What is supported in Bare Metal

This guide describes the current `boot` surface as the compiler actually
implements it today.

The most important rule is this:

- `boot` is a freestanding entry point.
- It does not use the hosted `start(string[] args)` runtime startup path.
- `boot` now ships with a small freestanding sequence runtime for homogeneous
	arrays and strings.
- Features outside that freestanding helper subset are still not bare-metal-safe
	by default unless your environment provides the needed symbols too.

## Entry Point Rules

You can do these things:

- Declare exactly one bare-metal entry point with `boot -> expr;` or
	`boot -> { ... };`.
- Return an `int32`-like halt code from `boot`, even though on real bare metal
	that value may just be ignored after control returns.

You cannot do these things:

- Give `boot` parameters.
- Declare both `boot` and `start` in the same compilation unit.
- Declare more than one `boot`.

## Definitely Supported

These features are part of the conservative bare-metal-safe subset and do not
depend on the hosted Calynda process runtime:

- Primitive scalar values such as integers, booleans, and chars.
- Scalar unary and binary operations.
- Scalar casts.
- Local variables and direct assignment to locals.
- Direct control flow such as `return`, branching, and loops encoded through
	normal expressions/statements.
- Direct calls to named top-level functions.
- Explicit recursion when it stays on that direct top-level call path.
- Inline assembly declarations and calling them directly.
- Bare-metal `_start` emission for `boot` programs.

`boot` also has built-in freestanding sequence support for these homogeneous
array and string operations:

- Array literals such as `[1, 2, 3]`.
- Array `.length`.
- Array indexing such as `values[i]`.
- Array index stores such as `values[i] = next`.
- `car(array)` and `cdr(array)`.
- String `.length`.
- String indexing such as `text[i]`.
- `car(string)` and `cdr(string)`.

In practice, if your code compiles down to normal arithmetic, local state,
direct global calls, explicit MMIO or inline assembly, and this small sequence
helper subset, it is a good bare-metal candidate.

## Freestanding Sequence Runtime

The compiler now ships a dedicated boot runtime archive:

- `calynda_runtime_boot.a`

That archive provides the helper surface needed for homogeneous arrays and
strings in `boot` code without depending on hosted process startup.

Important limits:

- It is intentionally small and only covers the sequence subset listed above.
- It uses a fixed freestanding arena for array literals and `cdr(...)` results.
- Strings still use Calynda string objects internally, but in bare metal they
	behave like read-only `char` sequences for indexing, `.length`, `car`, and
	`cdr`.
- If you link your own freestanding object set instead of using
	`calynda_runtime_boot.a`, you must provide compatible definitions for the same
	helper symbols.

## Special-Cased Support

There is one important freestanding special case in the compiler today:

- `import io.stdlib;`
- `stdlib.print();`
- `stdlib.print(value);`

This is the only imported member call that the compiler currently recognizes as
freestanding-safe enough to lower specially in `boot` code.

Important limits:

- Only `io.stdlib.print` is supported this way.
- It only supports 0 or 1 argument.
- This still depends on your freestanding runtime exporting the print helper
	symbols that the compiler lowers to.
- The argument expression itself must also be bare-metal-safe. If evaluating the
	argument pulls in extra runtime helpers, you are back in runtime-dependent
	territory.

## Rejected By The Compiler

These are rejected as part of the current freestanding `boot` contract:

- Unknown imported member calls such as `stdlib.missing()`.
- Imported member calls other than `io.stdlib.print`.
- `io.stdlib.print` with more than one argument.

## Not Bare-Metal-Safe By Default

These features still lower through helper calls or hosted object machinery that
the freestanding sequence runtime does not cover.

Treat them as unsupported by default in `boot` code.

- Template literals.
- General member access.
- Member stores.
- Non-scalar casts that require runtime checking.
- `throw`.
- Tagged unions and heterogeneous arrays.
- First-class closures.
- Calling through callable values instead of direct named functions.
- Type-query builtins such as `typeof`, `isint`, `isfloat`, `isbool`,
	`isstring`, `isarray`, and `issametype`.
- Threading and concurrency features such as `spawn`, `Thread`, `Future<T>`,
	`Mutex`, and `Atomic<T>`.
- Hosted runtime startup features such as `start(string[] args)` argument boxing
	and `calynda_rt_start_process`.

## Manual Memory Features

Manual memory is also not bare-metal-safe by default.

These operations lower to helper or C runtime calls rather than plain direct
machine instructions:

- `malloc`
- `calloc`
- `realloc`
- `free`
- `stackalloc`
- `cleanup(...)`
- `addr`
- `deref`
- `store`
- `offset`
- checked pointer variants such as `manual checked` and `ptr<T, checked>`

That means they only work in bare metal if your freestanding environment really
provides the required allocation and helper surface.

## Practical Writing Rules

If you want your `boot` code to be reliably portable across bare-metal targets,
stick to this style:

- Use primitive scalars and direct function calls.
- Use homogeneous arrays and strings freely when you stay inside `.length`,
	indexing, and `car`/`cdr`.
- Use inline assembly or MMIO wrappers for hardware access.
- Keep state in locals, globals, or hardware registers.
- Avoid any feature that feels object-like, runtime-backed, boxed, dynamic, or
	reflection-like.

Good bare-metal examples:

- `boot -> 42;`
- `boot -> { return uart_status(); };`
- `boot -> { uart_putc(72); uart_putc(73); return 0; };`
- `boot -> { int32[] values = [1, 2, 3]; return car(values) + cdr(values)[0]; };`
- `boot -> { string text = "hello"; return int32(car(text)) + int32(cdr(text).length); };`
- direct calls into `asm()` functions or hardware wrapper functions

Bad default bare-metal examples:

- `boot -> stdlib.print(text.length);`
- template literals such as ``hello ${value}``
- `boot -> object.member;`
- `boot -> typeof(value);`
- `boot -> spawn worker();`
- `boot -> malloc(64);`

## Conservative Rule Of Thumb

If the feature stays within primitive code, direct calls, inline assembly, and
the built-in homogeneous array/string sequence subset, it is a good bare-metal
fit.

If it needs helper support beyond `.length`, indexing, array literals, or
`car`/`cdr` on homogeneous arrays and strings, assume it is not safe in
bare-metal `boot` unless you have explicitly implemented that extra runtime
surface.
