## Summary

The `asm` flow currently behaves like a single-source compiler path, even though practical Calynda projects quickly need more than one file.

For bare-metal work, that forces an external merge step before calling `calynda asm --target riscv64`, because local imports are not resolved natively by the assembly path.

## Why This Matters

As soon as a project has a separate UART helper, register map, or board support module, the single-file requirement becomes a workflow tax.

In this workspace, I had to add a custom pre-merge script that:

- walks local `import ...;` lines
- resolves them to files under `src/`
- inlines them into one generated `.cal`
- preserves only one package declaration

That is workable as a local hack, but it should not be necessary for normal multi-file bare-metal development.

## What I Verified

### 1. The current bare-metal assembly flow needs a merged source file

This workspace now builds by generating:

- `build/uart_hello.merged.cal`
- `build/uart_hello.generated.s`

The merge exists only to feed the compiler a single `.cal` file.

### 2. The workaround is build-system logic, not language logic

The project includes a custom script:

- `scripts/merge_calynda_sources.sh`

That script recursively resolves imports such as:

```cal
import lib.io.stdlib;
```

into files like:

```text
src/lib/io/stdlib.cal
```

and writes one synthetic source file for the compiler.

### 3. This is exactly the kind of behavior the compiler should own

Import resolution, package consistency, duplicate suppression, and source ordering are compiler concerns. They are fragile when pushed into ad hoc shell scripts.

## Expected Behavior

At least one of these should be supported:

1. `calynda asm` accepts a `.car` package input, just like the higher-level build flow.
2. `calynda asm` accepts multiple `.cal` files and resolves imports itself.
3. `calynda asm` accepts an entry file and follows the local import graph natively.
4. A dedicated freestanding package mode is documented and supported for multi-file bare-metal output.

## Why This Feels Like a Feature Gap

Multi-file organization is not an advanced use case. For bare-metal work it is the first step beyond a trivial demo.

Requiring users to invent their own source-merging preprocessor means:

- package handling is easy to get subtly wrong
- error locations become harder to trust
- duplicate-definition behavior becomes toolchain-specific
- every bare-metal workspace has to rediscover the same workaround

## Environment

- Host: Debian/Linux x86_64
- Target: RISC-V 64 bare metal
- Emulator: `qemu-system-riscv64` (`virt`)
- Compiler flow: `calynda asm --target riscv64 ...`
- Current workaround: external merge step before assembly generation