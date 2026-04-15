#include "asm_emit_internal.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

AsmUnitLayout ae_compute_unit_layout(const MachineUnit *unit) {
    AsmUnitLayout layout;
    size_t block_index;
    size_t instruction_index;
    size_t named_local_words;

    memset(&layout, 0, sizeof(layout));
    layout.frame_words = unit->frame_slot_count;
    layout.spill_words = unit->spill_slot_count;
    layout.helper_words = unit->helper_slot_count;
    layout.outgoing_words = unit->outgoing_stack_slot_count;

    if (unit->block_count > 0) {
        const MachineBlock *entry_block = &unit->blocks[0];

        for (instruction_index = 0; instruction_index < entry_block->instruction_count; instruction_index++) {
            if (strcmp(entry_block->instructions[instruction_index].text, "push r12") == 0) {
                layout.saves_r12 = true;
            } else if (strcmp(entry_block->instructions[instruction_index].text, "push r13") == 0) {
                layout.saves_r13 = true;
            }
        }
    }

    for (block_index = 0; block_index < unit->block_count; block_index++) {
        for (instruction_index = 0;
             instruction_index < unit->blocks[block_index].instruction_count;
             instruction_index++) {
            const char *text = unit->blocks[block_index].instructions[instruction_index].text;

            if (ae_starts_with(text, "call ")) {
                layout.has_calls = true;
            }
            if (strcmp(text, "push r10") == 0 || strcmp(text, "pop r10") == 0) {
                layout.preserves_r10 = true;
            }
            if (strcmp(text, "push r11") == 0 || strcmp(text, "pop r11") == 0) {
                layout.preserves_r11 = true;
            }
        }
    }

    layout.saved_reg_words = 1 + (layout.saves_r12 ? 1 : 0) + (layout.saves_r13 ? 1 : 0);
    layout.call_preserve_words = (layout.preserves_r10 ? 1 : 0) + (layout.preserves_r11 ? 1 : 0);
    named_local_words = layout.frame_words + layout.spill_words + layout.helper_words + layout.call_preserve_words + layout.outgoing_words;
    if (layout.has_calls && ((layout.saved_reg_words + named_local_words) % 2) != 0) {
        layout.alignment_pad_words = 1;
    }
    layout.total_local_words = named_local_words + layout.alignment_pad_words;
    return layout;
}

bool ae_translate_operand(AsmEmitContext *context,
                              const MachineUnit *unit,
                              const AsmUnitLayout *layout,
                              size_t block_index,
                              size_t instruction_index,
                              const char *operand_text,
                              bool destination,
                              AsmOperand *operand) {
    char *inner = NULL;

    (void)block_index;
    (void)instruction_index;
    (void)destination;

    if (!operand || !operand_text) {
        return false;
    }
    memset(operand, 0, sizeof(*operand));

    if (ae_is_register_name(operand_text)) {
        operand->kind = ASM_OPERAND_REGISTER;
        operand->text = ae_copy_text(operand_text);
        return operand->text != NULL;
    }
    if ((operand_text[0] == '-' && isdigit((unsigned char)operand_text[1])) ||
        isdigit((unsigned char)operand_text[0])) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text(operand_text);
        return operand->text != NULL;
    }
    if (strcmp(operand_text, "null") == 0) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text("0");
        return operand->text != NULL;
    }
    if (strcmp(operand_text, "bool(true)") == 0 || strcmp(operand_text, "bool(false)") == 0) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = ae_copy_text(strcmp(operand_text, "bool(true)") == 0 ? "1" : "0");
        return operand->text != NULL;
    }

    inner = ae_between_parens(operand_text, "frame(");
    if (inner) {
        const MachineFrameSlot *slot = ae_find_frame_slot(unit, inner);
        free(inner);
        if (!slot) {
            return false;
        }
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("QWORD PTR [rbp - %zu]", ae_frame_slot_offset(layout, slot->index));
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "spill(");
    if (inner) {
        size_t spill_index = (size_t)strtoull(inner, NULL, 10);
        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("QWORD PTR [rbp - %zu]", ae_spill_slot_offset(layout, unit, spill_index));
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "helper(");
    if (inner) {
        size_t helper_index = (size_t)strtoull(inner, NULL, 10);
        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("QWORD PTR [rbp - %zu]", ae_helper_slot_offset(layout, unit, helper_index));
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "argin(");
    if (inner) {
        size_t argument_index = (size_t)strtoull(inner, NULL, 10);
        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("QWORD PTR [rbp + %zu]", 16 + (argument_index * 8));
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "argout(");
    if (inner) {
        size_t argument_index = (size_t)strtoull(inner, NULL, 10);
        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("QWORD PTR [rsp + %zu]", argument_index * 8);
        return operand->text != NULL;
    }
    inner = ae_between_parens(operand_text, "env(");
    if (inner) {
        size_t capture_index = (size_t)strtoull(inner, NULL, 10);
        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = ae_copy_format("QWORD PTR [r15 + %zu]", capture_index * 8);
        return operand->text != NULL;
    }

    /* Complex operand types handled in asm_emit_operand_ext.c */
    return ae_translate_operand_ext(context, unit, layout, operand_text, destination, operand);
}
