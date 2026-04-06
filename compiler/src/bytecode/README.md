# Bytecode

## Overview

The bytecode phase generates a portable virtual machine bytecode from the MIR. This is an alternative to native code generation for interpretation or JIT compilation.

## Purpose

- Generate portable bytecode for cross-platform execution
- Enable faster development iteration (interpret instead of compile)
- Support dynamic loading and hot-swapping
- Provide foundation for JIT compilation
- Enable debugging and profiling at bytecode level

## Key Components

### Files

- **bytecode.c/h**: Bytecode program structure and API
- **bytecode_instr_types.h**: Bytecode instruction types
- **bytecode_internal.h**: Internal data structures
- **bytecode_lower.c**: Main lowering driver (MIR → Bytecode)
- **bytecode_lower_instr.c**: Instruction lowering
- **bytecode_constants.c**: Constant pool management
- **bytecode_memory.c**: Memory management
- **bytecode_helpers.c**: Helper functions
- **bytecode_dump.c**: Bytecode visualization
- **bytecode_dump_instr.c**: Instruction dumping
- **bytecode_dump_helpers.c**: Dump utilities
- **bytecode_dump_names.c**: Name formatting
- **bytecode_dump_internal.h**: Internal dump helpers

## Bytecode Format

### Instruction Set
Calynda bytecode is stack-based with the following categories:

#### Stack Operations
- `PUSH`: Push value onto stack
- `POP`: Pop value from stack
- `DUP`: Duplicate top of stack
- `LOAD_LOCAL`: Load local variable
- `STORE_LOCAL`: Store to local variable

#### Arithmetic & Logic
- `ADD`, `SUB`, `MUL`, `DIV`, `MOD`
- `AND`, `OR`, `XOR`, `NOT`
- `SHL`, `SHR` (shift left/right)
- `EQ`, `NE`, `LT`, `LE`, `GT`, `GE`

#### Control Flow
- `JUMP`: Unconditional jump
- `JUMP_IF_TRUE`: Conditional jump
- `JUMP_IF_FALSE`: Conditional jump
- `CALL`: Function call
- `RETURN`: Return from function
- `THROW`: Throw exception

#### Object Operations
- `NEW_ARRAY`: Create array
- `LOAD_INDEX`: Array element access
- `STORE_INDEX`: Array element assignment
- `LOAD_MEMBER`: Object member access
- `STORE_MEMBER`: Object member assignment
- `NEW_CLOSURE`: Create closure
- `LOAD_CAPTURE`: Access captured variable

### Constant Pool
Bytecode files include a constant pool containing:
- Integer literals
- Float literals
- String literals
- Type descriptors
- Function names

### Binary Format
```
┌─────────────────────────┐
│ Header                  │
│  - Magic: "CALBC"       │
│  - Version: 1           │
│  - Target: VM/JIT       │
├─────────────────────────┤
│ Constant Pool           │
│  - Count                │
│  - Constants[]          │
├─────────────────────────┤
│ Units[]                 │
│  - Name                 │
│  - Locals count         │
│  - Bytecode blocks[]    │
└─────────────────────────┘
```

## Data Structures

### Bytecode Program
```c
typedef struct {
    BytecodeTargetKind  target;      // VM or JIT
    BytecodeConstant   *constants;
    size_t              constant_count;
    BytecodeUnit       *units;
    size_t              unit_count;
} BytecodeProgram;
```

### Bytecode Unit
```c
typedef struct {
    char              *name;
    BytecodeLocal     *locals;
    size_t             local_count;
    BytecodeBasicBlock *blocks;
    size_t             block_count;
} BytecodeUnit;
```

### Bytecode Instruction
```c
typedef struct {
    BytecodeOpcode  opcode;
    BytecodeValue   operands[3];
    // Opcode-specific data...
} BytecodeInstruction;
```

## Execution Model

### Stack Machine
- Operands pushed onto evaluation stack
- Operations consume from stack, push result
- Locals accessed by index

### Call Frame
Each function call creates a frame:
```
+------------------+
| Return address   |
+------------------+
| Locals           |
+------------------+
| Evaluation stack |
+------------------+
```

## Optimizations

Bytecode-level optimizations:
- **Constant folding**: Evaluate constant expressions at compile-time
- **Dead code elimination**: Remove unreachable bytecode
- **Peephole optimization**: Local instruction pattern matching
- **Jump threading**: Optimize jump chains

## Usage

```c
BytecodeProgram bytecode;
bytecode_program_init(&bytecode);

if (!bytecode_build_program(&bytecode, &mir_program)) {
    const BytecodeBuildError *error = bytecode_get_error(&bytecode);
    // Handle error
}

// Dump human-readable bytecode
bytecode_dump_program(stdout, &bytecode);

// Save to file (binary format)
bytecode_save(&bytecode, "output.calbc");

bytecode_program_free(&bytecode);
```

## Interpretation vs Compilation

### Bytecode Interpreter
- Execute bytecode directly
- Slower but simple
- Good for development/debugging

### JIT Compilation
- Compile bytecode to native code at runtime
- Fast execution
- Complex implementation

## Design Notes

- Bytecode is platform-independent
- Bytecode format is versioned for compatibility
- Bytecode can be serialized and deserialized
- Future: Add bytecode verifier for safety
- Future: Support for bytecode-level debugging