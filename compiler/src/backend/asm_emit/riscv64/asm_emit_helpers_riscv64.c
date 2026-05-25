#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

static bool ae_rv64_immediate_fits_i12(long long value) {
    return value >= -2048 && value <= 2047;
}

static const char *ae_rv64_pick_scratch(const char *avoid1,
                                        const char *avoid2,
                                        const char *avoid3) {
    static const char *candidates[] = {"t6", "t0", "s1"};
    size_t index;

    for (index = 0; index < sizeof(candidates) / sizeof(candidates[0]); index++) {
        const char *candidate = candidates[index];

        if ((avoid1 && strcmp(candidate, avoid1) == 0) ||
            (avoid2 && strcmp(candidate, avoid2) == 0) ||
            (avoid3 && strcmp(candidate, avoid3) == 0)) {
            continue;
        }
        return candidate;
    }

    return NULL;
}

static bool ae_rv64_parse_memory_operand(const char *memory_text,
                                         long long *offset_out,
                                         char **base_reg_out) {
    const char *paren;
    const char *close_paren;
    char *offset_text;
    char *end_ptr;
    long long offset;

    if (!memory_text || !offset_out || !base_reg_out) {
        return false;
    }

    paren = strchr(memory_text, '(');
    close_paren = paren ? strchr(paren + 1, ')') : NULL;
    if (!paren || !close_paren || close_paren[1] != '\0') {
        return false;
    }

    offset_text = ae_copy_text_n(memory_text, (size_t)(paren - memory_text));
    if (!offset_text) {
        return false;
    }

    end_ptr = NULL;
    offset = strtoll(offset_text, &end_ptr, 10);
    if (!end_ptr || *end_ptr != '\0') {
        free(offset_text);
        return false;
    }

    *base_reg_out = ae_copy_text_n(paren + 1, (size_t)(close_paren - paren - 1));
    free(offset_text);
    if (!*base_reg_out) {
        return false;
    }

    *offset_out = offset;
    return true;
}

static bool ae_rv64_emit_immediate_op(FILE *out,
                                      const char *mnemonic,
                                      const char *dest_reg,
                                      const char *src_reg,
                                      const char *immediate_text) {
    char *end_ptr = NULL;
    long long immediate = 0;
    const char *scratch_reg;
    const char *register_mnemonic;

    if (!out || !mnemonic || !dest_reg || !src_reg || !immediate_text) {
        return false;
    }

    immediate = strtoll(immediate_text, &end_ptr, 10);
    if (end_ptr && *end_ptr == '\0' && ae_rv64_immediate_fits_i12(immediate)) {
        return ae_emit_line(out,
                            "    %s %s, %s, %s\n",
                            mnemonic,
                            dest_reg,
                            src_reg,
                            immediate_text);
    }

    if (strcmp(mnemonic, "addi") == 0) {
        register_mnemonic = "add";
    } else if (strcmp(mnemonic, "xori") == 0) {
        register_mnemonic = "xor";
    } else {
        return false;
    }

    scratch_reg = ae_rv64_pick_scratch(dest_reg, src_reg, NULL);
    if (!scratch_reg) {
        return false;
    }

    return ae_emit_line(out, "    li %s, %s\n", scratch_reg, immediate_text) &&
           ae_emit_line(out,
                        "    %s %s, %s, %s\n",
                        register_mnemonic,
                        dest_reg,
                        src_reg,
                        scratch_reg);
}

bool ae_rv64_emit_address(FILE *out, const char *dest_reg, const char *memory_text) {
    long long offset;
    char *base_reg = NULL;
    const char *scratch_reg;
    bool ok;

    if (!out || !dest_reg || !memory_text) {
        return false;
    }

    if (!ae_rv64_parse_memory_operand(memory_text, &offset, &base_reg)) {
        return false;
    }

    if (ae_rv64_immediate_fits_i12(offset)) {
        ok = ae_emit_line(out, "    addi %s, %s, %lld\n", dest_reg, base_reg, offset);
        free(base_reg);
        return ok;
    }

    scratch_reg = ae_rv64_pick_scratch(dest_reg, base_reg, NULL);
    if (!scratch_reg) {
        free(base_reg);
        return false;
    }

    ok = ae_emit_line(out, "    li %s, %lld\n", scratch_reg, offset) &&
         ae_emit_line(out, "    add %s, %s, %s\n", dest_reg, base_reg, scratch_reg);
    free(base_reg);
    return ok;
}

bool ae_rv64_emit_load(FILE *out, const char *dest_reg, const char *memory_text) {
    long long offset;
    char *base_reg = NULL;
    const char *scratch_reg;
    bool ok;

    if (!out || !dest_reg || !memory_text) {
        return false;
    }

    if (!ae_rv64_parse_memory_operand(memory_text, &offset, &base_reg) ||
        ae_rv64_immediate_fits_i12(offset)) {
        free(base_reg);
        return ae_emit_line(out, "    ld %s, %s\n", dest_reg, memory_text);
    }

    scratch_reg = ae_rv64_pick_scratch(dest_reg, base_reg, NULL);
    if (!scratch_reg) {
        free(base_reg);
        return false;
    }

    ok = ae_emit_line(out, "    li %s, %lld\n", scratch_reg, offset) &&
         ae_emit_line(out, "    add %s, %s, %s\n", scratch_reg, base_reg, scratch_reg) &&
         ae_emit_line(out, "    ld %s, 0(%s)\n", dest_reg, scratch_reg);
    free(base_reg);
    return ok;
}

bool ae_rv64_emit_store(FILE *out, const char *src_reg, const char *memory_text) {
    long long offset;
    char *base_reg = NULL;
    const char *scratch_reg;
    bool ok;

    if (!out || !src_reg || !memory_text) {
        return false;
    }

    if (!ae_rv64_parse_memory_operand(memory_text, &offset, &base_reg) ||
        ae_rv64_immediate_fits_i12(offset)) {
        free(base_reg);
        return ae_emit_line(out, "    sd %s, %s\n", src_reg, memory_text);
    }

    scratch_reg = ae_rv64_pick_scratch(src_reg, base_reg, NULL);
    if (!scratch_reg) {
        free(base_reg);
        return false;
    }

    ok = ae_emit_line(out, "    li %s, %lld\n", scratch_reg, offset) &&
         ae_emit_line(out, "    add %s, %s, %s\n", scratch_reg, base_reg, scratch_reg) &&
         ae_emit_line(out, "    sd %s, 0(%s)\n", src_reg, scratch_reg);
    free(base_reg);
    return ok;
}

bool ae_rv64_emit_stack_adjust(FILE *out, long long delta) {
    if (!out) {
        return false;
    }

    if (delta == 0) {
        return true;
    }

    if (ae_rv64_immediate_fits_i12(delta)) {
        return ae_emit_line(out, "    addi sp, sp, %lld\n", delta);
    }

    return ae_emit_line(out, "    li t6, %lld\n", delta) &&
           ae_emit_line(out, "    add sp, sp, t6\n");
}

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
        return ae_rv64_emit_load(out, dest->text, src->text);
    }
    /* memory ← reg (store) */
    if (dest->kind == ASM_OPERAND_MEMORY && src->kind == ASM_OPERAND_REGISTER) {
        return ae_rv64_emit_store(out, src->text, dest->text);
    }
    /* memory ← immediate: use t0 as scratch */
    if (dest->kind == ASM_OPERAND_MEMORY && src->kind == ASM_OPERAND_IMMEDIATE) {
        return ae_emit_line(out, "    li t0, %s\n", src->text) &&
               ae_rv64_emit_store(out, "t0", dest->text);
    }
    /* memory ← memory: load into scratch, store */
    if (dest->kind == ASM_OPERAND_MEMORY && src->kind == ASM_OPERAND_MEMORY) {
        return ae_rv64_emit_load(out, "t0", src->text) &&
               ae_rv64_emit_store(out, "t0", dest->text);
    }
    /* reg/mem ← address: load address via la pseudo */
    if (src->kind == ASM_OPERAND_ADDRESS) {
        const char *dreg = (dest->kind == ASM_OPERAND_REGISTER) ? dest->text : "t0";
        bool ok = ae_emit_line(out, "    la %s, %s\n", dreg, src->text);

        if (ok && dest->kind == ASM_OPERAND_MEMORY) {
            ok = ae_rv64_emit_store(out, "t0", dest->text);
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
            if (!ae_rv64_emit_load(out, "s1", src->text)) {
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
        if (!ae_rv64_emit_load(out, "t0", src1->text)) {
            return false;
        }
        s1 = "t0";
    }
    if (src2->kind == ASM_OPERAND_MEMORY) {
        if (!ae_rv64_emit_load(out, "s1", src2->text)) {
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
             ae_rv64_emit_store(out, "t0", dest->text);
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
            if (!ae_rv64_emit_load(out, "t0", right->text)) {
                return false;
            }
            src = "t0";
        }
        if (left->kind == ASM_OPERAND_MEMORY) {
            return ae_emit_line(out, "    %s t0, %s\n", mnemonic, src) &&
                   ae_rv64_emit_store(out, "t0", left->text);
        }
        return ae_emit_line(out, "    %s %s, %s\n", mnemonic, left->text, src);
    }
    /* xori rd, rs, imm — used for boolean inversion */
    if (strcmp(mnemonic, "xori") == 0 || strcmp(mnemonic, "addi") == 0) {
        const char *src = left->text;
        const char *dest = left->text;

        if (left->kind == ASM_OPERAND_MEMORY) {
            if (!ae_rv64_emit_load(out, "t0", left->text)) {
                return false;
            }
            src = "t0";
            dest = "t0";
        }
        if (!ae_rv64_emit_immediate_op(out, mnemonic, dest, src, right->text)) {
            return false;
        }
        if (left->kind == ASM_OPERAND_MEMORY) {
            return ae_rv64_emit_store(out, "t0", left->text);
        }
        return true;
    }
    if (strcmp(mnemonic, "lea") == 0) {
        /* lea Xd, frame/spill/helper → compute address using sub from FP */
        if (right->kind == ASM_OPERAND_MEMORY) {
            if (left->kind == ASM_OPERAND_MEMORY) {
                return ae_rv64_emit_address(out, "t6", right->text) &&
                       ae_rv64_emit_store(out, "t6", left->text);
            }
            return ae_rv64_emit_address(out, left->text, right->text);
        }
        /* Fallback: mov */
        return ae_rv64_emit_mov(out, left, right);
    }
    /* mvn → not (ARM64 mnemonic remapping) */
    if (strcmp(mnemonic, "mvn") == 0) {
        const char *src = right->text;

        if (right->kind == ASM_OPERAND_MEMORY) {
            if (!ae_rv64_emit_load(out, "t0", right->text)) {
                return false;
            }
            src = "t0";
        }
        if (left->kind == ASM_OPERAND_MEMORY) {
            return ae_emit_line(out, "    not t0, %s\n", src) &&
                   ae_rv64_emit_store(out, "t0", left->text);
        }
        return ae_emit_line(out, "    not %s, %s\n", left->text, src);
    }
    /* Generic two-operand fallback */
    return ae_emit_line(out, "    %s %s, %s\n", mnemonic, left->text, right->text);
}
