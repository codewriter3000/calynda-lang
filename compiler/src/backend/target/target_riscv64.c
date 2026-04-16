#include "target.h"

/*
 * RISC-V 64 (RV64GC) LP64D ELF register IDs.
 * Register mapping follows the standard RISC-V calling convention.
 */
enum {
    RV64_REG_ZERO = 0,   /* x0  — hardwired zero            */
    RV64_REG_RA,          /* x1  — return address             */
    RV64_REG_SP,          /* x2  — stack pointer              */
    RV64_REG_GP,          /* x3  — global pointer             */
    RV64_REG_TP,          /* x4  — thread pointer             */
    RV64_REG_T0,          /* x5  — temporary / alt link       */
    RV64_REG_T1,          /* x6  — temporary                  */
    RV64_REG_T2,          /* x7  — temporary                  */
    RV64_REG_S0,          /* x8  — saved register / FP        */
    RV64_REG_S1,          /* x9  — saved register             */
    RV64_REG_A0,          /* x10 — argument / return value    */
    RV64_REG_A1,          /* x11 — argument / return value    */
    RV64_REG_A2,          /* x12 — argument                   */
    RV64_REG_A3,          /* x13 — argument                   */
    RV64_REG_A4,          /* x14 — argument                   */
    RV64_REG_A5,          /* x15 — argument                   */
    RV64_REG_A6,          /* x16 — argument                   */
    RV64_REG_A7,          /* x17 — argument                   */
    RV64_REG_S2,          /* x18 — saved register             */
    RV64_REG_S3,          /* x19 — saved register             */
    RV64_REG_S4,          /* x20 — saved register             */
    RV64_REG_S5,          /* x21 — saved register             */
    RV64_REG_S6,          /* x22 — saved register             */
    RV64_REG_S7,          /* x23 — saved register             */
    RV64_REG_S8,          /* x24 — saved register             */
    RV64_REG_S9,          /* x25 — saved register             */
    RV64_REG_S10,         /* x26 — saved register             */
    RV64_REG_S11,         /* x27 — saved register             */
    RV64_REG_T3,          /* x28 — temporary                  */
    RV64_REG_T4,          /* x29 — temporary                  */
    RV64_REG_T5,          /* x30 — temporary                  */
    RV64_REG_T6           /* x31 — temporary                  */
};

static const TargetRegister rv64_registers[] = {
    { RV64_REG_ZERO, "zero" },
    { RV64_REG_RA,   "ra"   },
    { RV64_REG_SP,   "sp"   },
    { RV64_REG_GP,   "gp"   },
    { RV64_REG_TP,   "tp"   },
    { RV64_REG_T0,   "t0"   },
    { RV64_REG_T1,   "t1"   },
    { RV64_REG_T2,   "t2"   },
    { RV64_REG_S0,   "s0"   },
    { RV64_REG_S1,   "s1"   },
    { RV64_REG_A0,   "a0"   },
    { RV64_REG_A1,   "a1"   },
    { RV64_REG_A2,   "a2"   },
    { RV64_REG_A3,   "a3"   },
    { RV64_REG_A4,   "a4"   },
    { RV64_REG_A5,   "a5"   },
    { RV64_REG_A6,   "a6"   },
    { RV64_REG_A7,   "a7"   },
    { RV64_REG_S2,   "s2"   },
    { RV64_REG_S3,   "s3"   },
    { RV64_REG_S4,   "s4"   },
    { RV64_REG_S5,   "s5"   },
    { RV64_REG_S6,   "s6"   },
    { RV64_REG_S7,   "s7"   },
    { RV64_REG_S8,   "s8"   },
    { RV64_REG_S9,   "s9"   },
    { RV64_REG_S10,  "s10"  },
    { RV64_REG_S11,  "s11"  },
    { RV64_REG_T3,   "t3"   },
    { RV64_REG_T4,   "t4"   },
    { RV64_REG_T5,   "t5"   },
    { RV64_REG_T6,   "t6"   }
};

/* LP64D: a0-a7 for integer argument passing */
static const TargetRegister rv64_arg_registers[] = {
    { RV64_REG_A0, "a0" },
    { RV64_REG_A1, "a1" },
    { RV64_REG_A2, "a2" },
    { RV64_REG_A3, "a3" },
    { RV64_REG_A4, "a4" },
    { RV64_REG_A5, "a5" },
    { RV64_REG_A6, "a6" },
    { RV64_REG_A7, "a7" }
};

/* Caller-saved temporaries used for vreg allocation */
static const TargetRegister rv64_allocatable_registers[] = {
    { RV64_REG_T1, "t1" },
    { RV64_REG_T2, "t2" },
    { RV64_REG_T3, "t3" },
    { RV64_REG_T4, "t4" },
    { RV64_REG_T5, "t5" },
    { RV64_REG_T6, "t6" }
};

/* LP64D callee-saved: s0-s11, ra */
static const TargetRegister rv64_callee_saved_registers[] = {
    { RV64_REG_RA,  "ra"  },
    { RV64_REG_S0,  "s0"  },
    { RV64_REG_S1,  "s1"  },
    { RV64_REG_S2,  "s2"  },
    { RV64_REG_S3,  "s3"  },
    { RV64_REG_S4,  "s4"  },
    { RV64_REG_S5,  "s5"  },
    { RV64_REG_S6,  "s6"  },
    { RV64_REG_S7,  "s7"  },
    { RV64_REG_S8,  "s8"  },
    { RV64_REG_S9,  "s9"  },
    { RV64_REG_S10, "s10" },
    { RV64_REG_S11, "s11" }
};

static const TargetDescriptor rv64_lp64d_elf_descriptor = {
    .kind                        = TARGET_KIND_RISCV64_LP64D_ELF,
    .name                        = "riscv64_lp64d_elf",
    .asm_syntax                  = TARGET_ASM_SYNTAX_GNU_RISCV64,
    .word_size                   = 8,
    .stack_alignment             = 16,

    .registers                   = rv64_registers,
    .register_count              = sizeof(rv64_registers) / sizeof(rv64_registers[0]),

    .return_register             = { RV64_REG_A0, "a0" },
    .frame_pointer               = { RV64_REG_S0, "s0" },
    .stack_pointer               = { RV64_REG_SP, "sp" },
    .closure_env_register        = { RV64_REG_S11, "s11" },

    .work_register               = { RV64_REG_T0, "t0" },

    .arg_registers               = rv64_arg_registers,
    .arg_register_count          = sizeof(rv64_arg_registers) / sizeof(rv64_arg_registers[0]),

    .allocatable_registers       = rv64_allocatable_registers,
    .allocatable_register_count  = sizeof(rv64_allocatable_registers) / sizeof(rv64_allocatable_registers[0]),

    .callee_saved_registers      = rv64_callee_saved_registers,
    .callee_saved_register_count = sizeof(rv64_callee_saved_registers) / sizeof(rv64_callee_saved_registers[0]),

    .linker_command              = "riscv64-linux-gnu-gcc",
    .linker_flags                = "-static"
};

const TargetDescriptor *target_get_riscv64_descriptor(void) {
    return &rv64_lp64d_elf_descriptor;
}
