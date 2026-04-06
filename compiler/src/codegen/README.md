# Code Generation

## Overview

The codegen phase performs target-specific preparation for native code generation. It selects instructions, allocates registers, and determines stack frame layout.

## Purpose

- Perform instruction selection (LIR instructions → machine instructions)
- Allocate virtual registers to physical registers
- Determine stack frame layout
- Identify which operations need runtime support
- Prepare for assembly emission

## Key Components

### Files

- **codegen.c/h**: Codegen program structure and API
- **codegen_build.c**: Main codegen driver (LIR → Codegen)
- **codegen_helpers.c**: Helper functions
- **codegen_infer.c**: Type and operation inference
- **codegen_internal.h**: Internal data structures
- **codegen_dump.c**: Codegen visualization
- **codegen_dump_helpers.c**: Dump utilities
- **codegen_names.c**: Name formatting

## Key Concepts

### Instruction Selection
Map LIR instructions to either:
1. **Direct Patterns**: Can be implemented with native instructions
   - Arithmetic operations
   - Comparisons
   - Loads and stores
   - Function calls

2. **Runtime Helpers**: Require runtime library support
   - Closure creation
   - Dynamic dispatch
   - Type casting
   - Complex data structure operations

### Register Allocation
Allocate virtual registers to physical registers:
```c
typedef struct {
    size_t             vreg_index;
    CodegenVRegLocation location;  // Register or spill slot
} CodegenVRegAllocation;
```

### Frame Layout
Determine stack frame structure:
- Parameter locations
- Local variable slots
- Spill slots for register pressure
- Saved registers
- Return address

## Data Structures

### Codegen Program
```c
typedef struct {
    CodegenTargetKind    target;
    TargetDescriptor    *target_desc;
    CodegenUnit         *units;
} CodegenProgram;
```

### Codegen Unit
```c
typedef struct {
    char                  *name;
    CodegenFrameSlot      *frame_slots;
    CodegenVRegAllocation *vreg_allocations;
    size_t                 spill_slot_count;
    CodegenBlock          *blocks;
} CodegenUnit;
```

### Selected Instructions
```c
typedef struct {
    LirInstructionKind kind;
    CodegenSelection   selection;  // Direct or runtime
} CodegenSelectedInstruction;
```

## Register Allocation Strategy

Current implementation uses a simple strategy:
1. **Caller-saved registers** for temporaries
2. **Callee-saved registers** for long-lived values
3. **Spill to stack** when running out of registers

Future: Graph-coloring or linear-scan register allocation

## Target Abstraction

The codegen phase queries the target descriptor for:
- Available registers
- Calling convention
- Instruction capabilities
- ABI requirements

## Usage

```c
const TargetDescriptor *target = target_get_x86_64_sysv();

CodegenProgram codegen;
codegen_program_init(&codegen);

if (!codegen_build_program(&codegen, &lir_program, target)) {
    const CodegenBuildError *error = codegen_get_error(&codegen);
    // Handle error
}

codegen_dump_program(stdout, &codegen);
codegen_program_free(&codegen);
```

## Design Notes

- Codegen is target-aware but still somewhat abstract
- Physical machine code emission happens in the backend
- Codegen decisions directly impact performance
- Register allocation is critical for efficiency