#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

bool mc_build_unit(MachineBuildContext *context,
                   const LirUnit *lir_unit,
                   const CodegenUnit *codegen_unit,
                   MachineUnit *machine_unit) {
    size_t block_index;

    if (!lir_unit || !codegen_unit || !machine_unit) {
        return false;
    }
    if (lir_unit->block_count != codegen_unit->block_count) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Machine emission requires matching LIR/codegen block counts for unit %s.",
                     lir_unit->name);
        return false;
    }

    memset(machine_unit, 0, sizeof(*machine_unit));
    machine_unit->kind = lir_unit->kind;
    machine_unit->name = ast_copy_text(lir_unit->name);
    machine_unit->return_type = lir_unit->return_type;
    machine_unit->is_exported = lir_unit->symbol && lir_unit->symbol->is_exported;
    machine_unit->is_static = lir_unit->symbol && lir_unit->symbol->is_static;
    machine_unit->parameter_count = lir_unit->parameter_count;
    machine_unit->frame_slot_count = codegen_unit->frame_slot_count;
    machine_unit->spill_slot_count = codegen_unit->spill_slot_count;
    machine_unit->is_boot = lir_unit->is_boot;
    machine_unit->helper_slot_count = mc_compute_helper_slot_count(lir_unit, codegen_unit);
    machine_unit->outgoing_stack_slot_count = mc_compute_outgoing_stack_slot_count(lir_unit, codegen_unit, context->program->target_desc);
    machine_unit->block_count = lir_unit->block_count;
    if (!machine_unit->name) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while naming machine unit.");
        return false;
    }

    if (lir_unit->kind == LIR_UNIT_ASM) {
        machine_unit->asm_body = ast_copy_text_n(codegen_unit->asm_body,
                                                 codegen_unit->asm_body_length);
        machine_unit->asm_body_length = codegen_unit->asm_body_length;
        if (codegen_unit->asm_body && !machine_unit->asm_body) {
            mc_unit_free(machine_unit);
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while building machine asm body.");
            return false;
        }
        return true;
    }

    if (machine_unit->frame_slot_count > 0) {
        size_t slot_index;

        machine_unit->frame_slots = calloc(machine_unit->frame_slot_count,
                                           sizeof(*machine_unit->frame_slots));
        if (!machine_unit->frame_slots) {
            mc_unit_free(machine_unit);
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while allocating machine frame slots.");
            return false;
        }
        for (slot_index = 0; slot_index < machine_unit->frame_slot_count; slot_index++) {
            machine_unit->frame_slots[slot_index].index = slot_index;
            machine_unit->frame_slots[slot_index].kind = codegen_unit->frame_slots[slot_index].kind;
            machine_unit->frame_slots[slot_index].name = ast_copy_text(codegen_unit->frame_slots[slot_index].name);
            machine_unit->frame_slots[slot_index].type = codegen_unit->frame_slots[slot_index].type;
            machine_unit->frame_slots[slot_index].is_final = codegen_unit->frame_slots[slot_index].is_final;
            if (!machine_unit->frame_slots[slot_index].name) {
                mc_unit_free(machine_unit);
                mc_set_error(context,
                             (AstSourceSpan){0},
                             NULL,
                             "Out of memory while naming machine frame slots.");
                return false;
            }
        }
    }
    if (machine_unit->block_count > 0) {
        machine_unit->blocks = calloc(machine_unit->block_count, sizeof(*machine_unit->blocks));
        if (!machine_unit->blocks) {
            mc_unit_free(machine_unit);
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while allocating machine blocks.");
            return false;
        }
    }

    for (block_index = 0; block_index < machine_unit->block_count; block_index++) {
        MachineBlock *block = &machine_unit->blocks[block_index];
        const LirBasicBlock *lir_block = &lir_unit->blocks[block_index];
        const CodegenBlock *codegen_block = &codegen_unit->blocks[block_index];
        size_t instruction_index;

        if (lir_block->instruction_count != codegen_block->instruction_count) {
            mc_unit_free(machine_unit);
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Machine emission requires matching LIR/codegen instruction counts for block %s.",
                         lir_block->label);
            return false;
        }

        block->label = ast_copy_text(lir_block->label);
        if (!block->label) {
            mc_unit_free(machine_unit);
            mc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while naming machine blocks.");
            return false;
        }

        if (block_index == 0 && !mc_emit_entry_prologue(context, codegen_unit, machine_unit, block)) {
            mc_unit_free(machine_unit);
            return false;
        }

        for (instruction_index = 0;
             instruction_index < lir_block->instruction_count;
             instruction_index++) {
            if (!mc_emit_instruction(context,
                                     lir_unit,
                                     codegen_unit,
                                     lir_block,
                                     codegen_block,
                                     instruction_index,
                                     block)) {
                mc_unit_free(machine_unit);
                return false;
            }
        }
        if (!mc_emit_terminator(context,
                                lir_unit,
                                codegen_unit,
                                lir_block,
                                machine_unit,
                                block)) {
            mc_unit_free(machine_unit);
            return false;
        }
    }

    return true;
}
