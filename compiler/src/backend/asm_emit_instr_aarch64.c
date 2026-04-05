#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

/*
 * ARM64 (AArch64) assembly instruction emitter.
 *
 * The machine IR uses generic mnemonics with abstract operands (frame(),
 * spill(), helper(), etc.).  This file translates them into GNU-as
 * compatible AArch64 assembly.
 *
 * Prologue/epilogue layout:
 *   stp x29, x30, [sp, #-16]!        // save FP+LR
 *   mov x29, sp                       // set up frame pointer
 *   sub sp, sp, #frame_size           // allocate locals (multiple of 16)
 *   str x16, [x29, #-8]              // save work register
 *
 * The frame slots are addressed as [x29, #-offset] using the SAME offset
 * formulas as x86_64 (ae_frame_slot_offset, ae_spill_slot_offset, etc.)
 * since saved_reg_words = 1 accounts for the work register at [x29, #-8].
 */

/* Forward declarations for functions defined later in this file. */
static bool ae_translate_operand_ext_aarch64(AsmEmitContext *context,
                                             const MachineUnit *unit,
                                             const AsmUnitLayout *layout,
                                             const char *operand_text,
                                             bool destination,
                                             AsmOperand *operand);

/* ------------------------------------------------------------------ */
/*  Caller-saved register preserve detection (ARM64)                   */
/* ------------------------------------------------------------------ */

/*
 * Map an ARM64 caller-saved allocatable register name to a preserve index.
 * Returns -1 if the register is not a caller-saved allocatable register.
 * ARM64 allocatable registers: x9-x15 (all are caller-saved).
 */
static int ae_arm64_preserve_index(const char *reg_name) {
    if (strlen(reg_name) < 2 || reg_name[0] != 'x') {
        return -1;
    }
    {
        int regnum = atoi(reg_name + 1);

        if (regnum >= 9 && regnum <= 15) {
            return regnum - 9;
        }
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/*  AArch64 unit layout computation                                    */
/* ------------------------------------------------------------------ */

AsmUnitLayout ae_compute_unit_layout_aarch64(const MachineUnit *unit) {
    AsmUnitLayout layout;
    size_t block_index;
    size_t instruction_index;
    size_t named_local_words;
    bool preserve_seen[7];
    int pi;
    size_t preserve_count;

    memset(&layout, 0, sizeof(layout));
    memset(preserve_seen, 0, sizeof(preserve_seen));
    layout.frame_words = unit->frame_slot_count;
    layout.spill_words = unit->spill_slot_count;
    layout.helper_words = unit->helper_slot_count;
    layout.outgoing_words = unit->outgoing_stack_slot_count;

    /*
     * The work register (x16) is always saved: saved_reg_words = 1.
     * On ARM64, no allocatable registers (x9-x15) are callee-saved,
     * so the machine prologue does not push any additional registers.
     */
    layout.saved_reg_words = 1;

    /* Detect calls and caller-saved preserve push/pop */
    for (block_index = 0; block_index < unit->block_count; block_index++) {
        for (instruction_index = 0;
             instruction_index < unit->blocks[block_index].instruction_count;
             instruction_index++) {
            const char *text = unit->blocks[block_index].instructions[instruction_index].text;

            if (ae_starts_with(text, "bl ")) {
                layout.has_calls = true;
            }
            if (ae_starts_with(text, "push ")) {
                const char *reg = text + 5;

                pi = ae_arm64_preserve_index(reg);
                if (pi >= 0) {
                    preserve_seen[pi] = true;
                }
            }
        }
    }

    preserve_count = 0;
    for (pi = 0; pi < 7; pi++) {
        if (preserve_seen[pi]) {
            preserve_count++;
        }
    }
    layout.call_preserve_words = preserve_count;

    named_local_words = layout.frame_words + layout.spill_words +
                        layout.helper_words + layout.call_preserve_words +
                        layout.outgoing_words;

    /* ARM64 requires 16-byte stack alignment. The stp x29,x30 uses 16 bytes.
     * The sub sp allocates (saved_reg_words + total_local_words) * 8 bytes,
     * which must be a multiple of 16.  So (saved_reg_words + total) must be even. */
    if ((layout.saved_reg_words + named_local_words) % 2 != 0) {
        layout.alignment_pad_words = 1;
    }
    layout.total_local_words = named_local_words + layout.alignment_pad_words;
    return layout;
}

/* ------------------------------------------------------------------ */
/*  AArch64 operand translation                                        */
/* ------------------------------------------------------------------ */

bool ae_translate_operand_aarch64(AsmEmitContext *context,
                                  const MachineUnit *unit,
                                  const AsmUnitLayout *layout,
                                  const char *operand_text,
                                  bool destination,
                                  AsmOperand *operand) {
    char *inner = NULL;

    (void)destination;

    if (!operand || !operand_text) {
        return false;
    }
    memset(operand, 0, sizeof(*operand));

    /* Register operand */
    if (ae_is_register_name(operand_text)) {
        operand->kind = ASM_OPERAND_REGISTER;
        operand->text = ae_copy_text(operand_text);
        return operand->text != NULL;
    }

    /* Numeric literal → #N */
    if ((operand_text[0] == '-' && operand_text[1] >= '0' && operand_text[1] <= '9') ||
        (operand_text[0] >= '0' && operand_text[0] <= '9')) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_format("#%s", operand_text);
        return operand->text != NULL;
    }

    /* null → xzr (zero register) or #0 */
    if (strcmp(operand_text, "null") == 0) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text("#0");
        return operand->text != NULL;
    }

    /* bool(true) / bool(false) */
    if (strcmp(operand_text, "bool(true)") == 0) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text("#1");
        return operand->text != NULL;
    }
    if (strcmp(operand_text, "bool(false)") == 0) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text("#0");
        return operand->text != NULL;
    }

    /* frame(name) → [x29, #-offset] */
    inner = ae_between_parens(operand_text, "frame(");
    if (inner) {
        const MachineFrameSlot *slot = ae_find_frame_slot(unit, inner);

        free(inner);
        if (!slot) {
            return false;
        }
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("[x29, #-%zu]",
                                       ae_frame_slot_offset(layout, slot->index));
        return operand->text != NULL;
    }

    /* spill(N) → [x29, #-offset] */
    inner = ae_between_parens(operand_text, "spill(");
    if (inner) {
        size_t spill_index = (size_t)strtoull(inner, NULL, 10);

        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("[x29, #-%zu]",
                                       ae_spill_slot_offset(layout, unit, spill_index));
        return operand->text != NULL;
    }

    /* helper(N) → [x29, #-offset] */
    inner = ae_between_parens(operand_text, "helper(");
    if (inner) {
        size_t helper_index = (size_t)strtoull(inner, NULL, 10);

        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("[x29, #-%zu]",
                                       ae_helper_slot_offset(layout, unit, helper_index));
        return operand->text != NULL;
    }

    /* argin(N) → [x29, #16+N*8] (stack args from caller, above FP+LR) */
    inner = ae_between_parens(operand_text, "argin(");
    if (inner) {
        size_t argument_index = (size_t)strtoull(inner, NULL, 10);

        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("[x29, #%zu]", 16 + (argument_index * 8));
        return operand->text != NULL;
    }

    /* argout(N) → [sp, #N*8] */
    inner = ae_between_parens(operand_text, "argout(");
    if (inner) {
        size_t argument_index = (size_t)strtoull(inner, NULL, 10);

        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("[sp, #%zu]", argument_index * 8);
        return operand->text != NULL;
    }

    /* env(N) → [x28, #N*8] (closure environment) */
    inner = ae_between_parens(operand_text, "env(");
    if (inner) {
        size_t capture_index = (size_t)strtoull(inner, NULL, 10);

        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("[x28, #%zu]", capture_index * 8);
        return operand->text != NULL;
    }

    /* Delegate to ext handler for complex types */
    return ae_translate_operand_ext_aarch64(context, unit, layout,
                                            operand_text, destination, operand);
}

/* ------------------------------------------------------------------ */
/*  Extended operand translation for ARM64                             */
/* ------------------------------------------------------------------ */

static bool ae_translate_operand_ext_aarch64(AsmEmitContext *context,
                                             const MachineUnit *unit,
                                             const AsmUnitLayout *layout,
                                             const char *operand_text,
                                             bool destination,
                                             AsmOperand *operand) {
    char *inner = NULL;

    (void)layout;

    /* global(name) → data label */
    inner = ae_between_parens(operand_text, "global(");
    if (inner) {
        AsmGlobalSymbol *sym = ae_ensure_global_symbol(context, inner, destination);

        free(inner);
        if (!sym) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(sym->symbol);
        return operand->text != NULL;
    }

    /* code(name) → function label address */
    inner = ae_between_parens(operand_text, "code(");
    if (inner) {
        const MachineUnit *referenced_unit = ae_find_program_unit(context, inner);

        if (referenced_unit && referenced_unit->kind == LIR_UNIT_LAMBDA) {
            operand->kind = ASM_OPERAND_ADDRESS;
            operand->text = ae_closure_wrapper_symbol_name(inner);
            free(inner);
            return operand->text != NULL;
        }
        {
            AsmUnitSymbol *sym = ae_ensure_unit_symbol(context, inner);

            free(inner);
            if (!sym) {
                return false;
            }
            operand->kind = ASM_OPERAND_ADDRESS;
            operand->text = ae_copy_text(sym->symbol);
            return operand->text != NULL;
        }
    }

    /* symbol(name) → interned symbol string pointer */
    inner = ae_between_parens(operand_text, "symbol(");
    if (inner) {
        AsmByteLiteral *lit = ae_ensure_byte_literal(context, inner, strlen(inner), "sym");

        free(inner);
        if (!lit) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(lit->label);
        return operand->text != NULL;
    }

    /* text("...") → raw byte literal */
    inner = ae_between_parens(operand_text, "text(");
    if (inner) {
        char *decoded = NULL;
        size_t decoded_length = 0;
        AsmByteLiteral *lit;

        if (!ae_decode_quoted_text(inner, &decoded, &decoded_length)) {
            free(inner);
            return false;
        }
        lit = ae_ensure_byte_literal(context, decoded, decoded_length, "txt");
        free(decoded);
        free(inner);
        if (!lit) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(lit->label);
        return operand->text != NULL;
    }

    /* string("...") → string object literal */
    inner = ae_between_parens(operand_text, "string(");
    if (inner) {
        char *decoded = NULL;
        size_t decoded_length = 0;
        AsmStringObjectLiteral *lit;

        if (!ae_decode_quoted_text(inner, &decoded, &decoded_length)) {
            free(inner);
            return false;
        }
        lit = ae_ensure_string_literal(context, decoded, decoded_length);
        free(decoded);
        free(inner);
        if (!lit) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = ae_copy_text(lit->object_label);
        return operand->text != NULL;
    }

    /* tag(text) / tag(value) */
    inner = ae_between_parens(operand_text, "tag(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_format("#%s", strcmp(inner, "text") == 0 ? "0" : "1");
        free(inner);
        return operand->text != NULL;
    }

    /* type(name) → runtime type tag */
    inner = ae_between_parens(operand_text, "type(");
    if (inner) {
        long long type_tag = ae_runtime_type_tag_value(inner);

        free(inner);
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_format("#%lld", type_tag);
        return operand->text != NULL;
    }

    /* int32(N) / int64(N) */
    inner = ae_between_parens(operand_text, "int32(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_format("#%s", inner);
        free(inner);
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "int64(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_format("#%s", inner);
        free(inner);
        return operand->text != NULL;
    }

    /* Branch target labels (bbN) */
    if (ae_starts_with(operand_text, "bb")) {
        AsmUnitSymbol *sym = ae_ensure_unit_symbol(context, unit->name);

        if (!sym) {
            return false;
        }
        operand->kind = ASM_OPERAND_BRANCH_LABEL;
        operand->text = ae_copy_format(".L%s_%s", sym->symbol, operand_text);
        return operand->text != NULL;
    }

    /* Runtime function call target */
    if (ae_starts_with(operand_text, "__calynda_rt_")) {
        operand->kind = ASM_OPERAND_CALL_TARGET;
        operand->text = ae_copy_text(operand_text);
        return operand->text != NULL;
    }

    /* Named function call target */
    {
        AsmUnitSymbol *sym = ae_ensure_unit_symbol(context, operand_text);

        if (!sym) {
            return false;
        }
        operand->kind = ASM_OPERAND_CALL_TARGET;
        operand->text = ae_copy_text(sym->symbol);
        return operand->text != NULL;
    }
}

/* ------------------------------------------------------------------ */
/*  ARM64 mov helper: handles reg/imm/mem combinations                 */
/* ------------------------------------------------------------------ */

static bool ae_arm64_emit_mov(FILE *out, AsmOperand *dest, AsmOperand *src) {
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

static bool ae_arm64_emit_alu3(FILE *out,
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
        if (strcmp(mnemonic, "mov") == 0) {
            ok = ae_arm64_emit_mov(out, &left, &right);
            goto cleanup;
        }
        if (strcmp(mnemonic, "cmp") == 0) {
            /* cmp Xn, operand — both must be registers */
            if (left.kind == ASM_OPERAND_MEMORY) {
                ok = ae_emit_line(out, "    ldr x16, %s\n", left.text);
                if (!ok) goto cleanup;
                if (right.kind == ASM_OPERAND_MEMORY) {
                    ok = ae_emit_line(out, "    ldr x17, %s\n", right.text) &&
                         ae_emit_line(out, "    cmp x16, x17\n");
                } else {
                    ok = ae_emit_line(out, "    cmp x16, %s\n", right.text);
                }
            } else if (right.kind == ASM_OPERAND_MEMORY) {
                ok = ae_emit_line(out, "    ldr x17, %s\n", right.text) &&
                     ae_emit_line(out, "    cmp %s, x17\n", left.text);
            } else {
                ok = ae_emit_line(out, "    cmp %s, %s\n", left.text, right.text);
            }
            goto cleanup;
        }
        if (strcmp(mnemonic, "cset") == 0) {
            /* cset Xd, cc — right operand is a condition code name */
            ok = ae_emit_line(out, "    cset %s, %s\n", left.text, right.text);
            goto cleanup;
        }
        if (strcmp(mnemonic, "neg") == 0 || strcmp(mnemonic, "mvn") == 0) {
            /* neg Xd, Xn / mvn Xd, Xn */
            const char *src = right.text;

            if (right.kind == ASM_OPERAND_MEMORY) {
                if (!ae_emit_line(out, "    ldr x17, %s\n", right.text)) {
                    goto fail;
                }
                src = "x17";
            }
            if (left.kind == ASM_OPERAND_MEMORY) {
                ok = ae_emit_line(out, "    %s x16, %s\n", mnemonic, src) &&
                     ae_emit_line(out, "    str x16, %s\n", left.text);
            } else {
                ok = ae_emit_line(out, "    %s %s, %s\n", mnemonic, left.text, src);
            }
            goto cleanup;
        }
        if (strcmp(mnemonic, "lea") == 0) {
            /* lea Xd, frame/spill/helper → compute address using sub from FP */
            if (right.kind == ASM_OPERAND_MEMORY) {
                /* Parse [x29, #-N] to extract offset */
                const char *hash = strchr(right.text, '#');

                if (hash && hash[1] == '-') {
                    char *offset_str = ae_copy_text(hash + 2);

                    if (offset_str) {
                        size_t len = strlen(offset_str);

                        if (len > 0 && offset_str[len - 1] == ']') {
                            offset_str[len - 1] = '\0';
                        }
                        if (left.kind == ASM_OPERAND_MEMORY) {
                            ok = ae_emit_line(out, "    sub x16, x29, #%s\n", offset_str) &&
                                 ae_emit_line(out, "    str x16, %s\n", left.text);
                        } else {
                            ok = ae_emit_line(out, "    sub %s, x29, #%s\n",
                                              left.text, offset_str);
                        }
                        free(offset_str);
                        goto cleanup;
                    }
                }
            }
            /* Fallback: mov */
            ok = ae_arm64_emit_mov(out, &left, &right);
            goto cleanup;
        }
        /* Generic two-operand fallback */
        ok = ae_emit_line(out, "    %s %s, %s\n", mnemonic, left.text, right.text);
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
