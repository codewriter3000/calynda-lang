#include "asm_emit_internal.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

/* Forward declaration for extended operand handler. */
bool ae_translate_operand_ext_riscv64(AsmEmitContext *context,
                                      const MachineUnit *unit,
                                      const AsmUnitLayout *layout,
                                      const char *operand_text,
                                      bool destination,
                                      AsmOperand *operand);

/* ------------------------------------------------------------------ */
/*  Caller-saved register preserve detection (RV64)                    */
/* ------------------------------------------------------------------ */

/*
 * Map an RV64 caller-saved allocatable register name to a preserve index.
 * Returns -1 if the register is not a caller-saved allocatable register.
 * RV64 allocatable registers: t1-t6 (all are caller-saved).
 */
int ae_rv64_preserve_index(const char *reg_name) {
    if (!reg_name || reg_name[0] != 't') {
        return -1;
    }
    if (reg_name[1] >= '1' && reg_name[1] <= '2' && reg_name[2] == '\0') {
        return reg_name[1] - '1';
    }
    if (reg_name[1] >= '3' && reg_name[1] <= '6' && reg_name[2] == '\0') {
        return reg_name[1] - '3' + 2;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/*  RV64 unit layout computation                                       */
/* ------------------------------------------------------------------ */

AsmUnitLayout ae_compute_unit_layout_riscv64(const MachineUnit *unit) {
    AsmUnitLayout layout;
    size_t block_index;
    size_t instruction_index;
    size_t named_local_words;
    bool preserve_seen[6];
    int pi;
    size_t preserve_count;

    memset(&layout, 0, sizeof(layout));
    memset(preserve_seen, 0, sizeof(preserve_seen));
    layout.frame_words = unit->frame_slot_count;
    layout.spill_words = unit->spill_slot_count;
    layout.helper_words = unit->helper_slot_count;
    layout.outgoing_words = unit->outgoing_stack_slot_count;

    /*
     * saved_reg_words = 2: t0 (work register) and s1 (scratch2).
     * Both are saved in the prologue below s0.
     */
    layout.saved_reg_words = 2;

    /* Detect calls and caller-saved preserve push/pop */
    for (block_index = 0; block_index < unit->block_count; block_index++) {
        for (instruction_index = 0;
             instruction_index < unit->blocks[block_index].instruction_count;
             instruction_index++) {
            const char *text =
                unit->blocks[block_index].instructions[instruction_index].text;

            if (ae_starts_with(text, "call ")) {
                layout.has_calls = true;
            }
            if (ae_starts_with(text, "push ")) {
                pi = ae_rv64_preserve_index(text + 5);
                if (pi >= 0) {
                    preserve_seen[pi] = true;
                }
            }
        }
    }

    preserve_count = 0;
    for (pi = 0; pi < 6; pi++) {
        if (preserve_seen[pi]) {
            preserve_count++;
        }
    }
    layout.call_preserve_words = preserve_count;

    named_local_words = layout.frame_words + layout.spill_words +
                        layout.helper_words + layout.call_preserve_words +
                        layout.outgoing_words;

    /* RV64 requires 16-byte stack alignment.
     * addi sp, sp, -16 saves ra+s0.  The sub sp allocates
     * (saved_reg_words + total_local_words) * 8 bytes,
     * which must be a multiple of 16. */
    if ((layout.saved_reg_words + named_local_words) % 2 != 0) {
        layout.alignment_pad_words = 1;
    }
    layout.total_local_words = named_local_words + layout.alignment_pad_words;
    return layout;
}

/* ------------------------------------------------------------------ */
/*  RV64 operand translation                                           */
/* ------------------------------------------------------------------ */

bool ae_translate_operand_riscv64(AsmEmitContext *context,
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

    /* Numeric literal (RV64 GAS uses plain numbers, no # prefix) */
    if ((operand_text[0] == '-' && operand_text[1] >= '0' &&
         operand_text[1] <= '9') ||
        (operand_text[0] >= '0' && operand_text[0] <= '9')) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text(operand_text);
        return operand->text != NULL;
    }

    /* null → zero register or 0 */
    if (strcmp(operand_text, "null") == 0) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text("0");
        return operand->text != NULL;
    }

    /* bool(true) / bool(false) */
    if (strcmp(operand_text, "bool(true)") == 0) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text("1");
        return operand->text != NULL;
    }
    if (strcmp(operand_text, "bool(false)") == 0) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text("0");
        return operand->text != NULL;
    }

    /* frame(name) → -offset(s0) */
    inner = ae_between_parens(operand_text, "frame(");
    if (inner) {
        const MachineFrameSlot *slot = ae_find_frame_slot(unit, inner);

        free(inner);
        if (!slot) {
            return false;
        }
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("-%zu(s0)",
                                       ae_frame_slot_offset(layout, slot->index));
        return operand->text != NULL;
    }

    /* spill(N) → -offset(s0) */
    inner = ae_between_parens(operand_text, "spill(");
    if (inner) {
        size_t spill_index = (size_t)strtoull(inner, NULL, 10);

        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("-%zu(s0)",
                                       ae_spill_slot_offset(layout, unit, spill_index));
        return operand->text != NULL;
    }

    /* helper(N) → -offset(s0) */
    inner = ae_between_parens(operand_text, "helper(");
    if (inner) {
        size_t helper_index = (size_t)strtoull(inner, NULL, 10);

        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("-%zu(s0)",
                                       ae_helper_slot_offset(layout, unit, helper_index));
        return operand->text != NULL;
    }

    /* argin(N) → N*8(s0) (stack args from caller, at old SP) */
    inner = ae_between_parens(operand_text, "argin(");
    if (inner) {
        size_t argument_index = (size_t)strtoull(inner, NULL, 10);

        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("%zu(s0)", argument_index * 8);
        return operand->text != NULL;
    }

    /* argout(N) → N*8(sp) */
    inner = ae_between_parens(operand_text, "argout(");
    if (inner) {
        size_t argument_index = (size_t)strtoull(inner, NULL, 10);

        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("%zu(sp)", argument_index * 8);
        return operand->text != NULL;
    }

    /* env(N) → N*8(s11) (closure environment) */
    inner = ae_between_parens(operand_text, "env(");
    if (inner) {
        size_t capture_index = (size_t)strtoull(inner, NULL, 10);

        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("%zu(s11)", capture_index * 8);
        return operand->text != NULL;
    }

    /* Delegate to ext handler for complex types */
    return ae_translate_operand_ext_riscv64(context, unit, layout,
                                            operand_text, destination, operand);
}
