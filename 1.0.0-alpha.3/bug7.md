## Summary

Add a shorthand syntax for functions whose entire body is manual code.

Today, a function that is completely manual must be written like this:

```cal
int32 print = (string msg) -> {
    manual {
        ...
    };
};
```

This proposal adds a shorter equivalent form:

```cal
int32 print = manual(string msg) -> {
    ...
};
```

## Why This Matters

For bare-metal code, some helpers are entirely manual by nature:

- UART/MMIO helpers
- register access helpers
- pointer-based memory utilities
- low-level boot and board-support routines

In those cases, the current syntax repeats the same intent twice:

1. this function is conceptually low-level and manual
2. the whole function body is wrapped in `manual`

That redundancy makes the common bare-metal case more verbose than it needs to be.

## Current Example

The current UART helper shape in this workspace is:

```cal
int32 print = (string msg) -> {
    manual {
        ptr<int8> uart = 0x10000000;
        ...
    };

    return 0;
};
```

When the whole function is manual, the wrapper block adds little semantic value.

## Proposed Syntax

```cal
int32 print = manual(string msg) -> {
    ptr<int8> uart = 0x10000000;
    ...
    return 0;
};
```

This should mean the same thing as the fully wrapped version: the function body is parsed and checked in manual mode.

## Expected Behavior

At least one of these designs would work:

1. `manual(args) -> { ... }` marks the entire function body as manual.
2. `manual` is allowed as a modifier before the parameter list for lambda-style bindings.
3. A similar function-level modifier exists for named functions and bindings consistently.

The key requirement is that fully manual functions no longer need a nested `manual {}` block.

## Why This Should Be A Separate Request

This is not a runtime capability gap. It is a syntax and ergonomics improvement.

The existing bare-metal requests are mostly about what code can do. This one is about making an already-valid low-level pattern less noisy and easier to read.

## Design Notes

Useful properties of this feature would be:

1. exact semantic equivalence to wrapping the whole body in `manual {}`
2. no mixed-mode ambiguity inside the function body
3. clear parser behavior for bindings, lambdas, and any named-function surface Calynda supports

If the language wants to preserve explicit block-level `manual`, both forms could coexist:

```cal
int32 print = manual(string msg) -> {
    ...
};

int32 mixed = (int32 mode) -> {
    if (mode != 0) {
        manual {
            ...
        };
    }
    return 0;
};
```

## Environment

- Host: Debian/Linux x86_64
- Target: RISC-V 64 bare metal
- Emulator: `qemu-system-riscv64` (`virt`)
- Compiler flow: `calynda asm --target riscv64 ...` with custom freestanding link/load path