#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  RV64 mov helper: handles reg/imm/mem/addr combinations             */
/* ------------------------------------------------------------------ */

bool ae_rv64_emit_mov(FILE *out, AsmOperand *dest, AsmOperand *src) {
    /* reg ← reg */
    if (dest->kind == ASM_OPERAND_REGISTER && src->kind == ASM_OPERAND_REGISTER) {
        return ae_emit_line(out, "    mv %s, %s\n", dest->text, src->text);
    }
    /* reg ← immediate */
    if (dest->kind == ASM_OPERAND_REGISTER && src->kind == ASM_OPERAND_IMMEDIATE) {
        return ae_emit_line(out, "    li %s, %s\n", dest->text, src->text);
    }
    /* reg ← memory (load) */
    if (dest->kind == ASM_OPERAND_REGISTER && src->kind == ASM_OPERAND_MEMORY) {
        return ae_emit_line(out, "    ld %s, %s\n", dest->text, src->text);
    }
    /* memory ← reg (store) */
    if (dest->kind == ASM_OPERAND_MEMORY && src->kind == ASM_OPERAND_REGISTER) {
        return ae_emit_line(out, "    sd %s, %s\n", src->text, dest->text);
    }
    /* memory ← immediate: use t0 as scratch */
    if (dest->kind == ASM_OPERAND_MEMORY && src->kind == ASM_OPERAND_IMMEDIATE) {
        return ae_emit_line(out, "    li t0, %s\n", src->text) &&
               ae_emit_line(out, "    sd t0, %s\n", dest->text);
    }
    /* memory ← memory: load into scratch, store */
    if (dest->kind == ASM_OPERAND_MEMORY && src->kind == ASM_OPERAND_MEMORY) {
        return ae_emit_line(out, "    ld t0, %s\n", src->text) &&
               ae_emit_line(out, "    sd t0, %s\n", dest->text);
    }
    /* reg/mem ← address: load address via la pseudo */
    if (src->kind == ASM_OPERAND_ADDRESS) {
        const char *dreg = (dest->kind == ASM_OPERAND_REGISTER) ? dest->text : "t0";
        bool ok = ae_emit_line(out, "    la %s, %s\n", dreg, src->text);

        if (ok && dest->kind == ASM_OPERAND_MEMORY) {
            ok = ae_emit_line(out, "    sd t0, %s\n", dest->text);
        }
        return ok;
    }
    /* address ← reg/imm/mem: store to global (la to get address, then sd) */
    if (dest->kind == ASM_OPERAND_ADDRESS) {
        const char *value_reg = "s1";
        bool ok;

        if (src->kind == ASM_OPERAND_REGISTER) {
            value_reg = src->text;
        } else if (src->kind == ASM_OPERAND_IMMEDIATE) {
            if (!ae_emit_line(out, "    li s1, %s\n", src->text)) {
                return false;
            }
        } else if (src->kind == ASM_OPERAND_MEMORY) {
            if (!ae_emit_line(out, "    ld s1, %s\n", src->text)) {
                return false;
            }
        } else if (src->kind == ASM_OPERAND_ADDRESS) {
            if (!ae_emit_line(out, "    la s1, %s\n", src->text) ||
                !ae_emit_line(out, "    ld s1, 0(s1)\n")) {
                return false;
            }
        } else {
            return false;
        }
        ok = ae_emit_line(out, "    la t0, %s\n", dest->text) &&
             ae_emit_line(out, "    sd %s, 0(t0)\n", value_reg);
        return ok;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/*  RV64 3-operand ALU helper                                          */
/* ------------------------------------------------------------------ */

bool ae_rv64_emit_alu3(FILE *out, const char *mnemonic,
                       AsmOperand *dest, AsmOperand *src1, AsmOperand *src2) {
    const char *d = dest->text;
    const char *s1 = src1->text;
    const char *s2 = src2->text;
    bool ok;

    /* Load memory operands into scratch registers */
    if (src1->kind == ASM_OPERAND_MEMORY) {
        if (!ae_emit_line(out, "    ld t0, %s\n", src1->text)) {
            return false;
        }
        s1 = "t0";
    }
    if (src2->kind == ASM_OPERAND_MEMORY) {
        if (!ae_emit_line(out, "    ld s1, %s\n", src2->text)) {
            return false;
        }
        s2 = "s1";
    }
    /* Handle immediate src2 for instructions that need register operands */
    if (src2->kind == ASM_OPERAND_IMMEDIATE) {
        if (!ae_emit_line(out, "    li s1, %s\n", src2->text)) {
            return false;
        }
        s2 = "s1";
    }
    if (src1->kind == ASM_OPERAND_IMMEDIATE) {
        if (!ae_emit_line(out, "    li t0, %s\n", src1->text)) {
            return false;
        }
        s1 = "t0";
    }

    if (dest->kind == ASM_OPERAND_MEMORY) {
        ok = ae_emit_line(out, "    %s t0, %s, %s\n", mnemonic, s1, s2) &&
             ae_emit_line(out, "    sd t0, %s\n", dest->text);
    } else {
        ok = ae_emit_line(out, "    %s %s, %s, %s\n", mnemonic, d, s1, s2);
    }
    return ok;
}

/* ------------------------------------------------------------------ */
/*  RV64 two-operand instruction helper                                */
/* ------------------------------------------------------------------ */

bool ae_rv64_emit_two_op(FILE *out, const char *mnemonic,
                         AsmOperand *left, AsmOperand *right,
                         AsmEmitContext *context, const MachineUnit *unit,
                         const AsmUnitLayout *layout) {
    (void)context;
    (void)unit;
    (void)layout;

    if (strcmp(mnemonic, "mov") == 0) {
        return ae_rv64_emit_mov(out, left, right);
    }
    /* neg Xd, Xn / not Xd, Xn — two-operand form */
    if (strcmp(mnemonic, "neg") == 0 || strcmp(mnemonic, "not") == 0 ||
        strcmp(mnemonic, "seqz") == 0 || strcmp(mnemonic, "snez") == 0) {
        const char *src = right->text;

        if (right->kind == ASM_OPERAND_MEMORY) {
            if (!ae_emit_line(out, "    ld t0, %s\n", right->text)) {
                return false;
            }
            src = "t0";
        }
        if (left->kind == ASM_OPERAND_MEMORY) {
            return ae_emit_line(out, "    %s t0, %s\n", mnemonic, src) &&
                   ae_emit_line(out, "    sd t0, %s\n", left->text);
        }
        return ae_emit_line(out, "    %s %s, %s\n", mnemonic, left->text, src);
    }
    /* xori rd, rs, imm — used for boolean inversion */
    if (strcmp(mnemonic, "xori") == 0 || strcmp(mnemonic, "addi") == 0) {
        const char *src = left->text;

        if (left->kind == ASM_OPERAND_MEMORY) {
            if (!ae_emit_line(out, "    ld t0, %s\n", left->text)) {
                return false;
            }
            src = "t0";
        }
        if (left->kind == ASM_OPERAND_MEMORY) {
            return ae_emit_line(out, "    %s t0, t0, %s\n", mnemonic, right->text) &&
                   ae_emit_line(out, "    sd t0, %s\n", left->text);
        }
        return ae_emit_line(out, "    %s %s, %s, %s\n",
                            mnemonic, src, src, right->text);
    }
    if (strcmp(mnemonic, "lea") == 0) {
        /* lea Xd, frame/spill/helper → compute address using sub from FP */
        if (right->kind == ASM_OPERAND_MEMORY) {
            /* Parse -N(s0) to extract offset */
            if (right->text[0] == '-') {
                char *paren = strchr(right->text, '(');

                if (paren) {
                    char *offset_str = ae_copy_text_n(right->text + 1,
                                                      (size_t)(paren - right->text - 1));

                    if (offset_str) {
                        bool ok;

                        if (left->kind == ASM_OPERAND_MEMORY) {
                            ok = ae_emit_line(out, "    addi t0, s0, -%s\n", offset_str) &&
                                 ae_emit_line(out, "    sd t0, %s\n", left->text);
                        } else {
                            ok = ae_emit_line(out, "    addi %s, s0, -%s\n",
                                              left->text, offset_str);
                        }
                        free(offset_str);
                        return ok;
                    }
                }
            }
        }
        /* Fallback: mov */
        return ae_rv64_emit_mov(out, left, right);
    }
    /* mvn → not (ARM64 mnemonic remapping) */
    if (strcmp(mnemonic, "mvn") == 0) {
        const char *src = right->text;

        if (right->kind == ASM_OPERAND_MEMORY) {
            if (!ae_emit_line(out, "    ld t0, %s\n", right->text)) {
                return false;
            }
            src = "t0";
        }
        if (left->kind == ASM_OPERAND_MEMORY) {
            return ae_emit_line(out, "    not t0, %s\n", src) &&
                   ae_emit_line(out, "    sd t0, %s\n", left->text);
        }
        return ae_emit_line(out, "    not %s, %s\n", left->text, src);
    }
    /* Generic two-operand fallback */
    return ae_emit_line(out, "    %s %s, %s\n", mnemonic, left->text, right->text);
}
