#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations for helpers in other riscv64 files. */
int ae_rv64_preserve_index(const char *reg_name);
bool ae_rv64_emit_mov(FILE *out, AsmOperand *dest, AsmOperand *src);
bool ae_rv64_emit_alu3(FILE *out, const char *mnemonic,
                       AsmOperand *dest, AsmOperand *src1, AsmOperand *src2);
bool ae_rv64_emit_two_op(FILE *out, const char *mnemonic, AsmOperand *left,
                         AsmOperand *right, AsmEmitContext *context,
                         const MachineUnit *unit, const AsmUnitLayout *layout);

/* ---- RV64 mnemonic remapping from ARM64-style text ---- */
static const char *ae_rv64_remap_mnemonic(const char *m) {
    if (strcmp(m, "orr") == 0) return "or";
    if (strcmp(m, "eor") == 0) return "xor";
    if (strcmp(m, "lsl") == 0) return "sll";
    if (strcmp(m, "lsr") == 0) return "srl";
    if (strcmp(m, "asr") == 0) return "sra";
    if (strcmp(m, "sdiv") == 0) return "div";
    if (strcmp(m, "udiv") == 0) return "divu";
    return m;
}

/* ------------------------------------------------------------------ */
/*  RV64 instruction emission                                          */
/* ------------------------------------------------------------------ */

bool ae_emit_machine_instruction_riscv64(AsmEmitContext *context,
                                         FILE *out,
                                         const MachineUnit *unit,
                                         const AsmUnitLayout *layout,
                                         size_t unit_index,
                                         size_t block_index,
                                         size_t instruction_index,
                                         const char *instruction_text) {
    char *space;
    char *mnemonic = NULL, *operand_text = NULL;
    char *left_text = NULL, *right_text = NULL, *third_text = NULL;
    AsmOperand left, right, third;
    bool ok = false;
    size_t frame_size;

    (void)unit_index; (void)block_index; (void)instruction_index;
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    memset(&third, 0, sizeof(third));

    frame_size = (layout->saved_reg_words + layout->total_local_words) * 8;

    /* Skip prologue instructions (emitted by ae_emit_unit_text). */
    if (strcmp(instruction_text, "push s0") == 0 ||
        strcmp(instruction_text, "mov s0, sp") == 0 ||
        strcmp(instruction_text, "push t0") == 0 ||
        strcmp(instruction_text, "push s1") == 0 ||
        ae_starts_with(instruction_text, "sub sp, ") ||
        ae_starts_with(instruction_text, "add sp, ") ||
        strcmp(instruction_text, "pop t0") == 0 ||
        strcmp(instruction_text, "pop s1") == 0 ||
        strcmp(instruction_text, "pop s0") == 0) {
        return true;
    }

    /* Caller-saved register preserve: push tN → sd tN, -offset(s0) */
    if (ae_starts_with(instruction_text, "push ")) {
        const char *reg = instruction_text + 5;
        int pi = ae_rv64_preserve_index(reg);

        if (pi >= 0) {
            return ae_emit_line(out, "    sd %s, -%zu(s0)\n",
                                reg,
                                ae_call_preserve_offset(layout, unit, (size_t)pi));
        }
        return true;
    }
    if (ae_starts_with(instruction_text, "pop ")) {
        const char *reg = instruction_text + 4;
        int pi = ae_rv64_preserve_index(reg);

        if (pi >= 0) {
            return ae_emit_line(out, "    ld %s, -%zu(s0)\n",
                                reg,
                                ae_call_preserve_offset(layout, unit, (size_t)pi));
        }
        return true;
    }

    /* ret → full epilogue */
    if (strcmp(instruction_text, "ret") == 0) {
        if (frame_size > 0 && !ae_emit_line(out, "    addi sp, sp, %zu\n", frame_size))
            return false;
        return ae_emit_line(out, "    ld t0, -24(s0)\n    ld s1, -32(s0)\n"
                       "    ld s0, 0(sp)\n    ld ra, 8(sp)\n"
                       "    addi sp, sp, 16\n    ret\n");
    }

    /* Parse mnemonic + operands */
    space = strchr(instruction_text, ' ');
    if (!space) return false;
    mnemonic = ae_copy_text_n(instruction_text, (size_t)(space - instruction_text));
    operand_text = ae_trim_copy(space + 1);
    if (!mnemonic || !operand_text) { free(mnemonic); free(operand_text); return false; }

    /* Branch / call / jump instructions (single label operand) */
    if (strcmp(mnemonic, "call") == 0 || strcmp(mnemonic, "j") == 0) {
        if (!ae_translate_operand_riscv64(context, unit, layout,
                                          operand_text, false, &left)) {
            goto fail;
        }
        ok = ae_emit_line(out, "    %s %s\n", mnemonic, left.text);
        goto cleanup;
    }

    /* bne cond, zero, label — three-operand branch */
    if (strcmp(mnemonic, "bne") == 0 || strcmp(mnemonic, "beq") == 0) {
        /* Parse "cond, zero, label" */
        char *first_comma = strchr(operand_text, ',');

        if (first_comma) {
            char *rest = ae_trim_copy(first_comma + 1);
            char *second_comma = rest ? strchr(rest, ',') : NULL;

            if (second_comma) {
                char *cond_text = ae_copy_text_n(operand_text,
                                                  (size_t)(first_comma - operand_text));
                char *zero_text = ae_copy_text_n(rest,
                                                  (size_t)(second_comma - rest));
                char *label_text = ae_trim_copy(second_comma + 1);
                AsmOperand cond_op, label_op;
                char *zt = ae_trim_copy(zero_text);

                memset(&cond_op, 0, sizeof(cond_op));
                memset(&label_op, 0, sizeof(label_op));

                if (cond_text && zt && label_text &&
                    ae_translate_operand_riscv64(context, unit, layout,
                                                 ae_trim_copy(cond_text), false, &cond_op) &&
                    ae_translate_operand_riscv64(context, unit, layout,
                                                 label_text, false, &label_op)) {
                    ok = ae_emit_line(out, "    %s %s, %s, %s\n",
                                      mnemonic, cond_op.text, zt, label_op.text);
                }
                ae_free_operand(&cond_op);
                ae_free_operand(&label_op);
                free(cond_text);
                free(zero_text);
                free(zt);
                free(label_text);
            }
            free(rest);
        }
        goto cleanup;
    }

    /* No comma → single-operand instruction */
    if (!strchr(operand_text, ',')) {
        goto fail;
    }

    /* Split operands at commas */
    {
        char *first_comma = strchr(operand_text, ',');
        char *rest;

        left_text = ae_copy_text_n(operand_text,
                                   (size_t)(first_comma - operand_text));
        rest = ae_trim_copy(first_comma + 1);
        if (!left_text || !rest) { free(rest); goto fail; }
        {
            char *second_comma = strchr(rest, ',');

            if (second_comma) {
                right_text = ae_copy_text_n(rest,
                                            (size_t)(second_comma - rest));
                third_text = ae_trim_copy(second_comma + 1);
            } else {
                right_text = ae_copy_text(rest);
                third_text = NULL;
            }
        }
        free(rest);
    }

    /* Trim whitespace */
    { char *t; t = ae_trim_copy(left_text); free(left_text); left_text = t;
      t = ae_trim_copy(right_text); free(right_text); right_text = t;
      if (third_text) { t = ae_trim_copy(third_text); free(third_text); third_text = t; }
    }

    /* Translate first two operands */
    if (!ae_translate_operand_riscv64(context, unit, layout,
                                      left_text, true, &left) ||
        !ae_translate_operand_riscv64(context, unit, layout,
                                      right_text, false, &right)) {
        goto fail;
    }

    /* ---- Two-operand instructions ---- */
    if (!third_text) {
        ok = ae_rv64_emit_two_op(out, mnemonic, &left, &right,
                                 context, unit, layout);
        goto cleanup;
    }

    /* Translate third operand */
    if (!ae_translate_operand_riscv64(context, unit, layout,
                                      third_text, false, &third)) {
        goto fail;
    }

    /* Remap ARM64-style mnemonics to RV64 equivalents */
    {
        const char *rv_mnemonic = ae_rv64_remap_mnemonic(mnemonic);

        /* ---- Three-operand ALU instructions ---- */
        if (strcmp(rv_mnemonic, "add") == 0 || strcmp(rv_mnemonic, "sub") == 0 ||
            strcmp(rv_mnemonic, "mul") == 0 || strcmp(rv_mnemonic, "and") == 0 ||
            strcmp(rv_mnemonic, "or") == 0  || strcmp(rv_mnemonic, "xor") == 0 ||
            strcmp(rv_mnemonic, "sll") == 0 || strcmp(rv_mnemonic, "srl") == 0 ||
            strcmp(rv_mnemonic, "sra") == 0 || strcmp(rv_mnemonic, "div") == 0 ||
            strcmp(rv_mnemonic, "divu") == 0|| strcmp(rv_mnemonic, "rem") == 0 ||
            strcmp(rv_mnemonic, "remu") == 0|| strcmp(rv_mnemonic, "slt") == 0 ||
            strcmp(rv_mnemonic, "sltu") == 0) {
            ok = ae_rv64_emit_alu3(out, rv_mnemonic, &left, &right, &third);
            goto cleanup;
        }

        /* Fallback: emit as-is with remapped mnemonic */
        ok = ae_emit_line(out, "    %s %s, %s, %s\n",
                          rv_mnemonic, left.text, right.text, third.text);
    }
    goto cleanup;

fail:
    ok = false;
cleanup:
    ae_free_operand(&left);
    ae_free_operand(&right);
    ae_free_operand(&third);
    free(mnemonic);
    free(operand_text);
    free(left_text);
    free(right_text);
    free(third_text);
    return ok;
}
