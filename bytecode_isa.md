# Bytecode ISA

## Target

The current bytecode backend targets `portable-v1`.

It is a compiled backend, not an interpreter surface. The compiler lowers MIR into a structured bytecode program made of:

- a constant pool,
- bytecode units,
- bytecode basic blocks,
- explicit instructions and terminators.

## Program Shape

Each `BytecodeProgram` contains:

- a target tag: `portable-v1`
- a constant pool shared across all units
- one bytecode unit for each MIR unit

The constant pool currently interns three categories:

- literals
- symbols such as globals and member names
- template text fragments

## Units And Blocks

Bytecode units preserve the MIR unit split:

- `start`
- `binding`
- `init`
- `lambda`

Each unit carries:

- its name
- return type
- locals with kind and type metadata
- parameter count
- temp count
- an ordered list of basic blocks

Each block keeps:

- a label
- a linear instruction sequence
- one explicit terminator

## Values

Bytecode operands are explicit value references:

- temp
- local
- global symbol reference
- constant-pool reference

This keeps the bytecode close to MIR semantics while making all symbolic data interned and backend-owned.

## Instructions

The initial opcode set mirrors the current MIR surface:

- `BC_BINARY`
- `BC_UNARY`
- `BC_CLOSURE`
- `BC_CALL`
- `BC_CAST`
- `BC_MEMBER`
- `BC_INDEX_LOAD`
- `BC_ARRAY_LITERAL`
- `BC_TEMPLATE`
- `BC_STORE_LOCAL`
- `BC_STORE_GLOBAL`
- `BC_STORE_INDEX`
- `BC_STORE_MEMBER`
- `BC_UNION_NEW`
- `BC_UNION_GET_TAG`
- `BC_UNION_GET_PAYLOAD`
- `BC_HETERO_ARRAY_NEW`
- `BC_HETERO_ARRAY_GET_TAG`

Terminator opcodes are:

- `BC_RETURN`
- `BC_JUMP`
- `BC_BRANCH`
- `BC_THROW`

## Concurrency Helper Lowering

Portable bytecode keeps alpha.2 concurrency on the existing helper-call path.
There are no new concurrency-specific opcodes. `spawn`, `Thread.join()`,
`Thread.cancel()`, `Future<T>.get()`, `Future<T>.cancel()`, `Mutex.new()`,
`Mutex.lock()`, `Mutex.unlock()`, and `Atomic<T>.new/load/store/exchange`
lower as `BC_CALL` operations targeting the well-known `__calynda_rt_*`
runtime symbols.

Bytecode execution therefore still depends on a runtime that implements the
threading, future, mutex, and atomic helper surface.

## Backend Boundary

The backend split is now:

- native: AST -> HIR -> MIR -> LIR -> CodegenPlan -> MachineProgram -> x86_64 assembly -> linked executable
- bytecode: AST -> HIR -> MIR -> BytecodeProgram portable-v1

The bytecode backend exists so future portable execution remains compiled. Any future VM work should execute this bytecode format or a successor derived from it, not interpret AST, HIR, MIR, or LIR directly.
