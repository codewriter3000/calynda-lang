#include "lir.h"
#include "lir_internal.h"

#include <stdlib.h>
#include <string.h>

bool lr_emit_entry_abi(LirBuildContext *context,
                       const LirUnit *unit,
                       LirBasicBlock *block) {
    LirInstruction instruction;
    size_t i;
    size_t capture_index = 0;
    size_t argument_index = 0;

    if (!unit || !block) {
        return false;
    }

    for (i = 0; i < unit->slot_count; i++) {
        if (unit->slots[i].kind != LIR_SLOT_CAPTURE) {
            continue;
        }

        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = LIR_INSTR_INCOMING_CAPTURE;
        instruction.as.incoming_capture.slot_index = i;
        instruction.as.incoming_capture.capture_index = capture_index++;
        if (!lr_append_instruction(block, instruction)) {
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR incoming captures.");
            return false;
        }
    }

    for (i = 0; i < unit->slot_count; i++) {
        if (unit->slots[i].kind != LIR_SLOT_PARAMETER) {
            continue;
        }

        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = LIR_INSTR_INCOMING_ARG;
        instruction.as.incoming_arg.slot_index = i;
        instruction.as.incoming_arg.argument_index = argument_index++;
        if (!lr_append_instruction(block, instruction)) {
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR incoming arguments.");
            return false;
        }
    }

    return true;
}

static bool lower_mir_terminator(LirBuildContext *context,
                                 const LirUnit *unit,
                                 LirBasicBlock *block,
                                 const MirTerminator *terminator) {
    if (!block || !terminator) {
        return false;
    }

    memset(&block->terminator, 0, sizeof(block->terminator));
    switch (terminator->kind) {
    case MIR_TERM_NONE:
        block->terminator.kind = LIR_TERM_NONE;
        return true;

    case MIR_TERM_RETURN:
        block->terminator.kind = LIR_TERM_RETURN;
        block->terminator.as.return_term.has_value = terminator->as.return_term.has_value;
        if (!terminator->as.return_term.has_value) {
            return true;
        }
        return lr_operand_from_mir_value(context, unit,
                                         terminator->as.return_term.value,
                                         &block->terminator.as.return_term.value);

    case MIR_TERM_GOTO:
        block->terminator.kind = LIR_TERM_JUMP;
        block->terminator.as.jump_term.target_block = terminator->as.goto_term.target_block;
        return true;

    case MIR_TERM_BRANCH:
        block->terminator.kind = LIR_TERM_BRANCH;
        block->terminator.as.branch_term.true_block = terminator->as.branch_term.true_block;
        block->terminator.as.branch_term.false_block = terminator->as.branch_term.false_block;
        return lr_operand_from_mir_value(context, unit,
                                         terminator->as.branch_term.condition,
                                         &block->terminator.as.branch_term.condition);

    case MIR_TERM_THROW:
        block->terminator.kind = LIR_TERM_THROW;
        return lr_operand_from_mir_value(context, unit,
                                         terminator->as.throw_term.value,
                                         &block->terminator.as.throw_term.value);
    }

    return false;
}

static bool lower_mir_block(LirBuildContext *context,
                            const MirUnit *mir_unit,
                            const LirUnit *unit,
                            const MirBasicBlock *mir_block,
                            bool is_entry_block,
                            LirBasicBlock *block) {
    size_t i;

    if (!unit || !mir_block || !block) {
        return false;
    }

    memset(block, 0, sizeof(*block));
    block->label = ast_copy_text(mir_block->label);
    if (!block->label) {
        lr_set_error(context, (AstSourceSpan){0}, NULL,
                     "Out of memory while lowering LIR blocks.");
        return false;
    }

    if (is_entry_block && !lr_emit_entry_abi(context, unit, block)) {
        lr_basic_block_free(block);
        return false;
    }

    for (i = 0; i < mir_block->instruction_count; i++) {
        if (!lr_lower_mir_instruction(context, mir_unit, unit, block,
                                      &mir_block->instructions[i])) {
            lr_basic_block_free(block);
            return false;
        }
    }

    if (!lower_mir_terminator(context, unit, block, &mir_block->terminator)) {
        lr_basic_block_free(block);
        return false;
    }

    return true;
}

bool lr_lower_mir_unit(LirBuildContext *context,
                       const MirUnit *mir_unit,
                       LirUnit *unit) {
    size_t i;

    if (!context || !mir_unit || !unit) {
        return false;
    }

    memset(unit, 0, sizeof(*unit));
    unit->kind = lr_unit_kind_from_mir(mir_unit->kind);
    unit->name = ast_copy_text(mir_unit->name);
    unit->symbol = mir_unit->symbol;
    unit->return_type = mir_unit->return_type;
    unit->parameter_count = mir_unit->parameter_count;
    unit->virtual_register_count = mir_unit->next_temp_index;
    unit->is_boot = mir_unit->is_boot;
    if (!unit->name) {
        lr_set_error(context, (AstSourceSpan){0}, NULL,
                     "Out of memory while lowering LIR units.");
        return false;
    }

    if (mir_unit->kind == MIR_UNIT_ASM) {
        unit->asm_body = ast_copy_text_n(mir_unit->asm_body,
                                         mir_unit->asm_body_length);
        unit->asm_body_length = mir_unit->asm_body_length;
        if (mir_unit->asm_body && !unit->asm_body) {
            lr_unit_free(unit);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR asm body.");
            return false;
        }
        return true;
    }

    for (i = 0; i < mir_unit->local_count; i++) {
        LirSlot slot;

        memset(&slot, 0, sizeof(slot));
        slot.kind = lr_slot_kind_from_mir(mir_unit->locals[i].kind);
        slot.name = ast_copy_text(mir_unit->locals[i].name);
        slot.symbol = mir_unit->locals[i].symbol;
        slot.type = mir_unit->locals[i].type;
        slot.is_final = mir_unit->locals[i].is_final;
        slot.index = mir_unit->locals[i].index;
        if (!slot.name || !lr_append_slot(unit, slot)) {
            free(slot.name);
            lr_unit_free(unit);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR slots.");
            return false;
        }
        if (slot.kind == LIR_SLOT_CAPTURE) {
            unit->capture_count++;
        }
    }

    for (i = 0; i < mir_unit->block_count; i++) {
        LirBasicBlock block;

        if (!lower_mir_block(context, mir_unit, unit,
                             &mir_unit->blocks[i], i == 0, &block) ||
            !lr_append_block(unit, block)) {
            lr_basic_block_free(&block);
            lr_unit_free(unit);
            if (!context->program->has_error) {
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while assembling LIR blocks.");
            }
            return false;
        }
    }

    return true;
}
