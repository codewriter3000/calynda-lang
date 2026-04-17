<p align="center">
    <img src="calynda-icon.svg" width="350" alt="Calynda logo">
</p>

<p align="center">
  <a href="https://github.com/codewriter3000/calynda-lang/actions">
    <img src="https://github.com/codewriter3000/calynda-lang/workflows/CI/badge.svg" alt="GitHub Build Status">
  </a>
</p>

---

Calynda is a compiled functional systems programming language. The current 1.0.0-alpha.2 surface targets Linux on x86_64, AArch64, and RISC-V 64, and can also emit portable bytecode. The compiler is written in C and produces native executables directly from `.cal` source files.

## Repository Layout

This is a monorepo. The packages are:

| Package | Description |
|---|---|
| [`compiler/`](compiler/) | C compiler, runtime, and CLI |
| [`mcp-server/`](mcp-server/) | MCP server for AI assistant integration |
| [`vscode-calynda/`](vscode-calynda/) | VS Code syntax highlighting extension |

## Compiler Pipeline

The compiler lowers source through the following stages:

- Recursive-descent parsing with source-aware diagnostics
- Semantic analysis: symbol resolution and type checking
- Lowering: AST → HIR → MIR → LIR → codegen plan → machine program → assembly

Two backend paths are supported:

- **Native**: emits x86_64, AArch64, or RISC-V 64 assembly and links a native executable
- **Bytecode**: emits `portable-v1` bytecode text

There is no interpreter path. AST or IR interpretation is not part of the design.

## Language Features

- First-class lambdas with arrow syntax
- Homogeneous typed arrays and heterogeneous `arr<?>` arrays
- Tagged unions with constructor syntax and read-only `.tag` / `.payload` access
- Layout declarations for describing typed-pointer memory structures
- Unsafe manual memory management via `manual { ... };` and `manual checked { ... };`
- Typed pointers (`ptr<T>`) with `deref`, `store`, `offset`, and `addr` operations
- Deferred cleanup via `cleanup(value, fn)`, which runs `fn(value)` when the enclosing manual scope exits — including on `return` and `throw` paths
- Scope-local scratch storage via `stackalloc(size)`, reclaimed at manual-scope exit
- `manual checked` keeps a growable runtime bounds registry for safer pointer tracking
- Threading via `spawn`, `Thread`, `Future<T>`, and `Mutex`
- `Thread.join()` / `Thread.cancel()` and `Future<T>.get()` / `Future<T>.cancel()`
- `Atomic<T>` cells with `Atomic<T>.new(value)`, `.load()`, `.store(value)`, and `.exchange(value)` for first-class single-word runtime values
- `thread_local` storage for cross-thread identity (not ordinary stack locals)
- Inline assembly declarations via `asm()`
- Bare-metal entry point via `boot()` for freestanding environments without weakening the bare-metal contract to Linux-only exit behavior
- Template literals with string interpolation
- Ternary expressions, member access, index access, and casts
- CAR source archives for bundling `.cal` files
- `throw` for error propagation

The standard entry point is `start(string[] args)`. Bare-metal programs use `boot()` instead.

For concurrency, `spawn` of a zero-argument `void` callable returns `Thread`; `spawn` of a zero-argument non-`void` callable returns `Future<T>`.

For heterogeneous arrays, indexed reads produce `<external>` values, indexed writes are rejected, and equality on extracted values requires an explicit cast.

Layout field types are restricted to scalar primitive types.

More detail is in [RELEASE_NOTES.md](RELEASE_NOTES.md).

## Installation

For a normal user install:

```sh
sh ./compiler/install.sh
```

For a custom prefix:

```sh
sh ./compiler/install.sh --prefix /usr/local
```

The installer builds Calynda and installs:

- a `calynda` launcher in your bin directory
- the real CLI binary under `lib/calynda/`
- the required `calynda_runtime.a` archive beside that binary so generated executables can link correctly

If your install bin directory is not on `PATH`, the installer prints the export line to add to your shell profile.

## Build From Source

Requirements:

- `gcc`
- `make`
- standard Unix userland tools used by the build and install scripts

To build the CLI locally:

```sh
make -C compiler calynda
```

The resulting compiler binary is:

```text
compiler/build/calynda
```

## Quick Start

Create a file named `hello.cal`:

```cal
import io.stdlib;

start(string[] args) -> {
    var render = (string name) -> `hello ${name}`;
    stdlib.print(render(args[0]));
    return 0;
};
```

Build it:

```sh
calynda build hello.cal
```

Run it:

```sh
./build/hello world
```

Or compile and run in one step:

```sh
calynda run hello.cal world
```

## CLI

The current CLI commands are:

```text
calynda --version
calynda build [--strict-race-check] [--target T] <source> [-o output]
calynda run [--strict-race-check] [--target T] <source> [args...]
calynda pack <directory> [-o output.car]
calynda asm [--strict-race-check] [--target T] <source.cal>
calynda bytecode <source.cal>
calynda help
```

What they do:

- `--version`: print the CLI version metadata
- `build`: compile a `.cal` or `.car` source input to a native executable, optionally with the strict race checker enabled
- `run`: build a temporary native executable and execute it
- `pack`: bundle a directory of `.cal` files into a `.car` archive
- `asm`: emit native assembly to stdout (x86_64 by default, or `--target aarch64-linux` / `--target riscv64-linux`)
- `bytecode`: emit `portable-v1` bytecode text to stdout

`--strict-race-check` is the exact alpha.2 flag spelling for the stricter shared-state race checker.

## Grammar

The grammar lives in [compiler/calynda.ebnf](compiler/calynda.ebnf).

## Backend Model

- **Native**: AST → HIR → MIR → LIR → CodegenPlan → MachineProgram → assembly → linked executable
- **Bytecode**: AST → HIR → MIR → BytecodeProgram `portable-v1`

The backend direction is described in [backend_strategy.md](backend_strategy.md), and the bytecode format is described in [bytecode_isa.md](bytecode_isa.md).

## Development

Run the full test suite:

```sh
./compiler/run_tests.sh
```

Or via the root Makefile:

```sh
make test
```

Useful compiler build targets:

```sh
make -C compiler calynda
make -C compiler build_native
make -C compiler emit_asm
make -C compiler emit_bytecode
make -C compiler test
make -C compiler clean
```

The local VS Code syntax extension lives under [vscode-calynda/README.md](vscode-calynda/README.md).

## MCP Server

An [MCP (Model Context Protocol)](mcp-server/README.md) server is included that enables AI assistants to deeply understand and work with Calynda. It provides code analysis, type validation, syntax explanation, code completion, examples, and formatting tools.

See [mcp-server/README.md](mcp-server/README.md) for installation and configuration instructions.

## Current Limits

- Native compilation currently targets Linux x86_64 SysV ELF, with cross-compilation support for AArch64 and RISC-V 64.
- Final native linking currently uses `gcc`.
- `Atomic<T>` is currently limited to first-class single-word runtime values.
- Forced cancellation uses pthread cancellation semantics; `Thread.cancel()` / `Future<T>.cancel()` do not yet guarantee Calynda-level cleanup execution for `manual` or `manual checked` scopes.
- `thread_local` is currently limited to storage with cross-thread identity, not arbitrary stack locals.
- The runtime surface is deliberately small and only covers the helpers the compiler lowers to today.
- The bytecode backend emits a portable compiled form, but this repository does not yet ship a bytecode runner.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
