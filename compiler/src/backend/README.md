# Backend (Machine Code Emission)

## Overview

The backend is the final phase of the compiler. It emits target-specific assembly code and executable binaries from the codegen representation.

## Purpose

- Emit native assembly code (x86-64, AArch64, etc.)
- Generate object files and executables
- Handle calling conventions and ABI requirements
- Emit proper prologue/epilogue for functions
- Generate runtime support code integration
- Create executable entry points

## Key Components

### Machine Code Generation

#### Files
- **machine.c/h**: Machine program structure and API
- **machine_build.c**: Build machine instructions from codegen
- **machine_emit.c**: High-level emission coordination
- **machine_instr.c**: Instruction emission
- **machine_instr_rt.c**: Runtime helper call emission
- **machine_instr_rt_ext.c**: Extended runtime operations
- **machine_ops.c**: Specific operation implementations
- **machine_operand.c**: Operand formatting
- **machine_helpers.c**: Helper functions
- **machine_layout.c**: Stack frame layout
- **machine_internal.h**: Internal data structures
- **machine_dump.c**: Machine code visualization

### Assembly Emission

#### Files
- **asm_emit.c/h**: Assembly emission API
- **asm_emit_instr.c**: Instruction emission
- **asm_emit_instr_aarch64.c**: AArch64-specific instructions
- **asm_emit_operand.c**: Operand formatting
- **asm_emit_operand_ext.c**: Extended operand handling
- **asm_emit_entry.c**: Function prologue/epilogue
- **asm_emit_entry_aarch64.c**: AArch64-specific entry/exit
- **asm_emit_helpers.c**: Assembly formatting helpers
- **asm_emit_sections.c**: Section management (.text, .data, .rodata)
- **asm_emit_symbols.c**: Symbol and label management
- **asm_emit_internal.h**: Internal emit helpers

### Target Descriptions

#### Files
- **target.c/h**: Target descriptor API
- **target_x86_64.c**: x86-64 System V ABI
- **target_aarch64.c**: AArch64 (ARM64) support

### Runtime ABI

#### Files
- **runtime_abi.c/h**: Runtime function interface
- **runtime_abi_names.c**: Runtime function names
- **runtime_abi_dump.c**: Runtime call visualization

## Supported Targets

### x86-64 (System V ABI)
- Supports Linux, macOS, BSD
- Register usage: RAX, RDI, RSI, RDX, RCX, R8, R9, R10, R11
- Calling convention: arguments in registers then stack
- Stack alignment: 16-byte

### AArch64 (ARM64)
- Supports Linux, macOS (Apple Silicon)
- Register usage: X0-X30, SP, FP
- Calling convention: arguments in X0-X7
- Stack alignment: 16-byte

## Assembly Output Format

The backend generates AT&T syntax assembly (`.s` files):
```asm
    .text
    .globl _start
_start:
    pushq   %rbp
    movq    %rsp, %rbp
    subq    $32, %rsp
    # ... function body ...
    movq    %rbp, %rsp
    popq    %rbp
    ret
```

## Frame Layout

Stack frame structure (x86-64 example):
```
High addresses
+------------------+
| Return address   |
+------------------+
| Saved RBP        | <- RBP
+------------------+
| Local variables  |
+------------------+
| Spill slots      |
+------------------+
| Temp storage     |
+------------------+
| Outgoing args    | <- RSP
+------------------+
Low addresses
```

## Calling Convention Implementation

The backend implements:
1. **Argument Passing**: registers vs stack
2. **Return Values**: register or memory
3. **Register Preservation**: caller-saved vs callee-saved
4. **Stack Alignment**: ABI-mandated alignment
5. **Red Zone**: (x86-64 System V ABI)

## Runtime Integration

The backend emits calls to runtime functions for:
- Memory allocation
- String operations
- Closure creation and invocation
- Dynamic dispatch
- Type conversions
- Exception handling

## Data Structures

### Machine Program
```c
typedef struct {
    CodegenTargetKind    target;
    TargetDescriptor    *target_desc;
    MachineUnit         *units;
} MachineProgram;
```

### Machine Unit
```c
typedef struct {
    char              *name;
    MachineFrameSlot  *frame_slots;
    size_t             spill_slot_count;
    MachineBlock      *blocks;
    bool               is_exported;
} MachineUnit;
```

### Machine Instruction
```c
typedef struct {
    char *text;  // Assembly instruction text
} MachineInstruction;
```

## Usage

```c
MachineProgram machine;
machine_program_init(&machine);

if (!machine_build_program(&machine, &lir_program, &codegen_program)) {
    const MachineBuildError *error = machine_get_error(&machine);
    // Handle error
}

// Emit assembly
bool asm_emit_program(stdout, &machine, &target_desc);

machine_program_free(&machine);
```

## Building Executables

The emitted assembly can be assembled and linked:
```bash
# Assemble
as output.s -o output.o

# Link with runtime
gcc output.o -L./runtime -lcalynda_rt -o program

# Run
./program
```

## Design Notes

- The backend is highly target-specific
- Assembly emission uses text generation (future: binary emission)
- The backend relies on external assembler (GNU as, LLVM)
- Runtime library is linked separately
- Future targets can be added by implementing target descriptors
## Changes in 1.0.0-alpha.6

- The runtime is now produced as two archives: hosted (`calynda_runtime.a`) and freestanding/bare-metal (`calynda_runtime_boot.a`). The boot archive is compiled with `-ffreestanding -fno-builtin -fno-stack-protector` and linked into `boot -> { ... };` programs.
- Cross-compiled boot archives (`calynda_runtime_boot_aarch64.a`, `calynda_runtime_boot_riscv64.a`) are produced by `make runtime-aarch64` / `make runtime-riscv64`.
- `machine_layout.c` was updated to align with the new entry-point split; `runtime_abi/runtime_abi.c` exposes the new NLR helpers.
