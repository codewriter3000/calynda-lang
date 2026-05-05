# Mid-Level Intermediate Representation (MIR)

## Overview

The MIR is the second intermediate representation. It transforms HIR's structured control flow into a control-flow graph (CFG) of basic blocks with explicit control flow edges.

## Purpose

- Convert structured control flow to explicit basic blocks
- Introduce SSA-like form with explicit temporaries
- Lower lambda closures to explicit capture records
- Prepare for low-level code generation
- Enable dataflow analysis and optimization

## Key Components

### Files

- **mir.c/h**: MIR program structure and API
- **mir_instr_types.h**: MIR instruction types
- **mir_lower.c**: Main lowering driver (HIR → MIR)
- **mir_expr.c**: Expression lowering to instructions
- **mir_expr_helpers.c**: Expression lowering helpers
- **mir_assign.c**: Assignment statement lowering
- **mir_control.c**: Control flow construction
- **mir_lvalue.c**: L-value (assignable location) handling
- **mir_store.c**: Store operation lowering
- **mir_lambda.c**: Lambda and closure lowering
- **mir_capture.c**: Closure capture handling
- **mir_capture_analysis.c**: Capture analysis
- **mir_union.c**: Union type operations
- **mir_value.c**: Value representation
- **mir_builders.c**: Instruction builders
- **mir_cleanup.c**: Cleanup and validation
- **mir_internal.h**: Internal data structures
- **mir_dump.c**: MIR visualization
- **mir_dump_instr.c**: Instruction dumping
- **mir_dump_helpers.c**: Dump utilities
- **mir_dump_internal.h**: Internal dump helpers

## Structure

### Basic Blocks
MIR organizes code into basic blocks:
```c
typedef struct MirBasicBlock {
    char           *label;
    MirInstruction *instructions;
    MirTerminator   terminator;
} MirBasicBlock;
```

### Terminators
Each basic block ends with a terminator:
- **Return**: Exit the function
- **Goto**: Unconditional jump
- **Branch**: Conditional jump (if-then-else)
- **Throw**: Exception throw

### Instructions
MIR instructions operate on values and produce results:
- **Binary operations**: `add`, `sub`, `mul`, `eq`, etc.
- **Loads**: Read from locals or globals
- **Stores**: Write to memory locations
- **Calls**: Function and method calls
- **Casts**: Type conversions
- **Array/Union operations**: Construction and access

### Values
MIR values are:
- **Locals**: Function parameters and local variables
- **Temporaries**: Intermediate computation results
- **Constants**: Literal values
- **Globals**: Global functions and data

## Transformations from HIR

1. **Control Flow Lowering**: `if`/`while` → basic blocks with branches
2. **Expression Flattening**: Complex expressions → sequence of instructions
3. **Temporary Introduction**: Each operation result gets a temporary
4. **Closure Lowering**: Lambda → closure creation + capture list
5. **Member Access**: → explicit field offset operations

## Data Structures

### MIR Unit
```c
typedef struct MirUnit {
    char          *name;
    CheckedType    return_type;
    MirLocal      *locals;
    MirBasicBlock *blocks;
    size_t         parameter_count;
    size_t         next_temp_index;
} MirUnit;
```

### MIR Instruction
```c
typedef struct {
    MirInstructionKind kind;
    MirValue           dest;     // Result destination (if applicable)
    MirValue           operands[3];
    // Kind-specific data...
} MirInstruction;
```

## Analysis Enabled

- **Dataflow analysis**: Reaching definitions, liveness
- **Dead code elimination**: Unreachable blocks, unused values
- **Constant propagation**: Known values at compile-time
- **Common subexpression elimination**: Redundant computation removal

## Usage

```c
MirProgram mir;
mir_program_init(&mir);

if (!mir_build_program(&mir, &hir_program)) {
    const MirBuildError *error = mir_get_error(&mir);
    // Handle error
}

mir_dump_program(stdout, &mir);
mir_program_free(&mir);
```

## Design Notes

- MIR is the first representation with explicit control flow graph
- MIR uses a flat array of locals + temporaries (not full SSA)
- MIR is target-independent; no machine-specific details yet
- MIR captures all semantic information needed for code generation
## Changes in 1.0.0-alpha.6

- `mir_capture` and `mir_capture_analysis*` were extended to materialise capture-by-reference: captured locals are addressed indirectly through a closure record so writes are visible to the enclosing scope.
- `mir_expr_call.c`, `mir_lambda.c`, `mir_lvalue.c`, and `mir_store.c` lower untyped (`var`) parameters and `|var` non-local-return assignments.
- `mir_tco.c` self-tail-call elimination remains in effect; it now correctly handles the reference-capture indirection.
- `mir_internal.h` carries the new builder signatures; helper builders live in `mir_builders.c`.
