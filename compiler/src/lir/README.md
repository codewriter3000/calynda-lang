# Low-Level Intermediate Representation (LIR)

## Overview

The LIR is the third and final intermediate representation before native code generation. It performs machine-independent optimizations and prepares for register allocation.

## Purpose

- Lower MIR to a more machine-friendly form
- Introduce explicit memory operations (spills, loads)
- Perform slot allocation (pre-register allocation)
- Make calling conventions explicit
- Optimize instruction sequences
- Prepare for target-specific code generation

## Key Components

### Files

- **lir.c/h**: LIR program structure and API
- **lir_instr_types.h**: LIR instruction types
- **lir_internal.h**: Internal data structures
- **lir_lower.c**: Main lowering driver (MIR → LIR)
- **lir_lower_instr.c**: Instruction lowering
- **lir_lower_instr_ext.c**: Extended instruction lowering
- **lir_lower_instr_stores.c**: Store operation lowering
- **lir_helpers.c**: Helper functions
- **lir_memory.c**: Memory management
- **lir_dump.c**: LIR visualization
- **lir_dump_instr.c**: Instruction dumping
- **lir_dump_helpers.c**: Dump utilities
- **lir_dump_internal.h**: Internal dump helpers

## Key Concepts

### Slots
LIR uses "slots" instead of arbitrary temporaries:
```c
typedef enum {
    LIR_SLOT_PARAMETER,
    LIR_SLOT_LOCAL,
    LIR_SLOT_TEMP
} LirSlotKind;
```

Each slot represents:
- A function parameter
- A local variable
- A temporary value

### Virtual Registers
LIR introduces virtual registers that will later be allocated to physical registers:
```c
typedef struct {
    size_t vreg_index;
    CheckedType type;
} LirOperand;
```

### Calling Convention Preparation
LIR makes function calls explicit:
- Argument passing order
- Return value handling
- Caller-saved vs callee-saved registers (prepared for)

## Transformations from MIR

1. **Slot Allocation**: Assign each MIR local/temp to a LIR slot
2. **Virtual Register Introduction**: Create vregs for instruction operands
3. **Instruction Selection Hints**: Mark instructions with patterns for codegen
4. **Control Flow Preservation**: Keep same basic block structure
5. **Memory Operation Lowering**: Explicit load/store for complex types

## Data Structures

### LIR Unit
```c
typedef struct LirUnit {
    char          *name;
    CheckedType    return_type;
    LirSlot       *slots;
    size_t         virtual_register_count;
    LirBasicBlock *blocks;
} LirUnit;
```

### LIR Instruction
```c
typedef struct {
    LirInstructionKind kind;
    LirOperand         dest;
    LirOperand         operands[3];
    // Kind-specific data...
} LirInstruction;
```

### LIR Operand Types
- **Virtual Register**: `vreg_42`
- **Immediate**: Constant values
- **Slot**: Direct reference to stack slot
- **Global Symbol**: Function or data name

## Optimization Opportunities

- **Redundant Load/Store Elimination**
- **Copy Propagation**
- **Dead Store Elimination**
- **Instruction Combining**

## Usage

```c
LirProgram lir;
lir_program_init(&lir);

if (!lir_build_program(&lir, &mir_program)) {
    const LirBuildError *error = lir_get_error(&lir);
    // Handle error
}

lir_dump_program(stdout, &lir);
lir_program_free(&lir);
```

## Design Notes

- LIR is still target-independent but closer to machine code
- LIR separates logical operations from physical register assignment
- LIR enables cross-platform optimizations before target selection
- LIR is designed to make register allocation straightforward