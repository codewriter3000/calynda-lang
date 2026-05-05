# Command-Line Interface (CLI)

## Overview

The CLI module implements the `calynda` compiler command-line tool. It provides user-facing commands for compiling, building, and managing Calynda projects.

## Purpose

- Provide command-line interface to the compiler
- Coordinate all compilation phases
- Handle file I/O and error reporting
- Support various output formats
- Manage build artifacts and caching

## Key Components

### Files

- **calynda.c**: Main entry point and command dispatcher
- **calynda_compile.c**: Compilation pipeline coordination
- **calynda_car.c**: CAR archive operations
- **calynda_utils.c**: CLI utilities
- **calynda_internal.h**: Internal CLI structures
- **dump_ast.c**: AST dumping command
- **dump_semantics.c**: Semantic information dumping
- **emit_bytecode.c**: Bytecode emission
- **emit_asm.c**: Assembly emission
- **build_native.c**: Native executable building
- **build_native_utils.c**: Build utilities

## Commands

### `calynda compile <file>`
Compile a single source file to bytecode or native code.

**Options:**
- `--output`, `-o`: Output file path
- `--target`: Target platform (x86_64, aarch64, bytecode)
- `--optimize`, `-O`: Optimization level
- `--emit`: Emission format (asm, bytecode, object)

**Example:**
```bash
calynda compile main.cal -o main --target x86_64
```

### `calynda build [project]`
Build a complete project with dependencies.

**Options:**
- `--release`: Build in release mode (optimizations enabled)
- `--target`: Target platform
- `--output-dir`: Build artifact directory

**Example:**
```bash
calynda build --release --target aarch64
```

### `calynda car <command>`
Manage CAR (Calynda Archive) source archives.

**Subcommands:**
- `create <dir> -o <file>`: Create archive from directory
- `extract <file> -o <dir>`: Extract archive to directory
- `list <file>`: List files in archive
- `info <file>`: Show archive information

**Example:**
```bash
calynda car create ./src -o mylib.car
calynda car list mylib.car
```

### `calynda dump <type> <file>`
Dump internal compiler representations for debugging.

**Types:**
- `ast`: Abstract Syntax Tree
- `symbols`: Symbol table
- `types`: Type information
- `hir`: High-level IR
- `mir`: Mid-level IR
- `lir`: Low-level IR
- `codegen`: Code generation info
- `machine`: Machine code
- `bytecode`: Bytecode

**Example:**
```bash
calynda dump ast main.cal
calynda dump mir main.cal
```

### `calynda run <file>`
Compile and immediately execute a Calynda program.

**Example:**
```bash
calynda run hello.cal
```

## Compilation Pipeline

The CLI orchestrates the full compilation pipeline:

```
Source Code (.cal)
    ↓
Tokenizer
    ↓
Parser → AST
    ↓
Symbol Table Builder
    ↓
Type Checker
    ↓
HIR Lowering
    ↓
MIR Lowering
    ↓
LIR Lowering
    ↓
┌───────────┬──────────────┐
│           │              │
Bytecode    Codegen        
            ↓              
            Backend        
            ↓              
        Assembly (.s)      
            ↓              
        Assemble & Link    
            ↓              
        Executable         
```

## Error Reporting

The CLI provides user-friendly error messages with:
- Source location (file, line, column)
- Code snippet with highlighting
- Error message and suggestions
- Related spans for context

Example output:
```
error: type mismatch in assignment
  --> main.cal:12:5
   |
12 |     x = "hello";
   |         ^^^^^^^ expected int32, found string
   |
note: variable declared here
  --> main.cal:10:5
   |
10 |     var x: int32 = 0;
   |         ^
```

## Build System Integration

The CLI supports:
- **Dependency Resolution**: Find and compile dependencies
- **Incremental Compilation**: Only recompile changed files
- **Caching**: Cache compilation artifacts
- **Parallel Builds**: Compile independent modules concurrently (planned)

## Configuration

### Project File (`calynda.toml`)
```toml
[package]
name = "myproject"
version = "0.1.0"
authors = ["Your Name"]

[dependencies]
stdlib = "1.0"

[build]
target = "x86_64"
optimize = true
```

## Native Executable Building

The `build_native` module:
1. Compiles source to assembly
2. Invokes system assembler (`as`)
3. Links with runtime library
4. Produces executable

**Requirements:**
- System assembler (GNU as or LLVM)
- System linker (ld or lld)
- Calynda runtime library

## Design Notes

- CLI is designed to be user-friendly and informative
- Error messages follow rustc/clang style
- Build system is incremental and cacheable
- CLI is the primary interface for developers
- Future: Language server protocol (LSP) integration
## Changes in 1.0.0-alpha.6

- `build_native.c` / `build_native_utils.c` link the freestanding boot archive (`calynda_runtime_boot.a`) for `boot -> { ... };` programs and the hosted archive (`calynda_runtime.a`) otherwise.
- `calynda_compile.c`, `calynda_commands.c`, `calynda_car.c`, and `calynda_utils.c` were extended for the new archive surface and standard-library auto-discovery.
- The driver continues to expose `--archive path.car` and `--archive-path dir`; bundled stdlib modules are picked up implicitly when present.
