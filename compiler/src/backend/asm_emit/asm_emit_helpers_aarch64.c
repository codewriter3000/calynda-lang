#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  ARM64 mov helper: handles reg/imm/mem combinations                 */
/* ------------------------------------------------------------------ */

bool ae_arm64_emit_mov(FILE *out, AsmOperand *dest, AsmOperand *src) {
    /* reg ← reg */
    if (dest->kind == ASM_OPERAND_REGISTER && src->kind == ASM_OPERAND_REGISTER) {
        return ae_emit_line(out, "    mov %s, %s\n", dest->text, src->text);
    }
    /* reg ← immediate */
    if (dest->kind == ASM_OPERAND_REGISTER && src->kind == ASM_OPERAND_IMMEDIATE) {
        return ae_emit_line(out, "    mov %s, %s\n", dest->text, src->text);
    }
    /* reg ← memory (load) */
    if (dest->kind == ASM_OPERAND_REGISTER && src->kind == ASM_OPERAND_MEMORY) {
        return ae_emit_line(out, "    ldr %s, %s\n", dest->text, src->text);
    }
    /* memory ← reg (store) */
    if (dest->kind == ASM_OPERAND_MEMORY && src->kind == ASM_OPERAND_REGISTER) {
        return ae_emit_line(out, "    str %s, %s\n", src->text, dest->text);
    }
    /* memory ← immediate: use x17 as scratch */
    if (dest->kind == ASM_OPERAND_MEMORY && src->kind == ASM_OPERAND_IMMEDIATE) {
        return ae_emit_line(out, "    mov x17, %s\n", src->text) &&
               ae_emit_line(out, "    str x17, %s\n", dest->text);
    }
    /* memory ← memory: load into scratch, store */
    if (dest->kind == ASM_OPERAND_MEMORY && src->kind == ASM_OPERAND_MEMORY) {
        return ae_emit_line(out, "    ldr x17, %s\n", src->text) &&
               ae_emit_line(out, "    str x17, %s\n", dest->text);
    }
    /* reg/mem ← address: load address via adrp+add */
    if (src->kind == ASM_OPERAND_ADDRESS) {
        const char *dreg = (dest->kind == ASM_OPERAND_REGISTER) ? dest->text : "x17";
        bool ok = ae_emit_line(out, "    adrp %s, %s\n", dreg, src->text) &&
                  ae_emit_line(out, "    add %s, %s, :lo12:%s\n", dreg, dreg, src->text);

        if (ok && dest->kind == ASM_OPERAND_MEMORY) {
            ok = ae_emit_line(out, "    str x17, %s\n", dest->text);
        }
        return ok;
    }
    /* address ← reg/imm/mem: store to global (adrp+add to get address, then str) */
    if (dest->kind == ASM_OPERAND_ADDRESS) {
        const char *value_reg = "x17";
        bool ok;

        if (src->kind == ASM_OPERAND_REGISTER) {
            value_reg = src->text;
        } else if (src->kind == ASM_OPERAND_IMMEDIATE) {
            if (!ae_emit_line(out, "    mov x17, %s\n", src->text)) {
                return false;
            }
        } else if (src->kind == ASM_OPERAND_MEMORY) {
            if (!ae_emit_line(out, "    ldr x17, %s\n", src->text)) {
                return false;
            }
        } else if (src->kind == ASM_OPERAND_ADDRESS) {
            /* Load source global address, then load its value */
            if (!ae_emit_line(out, "    adrp x17, %s\n", src->text) ||
                !ae_emit_line(out, "    ldr x17, [x17, :lo12:%s]\n", src->text)) {
                return false;
            }
        } else {
            return false;
        }
        ok = ae_emit_line(out, "    adrp x16, %s\n", dest->text) &&
             ae_emit_line(out, "    str %s, [x16, :lo12:%s]\n", value_reg, dest->text);
        return ok;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/*  ARM64 3-operand ALU helper                                         */
/* ------------------------------------------------------------------ */

bool ae_arm64_emit_alu3(FILE *out,
                        const char *mnemonic,
                        AsmOperand *dest,
                        AsmOperand *src1,
                        AsmOperand *src2) {
    const char *d = dest->text;
    const char *s1 = src1->text;
    const char *s2 = src2->text;
    bool ok;

    /* Load memory operands into scratch registers */
    if (src1->kind == ASM_OPERAND_MEMORY) {
        if (!ae_emit_line(out, "    ldr x16, %s\n", src1->text)) {
            return false;
        }
        s1 = "x16";
    }
    if (src2->kind == ASM_OPERAND_MEMORY) {
        if (!ae_emit_line(out, "    ldr x17, %s\n", src2->text)) {
            return false;
        }
        s2 = "x17";
    }

    if (dest->kind == ASM_OPERAND_MEMORY) {
        ok = ae_emit_line(out, "    %s x16, %s, %s\n", mnemonic, s1, s2) &&
             ae_emit_line(out, "    str x16, %s\n", dest->text);
    } else {
        ok = ae_emit_line(out, "    %s %s, %s, %s\n", mnemonic, d, s1, s2);
    }
    return ok;
}

/* ------------------------------------------------------------------ */
/*  ARM64 two-operand instruction helper                               */
/* ------------------------------------------------------------------ */

bool ae_arm64_emit_two_op(FILE *out,
                          const char *mnemonic,
                          AsmOperand *left,
                          AsmOperand *right,
                          AsmEmitContext *context,
                          const MachineUnit *unit,
                          const AsmUnitLayout *layout) {
    (void)context;
    (void)unit;
    (void)layout;

    if (strcmp(mnemonic, "mov") == 0) {
        return ae_arm64_emit_mov(out, left, right);
    }
    if (strcmp(mnemonic, "cmp") == 0) {
        /* cmp Xn, operand — both must be registers */
        if (left->kind == ASM_OPERAND_MEMORY) {
            if (!ae_emit_line(out, "    ldr x16, %s\n", left->text)) {
                return false;
            }
            if (right->kind == ASM_OPERAND_MEMORY) {
                return ae_emit_line(out, "    ldr x17, %s\n", right->text) &&
                       ae_emit_line(out, "    cmp x16, x17\n");
            }
            return ae_emit_line(out, "    cmp x16, %s\n", right->text);
        }
        if (right->kind == ASM_OPERAND_MEMORY) {
            return ae_emit_line(out, "    ldr x17, %s\n", right->text) &&
                   ae_emit_line(out, "    cmp %s, x17\n", left->text);
        }
        return ae_emit_line(out, "    cmp %s, %s\n", left->text, right->text);
    }
    if (strcmp(mnemonic, "cset") == 0) {
        /* cset Xd, cc — right operand is a condition code name */
        return ae_emit_line(out, "    cset %s, %s\n", left->text, right->text);
    }
    if (strcmp(mnemonic, "neg") == 0 || strcmp(mnemonic, "mvn") == 0) {
        /* neg Xd, Xn / mvn Xd, Xn */
        const char *src = right->text;

        if (right->kind == ASM_OPERAND_MEMORY) {
            if (!ae_emit_line(out, "    ldr x17, %s\n", right->text)) {
                return false;
            }
            src = "x17";
        }
        if (left->kind == ASM_OPERAND_MEMORY) {
            return ae_emit_line(out, "    %s x16, %s\n", mnemonic, src) &&
                   ae_emit_line(out, "    str x16, %s\n", left->text);
        }
        return ae_emit_line(out, "    %s %s, %s\n", mnemonic, left->text, src);
    }
    if (strcmp(mnemonic, "lea") == 0) {
        /* lea Xd, frame/spill/helper → compute address using sub from FP */
        if (right->kind == ASM_OPERAND_MEMORY) {
            /* Parse [x29, #-N] to extract offset */
            const char *hash = strchr(right->text, '#');

            if (hash && hash[1] == '-') {
                char *offset_str = ae_copy_text(hash + 2);

                if (offset_str) {
                    bool ok;
                    size_t len = strlen(offset_str);

                    if (len > 0 && offset_str[len - 1] == ']') {
                        offset_str[len - 1] = '\0';
                    }
                    if (left->kind == ASM_OPERAND_MEMORY) {
                        ok = ae_emit_line(out, "    sub x16, x29, #%s\n", offset_str) &&
                             ae_emit_line(out, "    str x16, %s\n", left->text);
                    } else {
                        ok = ae_emit_line(out, "    sub %s, x29, #%s\n",
                                          left->text, offset_str);
                    }
                    free(offset_str);
                    return ok;
                }
            }
        }
        /* Fallback: mov */
        return ae_arm64_emit_mov(out, left, right);
    }
    /* Generic two-operand fallback */
    return ae_emit_line(out, "    %s %s, %s\n", mnemonic, left->text, right->text);
}
