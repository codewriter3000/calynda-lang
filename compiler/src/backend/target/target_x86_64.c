#include "target.h"

/*
 * x86_64 SysV ELF register IDs — these match the CodegenRegister enum values
 * for backward compatibility during the refactoring transition.
 */
enum {
    X86_64_REG_RAX = 0,
    X86_64_REG_RDI,
    X86_64_REG_RSI,
    X86_64_REG_RDX,
    X86_64_REG_RCX,
    X86_64_REG_R8,
    X86_64_REG_R9,
    X86_64_REG_R10,
    X86_64_REG_R11,
    X86_64_REG_R12,
    X86_64_REG_R13,
    X86_64_REG_R14,
    X86_64_REG_R15,
    X86_64_REG_RBP,
    X86_64_REG_RSP,
    X86_64_REG_RBX
};

static const TargetRegister x86_64_registers[] = {
    { X86_64_REG_RAX, "rax" },
    { X86_64_REG_RDI, "rdi" },
    { X86_64_REG_RSI, "rsi" },
    { X86_64_REG_RDX, "rdx" },
    { X86_64_REG_RCX, "rcx" },
    { X86_64_REG_R8,  "r8"  },
    { X86_64_REG_R9,  "r9"  },
    { X86_64_REG_R10, "r10" },
    { X86_64_REG_R11, "r11" },
    { X86_64_REG_R12, "r12" },
    { X86_64_REG_R13, "r13" },
    { X86_64_REG_R14, "r14" },
    { X86_64_REG_R15, "r15" },
    { X86_64_REG_RBP, "rbp" },
    { X86_64_REG_RSP, "rsp" },
    { X86_64_REG_RBX, "rbx" }
};

static const TargetRegister x86_64_arg_registers[] = {
    { X86_64_REG_RDI, "rdi" },
    { X86_64_REG_RSI, "rsi" },
    { X86_64_REG_RDX, "rdx" },
    { X86_64_REG_RCX, "rcx" },
    { X86_64_REG_R8,  "r8"  },
    { X86_64_REG_R9,  "r9"  }
};

static const TargetRegister x86_64_allocatable_registers[] = {
    { X86_64_REG_R10, "r10" },
    { X86_64_REG_R11, "r11" },
    { X86_64_REG_R12, "r12" },
    { X86_64_REG_R13, "r13" }
};

static const TargetRegister x86_64_callee_saved_registers[] = {
    { X86_64_REG_RBX, "rbx" },
    { X86_64_REG_R12, "r12" },
    { X86_64_REG_R13, "r13" },
    { X86_64_REG_R14, "r14" },
    { X86_64_REG_R15, "r15" },
    { X86_64_REG_RBP, "rbp" }
};

static const TargetDescriptor x86_64_sysv_elf_descriptor = {
    .kind                        = TARGET_KIND_X86_64_SYSV_ELF,
    .name                        = "x86_64_sysv_elf",
    .asm_syntax                  = TARGET_ASM_SYNTAX_GNU_X86_64,
    .word_size                   = 8,
    .stack_alignment             = 16,

    .registers                   = x86_64_registers,
    .register_count              = sizeof(x86_64_registers) / sizeof(x86_64_registers[0]),

    .return_register             = { X86_64_REG_RAX, "rax" },
    .frame_pointer               = { X86_64_REG_RBP, "rbp" },
    .stack_pointer               = { X86_64_REG_RSP, "rsp" },
    .closure_env_register        = { X86_64_REG_R15, "r15" },

    .work_register               = { X86_64_REG_R14, "r14" },

    .arg_registers               = x86_64_arg_registers,
    .arg_register_count          = sizeof(x86_64_arg_registers) / sizeof(x86_64_arg_registers[0]),

    .allocatable_registers       = x86_64_allocatable_registers,
    .allocatable_register_count  = sizeof(x86_64_allocatable_registers) / sizeof(x86_64_allocatable_registers[0]),

    .callee_saved_registers      = x86_64_callee_saved_registers,
    .callee_saved_register_count = sizeof(x86_64_callee_saved_registers) / sizeof(x86_64_callee_saved_registers[0]),

    .linker_command              = "gcc",
    .linker_flags                = "-no-pie"
};

const TargetDescriptor *target_get_x86_64_descriptor(void) {
    return &x86_64_sysv_elf_descriptor;
}
