## Summary

For bare-metal code, `string` should be usable as a `char[]`-style byte sequence, or at least have a compiler-supported coercion to one.

Right now, `string` is accepted as a parameter type, but it is not practically usable as the low-level character buffer that early freestanding code expects.

## Why This Matters

In bare-metal programs, `string` is most useful when it can behave like a read-only character array.

That would make helpers like these straightforward:

```cal
int32 print = (string msg) -> {
    for each ch in msg { ... }
    return 0;
};
```

or, if Calynda wants to keep `string` distinct at the type level:

```cal
int32 print = (string msg) -> {
    char[] bytes = msg;
    ...
    return 0;
};
```

Without that, the type looks convenient at the function boundary but is not actually useful for UART output, parsing, or ROM-backed text.

## What I Verified

### 1. `string` parameters compile in bare-metal code

This shape is accepted:

```cal
int32 use = (string value) -> 0;
```

### 2. The string contents are not directly usable

In the same bare-metal workflow:

- `value[0]` failed because `string` was not indexable
- `addr(value)` failed because `string` was not accepted by the memory operation surface

### 3. The result is an awkward type boundary

`string` currently behaves like a valid parameter type without also behaving like a usable freestanding text container.

## Expected Behavior

At least one of these should be supported in bare-metal code:

1. `string` is treated as a read-only alias for `char[]`.
2. `string` can be implicitly coerced to `char[]` in freestanding code.
3. `string` exposes a built-in `.chars`, `.bytes`, or equivalent array-like view.
4. `string` becomes indexable and iterable in the bare-metal subset.

## Why This Should Be Separate From Bug 4

The broader byte-buffer issue is about having one reliable low-level data path.

This proposal is narrower and more ergonomic: if `string` already exists in the language and literals already produce it naturally, then bare-metal code should be able to treat that value as character data without having to re-express it through another container type first.

That is a distinct language design choice, not just a general array ergonomics problem.

## Environment

- Host: Debian/Linux x86_64
- Target: RISC-V 64 bare metal
- Emulator: `qemu-system-riscv64` (`virt`)
- Compiler flow: `calynda asm --target riscv64 ...` with custom freestanding link/load path