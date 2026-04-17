I’m drafting the focused issue now around the exact bare-metal boundary we just verified: `string` parameters compile, but freestanding code cannot legally access their bytes, so a real `print(string)` helper cannot be implemented in user code.



Use this as the issue body:

```md
## Summary

In bare-metal Calynda, a function can accept a `string` parameter, but freestanding user code does not appear to have a supported way to access that string's bytes.

This means a natural helper like `print(string msg)` can type-check at the function boundary, but cannot actually emit `msg` to UART from user code.

## Why This Matters

For bare-metal targets, a basic UART print helper is one of the first things you want to write.

Right now, the obvious shape:

```cal
int32 print = (string msg) -> { ... };
```

is not practically implementable in freestanding user code because the current surface does not allow me to legally walk the string contents.

## What I Verified

### 1. `string` parameters themselves compile

A probe like this compiles to RISC-V assembly:

```cal
boot() -> {
    return use("hi");
};

int32 use = (string value) -> 0;
```

So `string` is accepted as a parameter type in the bare-metal `boot()` flow.

### 2. Strings cannot be indexed in user code

This probe fails:

```cal
boot() -> {
    return use("hi");
};

int32 use = (string value) -> value[0];
```

Diagnostic:

```text
Cannot index expression of type string.
```

### 3. Taking the raw address of a string also fails

I tested whether freestanding code could treat a string as a runtime object and walk its bytes via a layout:

```cal
layout RtString { int64 header; int64 length; int64 bytes; };

int32 print = (string msg) -> {
    manual {
        ptr<RtString> s = addr(msg);
        _ = s;
    };
    return 0;
};

boot() -> print("hi");
```

This fails with:

```text
Memory operation argument must have an integral type but got string.
```

So `addr(msg)` is not accepted for `string` values in this context.

## Resulting Limitation

The language currently allows this shape:

```cal
int32 print = (string msg) -> 0;
```

but does not provide a usable freestanding way to do the next step:

- get the string length
- get a pointer to the string bytes
- iterate those bytes
- store them to UART

That makes `string` parameters much less useful for bare-metal code than they appear.

## Expected Behavior

At least one of these should be supported:

1. A safe, supported way to read string bytes and length from freestanding user code.
2. A built-in string iteration/indexing surface that works in bare-metal code.
3. A documented freestanding runtime helper for string byte access.
4. Clear documentation that `string` parameters are accepted but string contents are not currently accessible from bare-metal user code.

## Why This Feels Like a Bug / Gap

From the user perspective, accepting `string` as a parameter strongly suggests that simple helpers like `print(string msg)` should be implementable.

But in practice, the current freestanding surface seems to stop at parameter acceptance and provides no supported path to actually consume the string contents.

That is a surprising and important limitation for early bare-metal development.

## Environment

- Host: Debian/Linux x86_64
- Target: RISC-V 64 bare metal
- Emulator: `qemu-system-riscv64` (`virt`)
- Compiler: current source build of Calynda
- Flow: `calynda asm --target riscv64 ...` with custom freestanding link/load path
```

Suggested title:

```md
Bare-metal code can accept `string` parameters but cannot access string bytes in user code
```

A shorter variant if you want it more maintainer-direct:

```md
`string` parameters compile in `boot()` code, but freestanding user code cannot index strings or take `addr(string)` to read bytes
```