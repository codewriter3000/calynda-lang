#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations for helpers in other aarch64 files. */
int ae_arm64_preserve_index(const char *reg_name);
bool ae_arm64_emit_mov(FILE *out, AsmOperand *dest, AsmOperand *src);
bool ae_arm64_emit_alu3(FILE *out, const char *mnemonic,
                        AsmOperand *dest, AsmOperand *src1, AsmOperand *src2);
bool ae_arm64_emit_two_op(FILE *out, const char *mnemonic, AsmOperand *left,
                          AsmOperand *right, AsmEmitContext *context,
                          const MachineUnit *unit, const AsmUnitLayout *layout);

/* ------------------------------------------------------------------ */
/*  AArch64 instruction emission                                       */
/* ------------------------------------------------------------------ */

bool ae_emit_machine_instruction_aarch64(AsmEmitContext *context,
                                         FILE *out,
                                         const MachineUnit *unit,
                                         const AsmUnitLayout *layout,
                                         size_t unit_index,
                                         size_t block_index,
                                         size_t instruction_index,
                                         const char *instruction_text) {
    char *space;
    char *mnemonic = NULL;
    char *operand_text = NULL;
    char *left_text = NULL;
    char *right_text = NULL;
    char *third_text = NULL;
    AsmOperand left, right, third;
    bool ok = false;
    size_t frame_size;

    (void)unit_index;
    (void)block_index;
    (void)instruction_index;

    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    memset(&third, 0, sizeof(third));

    frame_size = (layout->saved_reg_words + layout->total_local_words) * 8;

    /*
     * Skip prologue instructions — these are emitted by ae_emit_unit_text
     * in asm_emit_sections.c.
     */
    if (strcmp(instruction_text, "push x29") == 0 ||
        strcmp(instruction_text, "mov x29, sp") == 0 ||
        strcmp(instruction_text, "push x16") == 0 ||
        ae_starts_with(instruction_text, "sub sp, ") ||
        ae_starts_with(instruction_text, "add sp, ") ||
        strcmp(instruction_text, "pop x16") == 0 ||
        strcmp(instruction_text, "pop x29") == 0) {
        return true;
    }

    /*
     * Caller-saved register preserve: push xN → str xN, [x29, #-offset]
     * (same as x86_64 converts push r10/r11 to frame-relative stores)
     */
    if (ae_starts_with(instruction_text, "push ")) {
        const char *reg = instruction_text + 5;
        int pi = ae_arm64_preserve_index(reg);

        if (pi >= 0) {
            return ae_emit_line(out, "    str %s, [x29, #-%zu]\n",
                                reg,
                                ae_call_preserve_offset(layout, unit, (size_t)pi));
        }
        return true;
    }
    if (ae_starts_with(instruction_text, "pop ")) {
        const char *reg = instruction_text + 4;
        int pi = ae_arm64_preserve_index(reg);

        if (pi >= 0) {
            return ae_emit_line(out, "    ldr %s, [x29, #-%zu]\n",
                                reg,
                                ae_call_preserve_offset(layout, unit, (size_t)pi));
        }
        return true;
    }

    /* ret → full epilogue */
    if (strcmp(instruction_text, "ret") == 0) {
        if (frame_size > 0 && !ae_emit_line(out, "    add sp, sp, #%zu\n", frame_size)) {
            return false;
        }
        return ae_emit_line(out, "    ldp x29, x30, [sp], #16\n") &&
               ae_emit_line(out, "    ret\n");
    }

    /* Parse mnemonic + operands */
    space = strchr(instruction_text, ' ');
    if (!space) {
        return false;
    }
    mnemonic = ae_copy_text_n(instruction_text, (size_t)(space - instruction_text));
    operand_text = ae_trim_copy(space + 1);
    if (!mnemonic || !operand_text) {
        free(mnemonic);
        free(operand_text);
        return false;
    }

    /* Branch / call instructions (single label operand) */
    if (strcmp(mnemonic, "bl") == 0 || strcmp(mnemonic, "b") == 0 ||
        strcmp(mnemonic, "b.ne") == 0) {
        if (!ae_translate_operand_aarch64(context, unit, layout,
                                          operand_text, false, &left)) {
            goto fail;
        }
        ok = ae_emit_line(out, "    %s %s\n", mnemonic, left.text);
        goto cleanup;
    }

    /* No comma → single-operand instruction (rare on ARM64) */
    if (!strchr(operand_text, ',')) {
        goto fail;
    }

    /* Split operands at commas */
    {
        char *first_comma = strchr(operand_text, ',');
        char *rest;

        left_text = ae_copy_text_n(operand_text, (size_t)(first_comma - operand_text));
        rest = ae_trim_copy(first_comma + 1);
        if (!left_text || !rest) {
            free(rest);
            goto fail;
        }

        {
            char *second_comma = strchr(rest, ',');

            if (second_comma) {
                right_text = ae_copy_text_n(rest, (size_t)(second_comma - rest));
                third_text = ae_trim_copy(second_comma + 1);
            } else {
                right_text = ae_copy_text(rest);
                third_text = NULL;
            }
        }
        free(rest);
    }

    /* Trim whitespace */
    {
        char *tmp;

        tmp = ae_trim_copy(left_text);
        free(left_text);
        left_text = tmp;
        tmp = ae_trim_copy(right_text);
        free(right_text);
        right_text = tmp;
        if (third_text) {
            tmp = ae_trim_copy(third_text);
            free(third_text);
            third_text = tmp;
        }
    }

    /* Translate first two operands */
    if (!ae_translate_operand_aarch64(context, unit, layout,
                                      left_text, true, &left) ||
        !ae_translate_operand_aarch64(context, unit, layout,
                                      right_text, false, &right)) {
        goto fail;
    }

    /* ---- Two-operand instructions ---- */
    if (!third_text) {
        ok = ae_arm64_emit_two_op(out, mnemonic, &left, &right,
                                  context, unit, layout);
        goto cleanup;
    }

    /* Translate third operand */
    if (!ae_translate_operand_aarch64(context, unit, layout,
                                      third_text, false, &third)) {
        goto fail;
    }

    /* ---- Three-operand ALU instructions ---- */
    if (strcmp(mnemonic, "add") == 0 || strcmp(mnemonic, "sub") == 0 ||
        strcmp(mnemonic, "mul") == 0 || strcmp(mnemonic, "and") == 0 ||
        strcmp(mnemonic, "orr") == 0 || strcmp(mnemonic, "eor") == 0 ||
        strcmp(mnemonic, "lsl") == 0 || strcmp(mnemonic, "lsr") == 0 ||
        strcmp(mnemonic, "asr") == 0 || strcmp(mnemonic, "udiv") == 0 ||
        strcmp(mnemonic, "sdiv") == 0) {
        ok = ae_arm64_emit_alu3(out, mnemonic, &left, &right, &third);
        goto cleanup;
    }

    /* msub Rd, Rm, Rn, Ra — four operands (third_text contains "Rn, Ra") */
    if (strcmp(mnemonic, "msub") == 0) {
        char *comma = strchr(third_text, ',');

        if (comma) {
            AsmOperand op3, op4;
            char *t3 = ae_copy_text_n(third_text, (size_t)(comma - third_text));
            char *t4 = ae_trim_copy(comma + 1);

            memset(&op3, 0, sizeof(op3));
            memset(&op4, 0, sizeof(op4));

            if (t3 && t4) {
                char *t3_trimmed = ae_trim_copy(t3);

                free(t3);
                t3 = t3_trimmed;
            }
            if (t3 && t4 &&
                ae_translate_operand_aarch64(context, unit, layout, t3, false, &op3) &&
                ae_translate_operand_aarch64(context, unit, layout, t4, false, &op4)) {
                ok = ae_emit_line(out, "    msub %s, %s, %s, %s\n",
                                  left.text, right.text, op3.text, op4.text);
            }
            ae_free_operand(&op3);
            ae_free_operand(&op4);
            free(t3);
            free(t4);
        }
        goto cleanup;
    }

    /* Fallback: emit as-is */
    ok = ae_emit_line(out, "    %s %s, %s, %s\n",
                      mnemonic, left.text, right.text, third.text);
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
