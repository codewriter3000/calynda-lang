#include "target.h"

/*
 * AArch64 AAPCS64 ELF register IDs.
 * These form an independent ID space from x86_64 — register IDs are
 * only meaningful within the context of a specific TargetDescriptor.
 */
enum {
    AARCH64_REG_X0 = 0,
    AARCH64_REG_X1,
    AARCH64_REG_X2,
    AARCH64_REG_X3,
    AARCH64_REG_X4,
    AARCH64_REG_X5,
    AARCH64_REG_X6,
    AARCH64_REG_X7,
    AARCH64_REG_X8,
    AARCH64_REG_X9,
    AARCH64_REG_X10,
    AARCH64_REG_X11,
    AARCH64_REG_X12,
    AARCH64_REG_X13,
    AARCH64_REG_X14,
    AARCH64_REG_X15,
    AARCH64_REG_X16,
    AARCH64_REG_X17,
    AARCH64_REG_X18,
    AARCH64_REG_X19,
    AARCH64_REG_X20,
    AARCH64_REG_X21,
    AARCH64_REG_X22,
    AARCH64_REG_X23,
    AARCH64_REG_X24,
    AARCH64_REG_X25,
    AARCH64_REG_X26,
    AARCH64_REG_X27,
    AARCH64_REG_X28,
    AARCH64_REG_X29,     /* frame pointer (FP) */
    AARCH64_REG_X30,     /* link register (LR) */
    AARCH64_REG_SP
};

static const TargetRegister aarch64_registers[] = {
    { AARCH64_REG_X0,  "x0"  },
    { AARCH64_REG_X1,  "x1"  },
    { AARCH64_REG_X2,  "x2"  },
    { AARCH64_REG_X3,  "x3"  },
    { AARCH64_REG_X4,  "x4"  },
    { AARCH64_REG_X5,  "x5"  },
    { AARCH64_REG_X6,  "x6"  },
    { AARCH64_REG_X7,  "x7"  },
    { AARCH64_REG_X8,  "x8"  },
    { AARCH64_REG_X9,  "x9"  },
    { AARCH64_REG_X10, "x10" },
    { AARCH64_REG_X11, "x11" },
    { AARCH64_REG_X12, "x12" },
    { AARCH64_REG_X13, "x13" },
    { AARCH64_REG_X14, "x14" },
    { AARCH64_REG_X15, "x15" },
    { AARCH64_REG_X16, "x16" },
    { AARCH64_REG_X17, "x17" },
    { AARCH64_REG_X18, "x18" },
    { AARCH64_REG_X19, "x19" },
    { AARCH64_REG_X20, "x20" },
    { AARCH64_REG_X21, "x21" },
    { AARCH64_REG_X22, "x22" },
    { AARCH64_REG_X23, "x23" },
    { AARCH64_REG_X24, "x24" },
    { AARCH64_REG_X25, "x25" },
    { AARCH64_REG_X26, "x26" },
    { AARCH64_REG_X27, "x27" },
    { AARCH64_REG_X28, "x28" },
    { AARCH64_REG_X29, "x29" },
    { AARCH64_REG_X30, "x30" },
    { AARCH64_REG_SP,  "sp"  }
};

/* AAPCS64: x0-x7 for argument passing */
static const TargetRegister aarch64_arg_registers[] = {
    { AARCH64_REG_X0,  "x0"  },
    { AARCH64_REG_X1,  "x1"  },
    { AARCH64_REG_X2,  "x2"  },
    { AARCH64_REG_X3,  "x3"  },
    { AARCH64_REG_X4,  "x4"  },
    { AARCH64_REG_X5,  "x5"  },
    { AARCH64_REG_X6,  "x6"  },
    { AARCH64_REG_X7,  "x7"  }
};

/* Caller-saved temporaries used for vreg allocation */
static const TargetRegister aarch64_allocatable_registers[] = {
    { AARCH64_REG_X9,  "x9"  },
    { AARCH64_REG_X10, "x10" },
    { AARCH64_REG_X11, "x11" },
    { AARCH64_REG_X12, "x12" },
    { AARCH64_REG_X13, "x13" },
    { AARCH64_REG_X14, "x14" },
    { AARCH64_REG_X15, "x15" }
};

/* AAPCS64 callee-saved: x19-x28, x29 (FP), x30 (LR) */
static const TargetRegister aarch64_callee_saved_registers[] = {
    { AARCH64_REG_X19, "x19" },
    { AARCH64_REG_X20, "x20" },
    { AARCH64_REG_X21, "x21" },
    { AARCH64_REG_X22, "x22" },
    { AARCH64_REG_X23, "x23" },
    { AARCH64_REG_X24, "x24" },
    { AARCH64_REG_X25, "x25" },
    { AARCH64_REG_X26, "x26" },
    { AARCH64_REG_X27, "x27" },
    { AARCH64_REG_X28, "x28" },
    { AARCH64_REG_X29, "x29" },
    { AARCH64_REG_X30, "x30" }
};

static const TargetDescriptor aarch64_aapcs_elf_descriptor = {
    .kind                        = TARGET_KIND_AARCH64_AAPCS_ELF,
    .name                        = "aarch64_aapcs_elf",
    .asm_syntax                  = TARGET_ASM_SYNTAX_GNU_AARCH64,
    .word_size                   = 8,
    .stack_alignment             = 16,

    .registers                   = aarch64_registers,
    .register_count              = sizeof(aarch64_registers) / sizeof(aarch64_registers[0]),

    .return_register             = { AARCH64_REG_X0, "x0" },
    .frame_pointer               = { AARCH64_REG_X29, "x29" },
    .stack_pointer               = { AARCH64_REG_SP, "sp" },
    .closure_env_register        = { AARCH64_REG_X28, "x28" },

    .work_register               = { AARCH64_REG_X16, "x16" },

    .arg_registers               = aarch64_arg_registers,
    .arg_register_count          = sizeof(aarch64_arg_registers) / sizeof(aarch64_arg_registers[0]),

    .allocatable_registers       = aarch64_allocatable_registers,
    .allocatable_register_count  = sizeof(aarch64_allocatable_registers) / sizeof(aarch64_allocatable_registers[0]),

    .callee_saved_registers      = aarch64_callee_saved_registers,
    .callee_saved_register_count = sizeof(aarch64_callee_saved_registers) / sizeof(aarch64_callee_saved_registers[0]),

    .linker_command              = "aarch64-linux-gnu-gcc",
    .linker_flags                = "-static"
};

const TargetDescriptor *target_get_aarch64_descriptor(void) {
    return &aarch64_aapcs_elf_descriptor;
}
