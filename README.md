# Calynda

Calynda is a compiled functional systems programming language.

The first public edition ships an end-to-end compiler pipeline, a native Linux x86_64 backend, a portable bytecode emitter, a runtime layer for dynamic language features, and a command-line tool that can build and run `.cal` programs.

## Repository Layout

This is a monorepo. The packages are:

| Package | Description |
|---|---|
| [`compiler/`](compiler/) | C compiler, runtime, and CLI |
| [`mcp-server/`](mcp-server/) | MCP server for AI assistant integration |
| [`vscode-calynda/`](vscode-calynda/) | VS Code syntax highlighting extension |

## Status

Calynda is currently at 0.1.0.

What is in this first edition:

- Recursive-descent parsing and source-aware diagnostics.
- Semantic analysis with symbol resolution and type checking.
- Lowering through AST, HIR, MIR, LIR, codegen planning, machine emission, and assembly emission.
- Native compilation for Linux x86_64 SysV ELF.
- Portable `portable-v1` bytecode emission.
- A small VS Code syntax extension in [vscode-calynda/README.md](vscode-calynda/README.md).

What is not in this edition:

- No interpreter path.
- No bytecode executor yet.
- No full language server yet.

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
- the required `runtime.o` beside that binary so generated executables can link correctly

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
calynda build <source.cal> [-o output]
calynda run <source.cal> [args...]
calynda asm <source.cal>
calynda bytecode <source.cal>
calynda help
```

What they do:

- `build`: compile a source file to a native executable
- `run`: build a temporary native executable and execute it
- `asm`: emit native x86_64 assembly to stdout
- `bytecode`: emit `portable-v1` bytecode text to stdout

## Language Snapshot

The grammar lives in [compiler/calynda.ebnf](compiler/calynda.ebnf).

Current language surface includes:

- a required single `start(string[] args)` entry point
- first-class lambdas with arrow syntax
- arrays
- function calls
- member access and index access
- casts
- assignments
- ternary expressions
- template literals with interpolation
- `throw`

The project is intentionally compiled-only right now. Native code and bytecode are the supported backend paths; interpreting AST or IR directly is not part of the design.

## Backend Model

Current backend split:

- native: AST -> HIR -> MIR -> LIR -> CodegenPlan -> MachineProgram -> x86_64 assembly -> linked executable
- bytecode: AST -> HIR -> MIR -> BytecodeProgram `portable-v1`

The backend direction is described in [backend_strategy.md](backend_strategy.md), and the current bytecode shape is described in [bytecode_isa.md](bytecode_isa.md).

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

- Native compilation currently targets Linux x86_64 SysV ELF.
- Final native linking currently uses `gcc`.
- The runtime surface is deliberately small and only covers the helpers the compiler lowers to today.
- The bytecode backend emits a portable compiled form, but this repository does not yet ship a bytecode runner.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).