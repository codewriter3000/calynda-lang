#ifndef CALYNDA_TARGET_H
#define CALYNDA_TARGET_H

#include <stdbool.h>
#include <stddef.h>

#define TARGET_MAX_REGISTERS     32
#define TARGET_MAX_ARG_REGISTERS 8
#define TARGET_MAX_ALLOCATABLE   16

typedef enum {
    TARGET_KIND_X86_64_SYSV_ELF = 0,
    TARGET_KIND_AARCH64_AAPCS_ELF,
    TARGET_KIND_RISCV64_LP64D_ELF
} TargetKind;

typedef enum {
    TARGET_ASM_SYNTAX_GNU_X86_64 = 0,
    TARGET_ASM_SYNTAX_GNU_AARCH64,
    TARGET_ASM_SYNTAX_GNU_RISCV64
} TargetAsmSyntax;

typedef struct {
    int         id;
    const char *name;
} TargetRegister;

typedef struct {
    TargetKind     kind;
    const char    *name;
    TargetAsmSyntax asm_syntax;
    size_t         word_size;
    size_t         stack_alignment;

    /* Full register set */
    const TargetRegister *registers;
    size_t                register_count;

    /* ABI registers */
    TargetRegister  return_register;
    TargetRegister  frame_pointer;
    TargetRegister  stack_pointer;
    TargetRegister  closure_env_register;

    /* Argument passing registers (caller side) */
    const TargetRegister *arg_registers;
    size_t                arg_register_count;

    /* Work/scratch register (not in allocatable set) */
    TargetRegister  work_register;

    /* Allocatable virtual registers */
    const TargetRegister *allocatable_registers;
    size_t                allocatable_register_count;

    /* Callee-saved registers */
    const TargetRegister *callee_saved_registers;
    size_t                callee_saved_register_count;

    /* Linker command template */
    const char *linker_command;
    const char *linker_flags;
} TargetDescriptor;

const TargetDescriptor *target_get_descriptor(TargetKind kind);
const TargetDescriptor *target_get_default(void);
TargetKind target_kind_from_name(const char *name, bool *found);
const char *target_kind_name(TargetKind kind);

const char *target_register_name(const TargetDescriptor *target, int register_id);
int target_register_id_by_name(const TargetDescriptor *target, const char *name);
bool target_register_is_callee_saved(const TargetDescriptor *target, int register_id);

#endif /* CALYNDA_TARGET_H */
