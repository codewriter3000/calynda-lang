## Summary

Freestanding code should be able to call imported helpers without pulling in hosted package/member runtime machinery.

Right now, there is a practical gap between:

- direct top-level helper calls in merged source, which can be made to work
- package-style imported calls, which pull in hosted runtime symbols and break a bare-metal link

## Why This Matters

Bare-metal code still needs modular structure. A small `stdlib` or board-support helper should not force the program onto the hosted runtime path.

In this workspace, the natural shape:

```cal
import lib.io.stdlib;

boot() -> {
    stdlib.print();
    return 0;
};
```

was not usable in the freestanding flow because package-member lowering introduced hosted runtime dependencies.

## What I Verified

### 1. Direct helper usage can be made to work in a freestanding build

With local files merged into one generated source file, simple direct calls such as:

```cal
print();
```

can be assembled, linked, and run in the custom bare-metal flow.

### 2. Package-member access pulls in hosted runtime symbols

When helper access followed the package-member shape, the resulting object code referenced hosted runtime pieces such as:

- `__calynda_pkg_stdlib`
- `__calynda_rt_member_load`
- `__calynda_rt_call_callable`

Those are not part of a minimal freestanding runtime surface.

### 3. The result is an artificial boundary in module design

The code can be organized across files, but only if the workspace flattens those files and avoids the more natural package-member call surface.

## Expected Behavior

At least one of these should be supported:

1. Freestanding imported calls lower statically when the callee is known at compile time.
2. Package-member access for local modules works without hosted runtime dispatch.
3. A dedicated `--freestanding` or equivalent mode explicitly disables hosted package/member lowering and uses direct symbol resolution instead.
4. The compiler documents which module patterns are supported in freestanding code and which still require the hosted runtime.

## Why This Feels Like a Feature Gap

The current behavior makes modular bare-metal code possible only through source flattening and careful avoidance of some otherwise natural call forms.

That means users have to understand backend lowering details just to structure a tiny UART helper. For early systems code, that is too much hidden policy in the compiler/runtime boundary.

## Environment

- Host: Debian/Linux x86_64
- Target: RISC-V 64 bare metal
- Emulator: `qemu-system-riscv64` (`virt`)
- Compiler flow: `calynda asm --target riscv64 ...` plus custom freestanding link
- Hosted symbols observed from package-member style helper access