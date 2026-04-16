#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

/* Forward declaration for extended operand handler. */
bool ae_translate_operand_ext_aarch64(AsmEmitContext *context,
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
int ae_arm64_preserve_index(const char *reg_name) {
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
