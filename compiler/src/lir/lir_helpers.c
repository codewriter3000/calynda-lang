#include "lir.h"
#include "lir_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool lr_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

void lr_set_error(LirBuildContext *context,
                  AstSourceSpan primary_span,
                  const AstSourceSpan *related_span,
                  const char *format,
                  ...) {
    va_list args;

    if (!context || !context->program || context->program->has_error) {
        return;
    }

    context->program->has_error = true;
    context->program->error.primary_span = primary_span;
    if (related_span && lr_source_span_is_valid(*related_span)) {
        context->program->error.related_span = *related_span;
        context->program->error.has_related_span = true;
    }

    va_start(args, format);
    vsnprintf(context->program->error.message,
              sizeof(context->program->error.message),
              format,
              args);
    va_end(args);
}

LirOperand lr_invalid_operand(void) {
    LirOperand operand;

    memset(&operand, 0, sizeof(operand));
    operand.kind = LIR_OPERAND_INVALID;
    return operand;
}

bool lr_operand_from_mir_value(LirBuildContext *context,
                               const LirUnit *unit,
                               MirValue value,
                               LirOperand *operand) {
    if (!operand) {
        return false;
    }

    *operand = lr_invalid_operand();
    operand->type = value.type;
    switch (value.kind) {
    case MIR_VALUE_INVALID:
        return true;

    case MIR_VALUE_TEMP:
        if (!unit || value.as.temp_index >= unit->virtual_register_count) {
            lr_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Internal error: missing LIR virtual register for MIR temp %zu.",
                         value.as.temp_index);
            return false;
        }
        operand->kind = LIR_OPERAND_VREG;
        operand->as.vreg_index = value.as.temp_index;
        return true;

    case MIR_VALUE_LOCAL:
        if (!unit || value.as.local_index >= unit->slot_count) {
            lr_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Internal error: missing LIR slot for MIR local %zu.",
                         value.as.local_index);
            return false;
        }
        operand->kind = LIR_OPERAND_SLOT;
        operand->as.slot_index = value.as.local_index;
        return true;

    case MIR_VALUE_GLOBAL:
        operand->kind = LIR_OPERAND_GLOBAL;
        operand->as.global_name = ast_copy_text(value.as.global_name);
        if (!operand->as.global_name) {
            lr_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while lowering LIR globals.");
            return false;
        }
        return true;

    case MIR_VALUE_LITERAL:
        operand->kind = LIR_OPERAND_LITERAL;
        operand->as.literal.kind = value.as.literal.kind;
        if (value.as.literal.kind == AST_LITERAL_BOOL) {
            operand->as.literal.bool_value = value.as.literal.bool_value;
            return true;
        }
        if (value.as.literal.kind == AST_LITERAL_NULL) {
            return true;
        }
        operand->as.literal.text = ast_copy_text(value.as.literal.text);
        if (!operand->as.literal.text) {
            lr_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while lowering LIR literals.");
            return false;
        }
        return true;
    }

    return false;
}

LirSlotKind lr_slot_kind_from_mir(MirLocalKind kind) {
    switch (kind) {
    case MIR_LOCAL_PARAMETER:
        return LIR_SLOT_PARAMETER;
    case MIR_LOCAL_LOCAL:
        return LIR_SLOT_LOCAL;
    case MIR_LOCAL_CAPTURE:
        return LIR_SLOT_CAPTURE;
    case MIR_LOCAL_SYNTHETIC:
        return LIR_SLOT_SYNTHETIC;
    }

    return LIR_SLOT_LOCAL;
}

LirUnitKind lr_unit_kind_from_mir(MirUnitKind kind) {
    switch (kind) {
    case MIR_UNIT_START:
        return LIR_UNIT_START;
    case MIR_UNIT_BINDING:
        return LIR_UNIT_BINDING;
    case MIR_UNIT_INIT:
        return LIR_UNIT_INIT;
    case MIR_UNIT_LAMBDA:
        return LIR_UNIT_LAMBDA;
    case MIR_UNIT_ASM:
        return LIR_UNIT_ASM;
    }

    return LIR_UNIT_BINDING;
}

bool lr_append_unit(LirProgram *program, LirUnit unit) {
    if (!lr_reserve_items((void **)&program->units,
                          &program->unit_capacity,
                          program->unit_count + 1,
                          sizeof(*program->units))) {
        return false;
    }

    program->units[program->unit_count++] = unit;
    return true;
}

bool lr_append_slot(LirUnit *unit, LirSlot slot) {
    if (!lr_reserve_items((void **)&unit->slots,
                          &unit->slot_capacity,
                          unit->slot_count + 1,
                          sizeof(*unit->slots))) {
        return false;
    }

    unit->slots[unit->slot_count++] = slot;
    return true;
}

bool lr_append_block(LirUnit *unit, LirBasicBlock block) {
    if (!lr_reserve_items((void **)&unit->blocks,
                          &unit->block_capacity,
                          unit->block_count + 1,
                          sizeof(*unit->blocks))) {
        return false;
    }

    unit->blocks[unit->block_count++] = block;
    return true;
}

bool lr_append_instruction(LirBasicBlock *block, LirInstruction instruction) {
    if (!lr_reserve_items((void **)&block->instructions,
                          &block->instruction_capacity,
                          block->instruction_count + 1,
                          sizeof(*block->instructions))) {
        return false;
    }

    block->instructions[block->instruction_count++] = instruction;
    return true;
}
