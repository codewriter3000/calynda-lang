## Summary

Add built-in `car()` and `cdr()` operations for arrays so small recursive and functional-style helpers are practical in Calynda.

For bare-metal code in particular, these would provide a compact way to consume byte buffers and other array inputs without depending on more elaborate slicing or iterator features.

## Why This Matters

Many small systems helpers naturally want one of these shapes:

- take the first element
- take the remaining tail
- recurse on the tail until empty

That is especially useful for:

- UART output helpers
- byte-buffer walkers
- packet and protocol parsing
- simple ROM-backed table processing

If Calynda wants to support a more Lisp-flavored utility surface, `car()` and `cdr()` would map cleanly onto arrays for those use cases.

## Proposed Behavior

For an array input `xs`:

- `car(xs)` returns the first element of the array
- `cdr(xs)` returns the tail of the array with the first element removed

For example:

```cal
int32 first = car(values);
int8[] rest = cdr(bytes);
```

If empty-array behavior is considered too implicit, then one of these should be chosen and documented:

1. trap or compile-time rejection when emptiness is provably unsafe
2. require the caller to guard with a length/empty check first
3. provide nullable or option-style variants

## Why This Helps The Current Bare-Metal Workflow

Right now, writing tiny recursive helpers is harder than it should be because several convenient array-adjacent patterns are already limited in freestanding code.

Built-in `car()` and `cdr()` would give the language one simple, explicit way to decompose arrays without requiring users to depend on richer slicing or indexing support first.

That would make helpers like byte-at-a-time UART output much easier to express.

## Design Notes

The useful scope would be:

1. arrays only, not arbitrary containers
2. generic over element type
3. available in bare-metal code without hosted runtime requirements

If `cdr()` returning a copied tail is too expensive, it could instead lower to a slice/view representation internally, as long as the surface stays simple.

## Environment

- Host: Debian/Linux x86_64
- Target: RISC-V 64 bare metal
- Emulator: `qemu-system-riscv64` (`virt`)
- Compiler flow: `calynda asm --target riscv64 ...` with custom freestanding link/load path