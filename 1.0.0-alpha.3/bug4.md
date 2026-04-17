## Summary

If bare-metal code cannot directly consume `string` values, it still needs a practical byte-buffer alternative for UART output, ROM data, and protocol work.

Today, some array-shaped forms type-check, but several basic byte-buffer ergonomics still appear too limited for simple freestanding helpers.

## Why This Matters

In early bare-metal code, the fallback after `print(string msg)` fails is usually one of these:

- pass a byte array
- use a byte literal
- loop over `.length`
- write a tiny recursive helper with an index parameter

Those patterns should be straightforward. Right now, enough of them fail that even very small helper code becomes trial-and-error.

## What I Verified

### 1. Some parameter shapes are accepted

In the bare-metal probes I ran, these parameter types were accepted:

- primitive scalars
- `string`
- `string[]`
- `int8[]`

So array-like types are present at the type boundary.

### 2. Basic helper ergonomics still break down

The following patterns were observed to fail in the same freestanding workflow:

- `.length` on a local `int8[]`
- `char[]` literal-style approaches used as a byte source
- default parameters on function bindings used to simplify small recursive helpers

### 3. This blocks the obvious fallback path

If `string` byte access is not available, the next-best approach is to pass bytes explicitly. But that is only attractive if byte buffers have a reliable minimal feature set.

## Expected Behavior

At least one of these should be supported:

1. Fully supported `int8[]` byte-buffer handling in freestanding code, including `.length` and iteration.
2. A first-class `bytes` or equivalent bare-metal-friendly buffer type.
3. Reliable literal syntax for byte data that can be emitted without hosted runtime features.
4. Clear documentation of the minimal supported helper subset for arrays, bytes, and small utility functions in freestanding programs.

## Why This Feels Like a Feature Gap

Bare-metal developers need one honest low-level data path they can depend on.

If `string` is not that path, then byte buffers need to be. If byte buffers are also only partially usable, simple UART, parsing, and packet code all become harder than they should be.

## Environment

- Host: Debian/Linux x86_64
- Target: RISC-V 64 bare metal
- Emulator: `qemu-system-riscv64` (`virt`)
- Compiler flow: `calynda asm --target riscv64 ...` with custom freestanding link/load path