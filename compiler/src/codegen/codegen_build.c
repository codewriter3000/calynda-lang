#include "codegen_internal.h"

bool cg_build_unit(CodegenBuildContext *context,
                   const LirUnit *lir_unit,
                   CodegenUnit *unit) {
    size_t i;
    CheckedType *vreg_types;

    if (!lir_unit || !unit) {
        return false;
    }

    memset(unit, 0, sizeof(*unit));
    unit->kind = lir_unit->kind;
    unit->name = ast_copy_text(lir_unit->name);
    unit->symbol = lir_unit->symbol;
    unit->return_type = lir_unit->return_type;
    unit->frame_slot_count = lir_unit->slot_count;
    unit->vreg_count = lir_unit->virtual_register_count;
    unit->block_count = lir_unit->block_count;
    unit->is_boot = lir_unit->is_boot;
    if (!unit->name) {
        cg_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while building codegen units.");
        return false;
    }

    if (lir_unit->kind == LIR_UNIT_ASM) {
        unit->asm_body = ast_copy_text_n(lir_unit->asm_body,
                                         lir_unit->asm_body_length);
        unit->asm_body_length = lir_unit->asm_body_length;
        if (lir_unit->asm_body && !unit->asm_body) {
            cg_unit_free(unit);
            cg_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while building codegen asm body.");
            return false;
        }
        return true;
    }

    if (unit->frame_slot_count > 0) {
        unit->frame_slots = calloc(unit->frame_slot_count, sizeof(*unit->frame_slots));
        if (!unit->frame_slots) {
            cg_unit_free(unit);
            cg_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while building codegen frame slots.");
            return false;
        }
    }
    for (i = 0; i < unit->frame_slot_count; i++) {
        unit->frame_slots[i].lir_slot_index = i;
        unit->frame_slots[i].frame_slot_index = i;
        unit->frame_slots[i].kind = lir_unit->slots[i].kind;
        unit->frame_slots[i].name = ast_copy_text(lir_unit->slots[i].name);
        unit->frame_slots[i].type = lir_unit->slots[i].type;
        unit->frame_slots[i].is_final = lir_unit->slots[i].is_final;
        if (!unit->frame_slots[i].name) {
            cg_unit_free(unit);
            cg_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while naming codegen frame slots.");
            return false;
        }
    }

    if (unit->vreg_count > 0) {
        unit->vreg_allocations = calloc(unit->vreg_count, sizeof(*unit->vreg_allocations));
        if (!unit->vreg_allocations) {
            cg_unit_free(unit);
            cg_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while building codegen register allocations.");
            return false;
        }
    }
    vreg_types = NULL;
    if (unit->vreg_count > 0) {
        vreg_types = calloc(unit->vreg_count, sizeof(*vreg_types));
        if (!vreg_types) {
            cg_unit_free(unit);
            cg_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while inferring codegen virtual-register types.");
            return false;
        }
        for (i = 0; i < unit->vreg_count; i++) {
            vreg_types[i] = cg_checked_type_invalid_value();
        }
        cg_infer_vreg_types(lir_unit, vreg_types, unit->vreg_count);
    }
    for (i = 0; i < unit->vreg_count; i++) {
        const TargetDescriptor *target = context->target;
        size_t alloc_count = target ? target->allocatable_register_count : 0;

        unit->vreg_allocations[i].vreg_index = i;
        unit->vreg_allocations[i].type = vreg_types ? vreg_types[i] : cg_checked_type_invalid_value();
        if (target && i < alloc_count) {
            unit->vreg_allocations[i].location.kind = CODEGEN_VREG_REGISTER;
            unit->vreg_allocations[i].location.as.reg = target->allocatable_registers[i].id;
        } else {
            unit->vreg_allocations[i].location.kind = CODEGEN_VREG_SPILL;
            unit->vreg_allocations[i].location.as.spill_slot_index =
                i - alloc_count;
            unit->spill_slot_count++;
        }
    }
    free(vreg_types);

    if (unit->block_count > 0) {
        unit->blocks = calloc(unit->block_count, sizeof(*unit->blocks));
        if (!unit->blocks) {
            cg_unit_free(unit);
            cg_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while building codegen blocks.");
            return false;
        }
    }
    for (i = 0; i < unit->block_count; i++) {
        size_t j;

        unit->blocks[i].label = ast_copy_text(lir_unit->blocks[i].label);
        unit->blocks[i].instruction_count = lir_unit->blocks[i].instruction_count;
        if (!unit->blocks[i].label) {
            cg_unit_free(unit);
            cg_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while naming codegen blocks.");
            return false;
        }

        if (unit->blocks[i].instruction_count > 0) {
            unit->blocks[i].instructions = calloc(unit->blocks[i].instruction_count,
                                                  sizeof(*unit->blocks[i].instructions));
            if (!unit->blocks[i].instructions) {
                cg_unit_free(unit);
                cg_set_error(context,
                             (AstSourceSpan){0},
                             NULL,
                             "Out of memory while building codegen instruction selections.");
                return false;
            }
        }

        for (j = 0; j < unit->blocks[i].instruction_count; j++) {
            if (!cg_select_instruction(context,
                                       &lir_unit->blocks[i].instructions[j],
                                       &unit->blocks[i].instructions[j])) {
                cg_unit_free(unit);
                return false;
            }
        }

        if (!cg_select_terminator(context,
                                  &lir_unit->blocks[i].terminator,
                                  &unit->blocks[i].terminator)) {
            cg_unit_free(unit);
            return false;
        }
    }

    return true;
}
