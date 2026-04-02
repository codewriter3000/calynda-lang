#include "lir.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    LirProgram      *program;
    const MirProgram *mir_program;
} LirBuildContext;

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size);
static bool source_span_is_valid(AstSourceSpan span);
static void lir_set_error(LirBuildContext *context,
                          AstSourceSpan primary_span,
                          const AstSourceSpan *related_span,
                          const char *format,
                          ...);
static LirOperand lir_invalid_operand(void);
static bool lir_operand_from_mir_value(LirBuildContext *context,
                                       const LirUnit *unit,
                                       MirValue value,
                                       LirOperand *operand);
static void lir_operand_free(LirOperand *operand);
static void lir_template_part_free(LirTemplatePart *part);
static void lir_instruction_free(LirInstruction *instruction);
static void lir_basic_block_free(LirBasicBlock *block);
static void lir_unit_free(LirUnit *unit);
static bool append_unit(LirProgram *program, LirUnit unit);
static bool append_slot(LirUnit *unit, LirSlot slot);
static bool append_block(LirUnit *unit, LirBasicBlock block);
static bool append_instruction(LirBasicBlock *block, LirInstruction instruction);
static LirSlotKind slot_kind_from_mir_kind(MirLocalKind kind);
static LirUnitKind unit_kind_from_mir_kind(MirUnitKind kind);
static bool emit_entry_abi(LirBuildContext *context,
                           const LirUnit *unit,
                           LirBasicBlock *block);
static bool lower_mir_instruction(LirBuildContext *context,
                                  const MirUnit *mir_unit,
                                  const LirUnit *unit,
                                  LirBasicBlock *block,
                                  const MirInstruction *instruction);
static bool lower_mir_terminator(LirBuildContext *context,
                                 const LirUnit *unit,
                                 LirBasicBlock *block,
                                 const MirTerminator *terminator);
static bool lower_mir_block(LirBuildContext *context,
                            const MirUnit *mir_unit,
                            const LirUnit *unit,
                            const MirBasicBlock *mir_block,
                            bool is_entry_block,
                            LirBasicBlock *block);
static bool lower_mir_unit(LirBuildContext *context,
                           const MirUnit *mir_unit,
                           LirUnit *unit);

void lir_program_init(LirProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
}

void lir_program_free(LirProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->unit_count; i++) {
        lir_unit_free(&program->units[i]);
    }
    free(program->units);
    memset(program, 0, sizeof(*program));
}

const LirBuildError *lir_get_error(const LirProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool lir_format_error(const LirBuildError *error,
                      char *buffer,
                      size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (source_span_is_valid(error->primary_span)) {
        if (error->has_related_span && source_span_is_valid(error->related_span)) {
            written = snprintf(buffer,
                               buffer_size,
                               "%d:%d: %s Related location at %d:%d.",
                               error->primary_span.start_line,
                               error->primary_span.start_column,
                               error->message,
                               error->related_span.start_line,
                               error->related_span.start_column);
        } else {
            written = snprintf(buffer,
                               buffer_size,
                               "%d:%d: %s",
                               error->primary_span.start_line,
                               error->primary_span.start_column,
                               error->message);
        }
    } else {
        written = snprintf(buffer, buffer_size, "%s", error->message);
    }

    return written >= 0 && (size_t)written < buffer_size;
}

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size) {
    void *resized;
    size_t new_capacity;

    if (*capacity >= needed) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 4 : *capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }

    resized = realloc(*items, new_capacity * item_size);
    if (!resized) {
        return false;
    }

    *items = resized;
    *capacity = new_capacity;
    return true;
}

static bool source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

static void lir_set_error(LirBuildContext *context,
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
    if (related_span && source_span_is_valid(*related_span)) {
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

static LirOperand lir_invalid_operand(void) {
    LirOperand operand;

    memset(&operand, 0, sizeof(operand));
    operand.kind = LIR_OPERAND_INVALID;
    return operand;
}

static bool lir_operand_from_mir_value(LirBuildContext *context,
                                       const LirUnit *unit,
                                       MirValue value,
                                       LirOperand *operand) {
    if (!operand) {
        return false;
    }

    *operand = lir_invalid_operand();
    operand->type = value.type;
    switch (value.kind) {
    case MIR_VALUE_INVALID:
        return true;

    case MIR_VALUE_TEMP:
        if (!unit || value.as.temp_index >= unit->virtual_register_count) {
            lir_set_error(context,
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
            lir_set_error(context,
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
            lir_set_error(context,
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
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR literals.");
            return false;
        }
        return true;
    }

    return false;
}

static void lir_operand_free(LirOperand *operand) {
    if (!operand) {
        return;
    }

    if (operand->kind == LIR_OPERAND_GLOBAL) {
        free(operand->as.global_name);
    } else if (operand->kind == LIR_OPERAND_LITERAL &&
               operand->as.literal.kind != AST_LITERAL_BOOL &&
               operand->as.literal.kind != AST_LITERAL_NULL) {
        free(operand->as.literal.text);
    }

    memset(operand, 0, sizeof(*operand));
}

static void lir_template_part_free(LirTemplatePart *part) {
    if (!part) {
        return;
    }

    if (part->kind == LIR_TEMPLATE_PART_TEXT) {
        free(part->as.text);
    } else {
        lir_operand_free(&part->as.value);
    }

    memset(part, 0, sizeof(*part));
}

static void lir_instruction_free(LirInstruction *instruction) {
    size_t i;

    if (!instruction) {
        return;
    }

    switch (instruction->kind) {
    case LIR_INSTR_INCOMING_ARG:
    case LIR_INSTR_INCOMING_CAPTURE:
        break;
    case LIR_INSTR_OUTGOING_ARG:
        lir_operand_free(&instruction->as.outgoing_arg.value);
        break;
    case LIR_INSTR_BINARY:
        lir_operand_free(&instruction->as.binary.left);
        lir_operand_free(&instruction->as.binary.right);
        break;
    case LIR_INSTR_UNARY:
        lir_operand_free(&instruction->as.unary.operand);
        break;
    case LIR_INSTR_CLOSURE:
        free(instruction->as.closure.unit_name);
        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            lir_operand_free(&instruction->as.closure.captures[i]);
        }
        free(instruction->as.closure.captures);
        break;
    case LIR_INSTR_CALL:
        lir_operand_free(&instruction->as.call.callee);
        break;
    case LIR_INSTR_CAST:
        lir_operand_free(&instruction->as.cast.operand);
        break;
    case LIR_INSTR_MEMBER:
        lir_operand_free(&instruction->as.member.target);
        free(instruction->as.member.member);
        break;
    case LIR_INSTR_INDEX_LOAD:
        lir_operand_free(&instruction->as.index_load.target);
        lir_operand_free(&instruction->as.index_load.index);
        break;
    case LIR_INSTR_ARRAY_LITERAL:
        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            lir_operand_free(&instruction->as.array_literal.elements[i]);
        }
        free(instruction->as.array_literal.elements);
        break;
    case LIR_INSTR_TEMPLATE:
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            lir_template_part_free(&instruction->as.template_literal.parts[i]);
        }
        free(instruction->as.template_literal.parts);
        break;
    case LIR_INSTR_STORE_SLOT:
        lir_operand_free(&instruction->as.store_slot.value);
        break;
    case LIR_INSTR_STORE_GLOBAL:
        free(instruction->as.store_global.global_name);
        lir_operand_free(&instruction->as.store_global.value);
        break;
    case LIR_INSTR_STORE_INDEX:
        lir_operand_free(&instruction->as.store_index.target);
        lir_operand_free(&instruction->as.store_index.index);
        lir_operand_free(&instruction->as.store_index.value);
        break;
    case LIR_INSTR_STORE_MEMBER:
        lir_operand_free(&instruction->as.store_member.target);
        free(instruction->as.store_member.member);
        lir_operand_free(&instruction->as.store_member.value);
        break;
    }

    memset(instruction, 0, sizeof(*instruction));
}

static void lir_basic_block_free(LirBasicBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    free(block->label);
    for (i = 0; i < block->instruction_count; i++) {
        lir_instruction_free(&block->instructions[i]);
    }
    free(block->instructions);
    switch (block->terminator.kind) {
    case LIR_TERM_RETURN:
        if (block->terminator.as.return_term.has_value) {
            lir_operand_free(&block->terminator.as.return_term.value);
        }
        break;
    case LIR_TERM_BRANCH:
        lir_operand_free(&block->terminator.as.branch_term.condition);
        break;
    case LIR_TERM_THROW:
        lir_operand_free(&block->terminator.as.throw_term.value);
        break;
    case LIR_TERM_NONE:
    case LIR_TERM_JUMP:
        break;
    }
    memset(block, 0, sizeof(*block));
}

static void lir_unit_free(LirUnit *unit) {
    size_t i;

    if (!unit) {
        return;
    }

    free(unit->name);
    for (i = 0; i < unit->slot_count; i++) {
        free(unit->slots[i].name);
    }
    free(unit->slots);
    for (i = 0; i < unit->block_count; i++) {
        lir_basic_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    memset(unit, 0, sizeof(*unit));
}

static bool append_unit(LirProgram *program, LirUnit unit) {
    if (!reserve_items((void **)&program->units,
                       &program->unit_capacity,
                       program->unit_count + 1,
                       sizeof(*program->units))) {
        return false;
    }

    program->units[program->unit_count++] = unit;
    return true;
}

static bool append_slot(LirUnit *unit, LirSlot slot) {
    if (!reserve_items((void **)&unit->slots,
                       &unit->slot_capacity,
                       unit->slot_count + 1,
                       sizeof(*unit->slots))) {
        return false;
    }

    unit->slots[unit->slot_count++] = slot;
    return true;
}

static bool append_block(LirUnit *unit, LirBasicBlock block) {
    if (!reserve_items((void **)&unit->blocks,
                       &unit->block_capacity,
                       unit->block_count + 1,
                       sizeof(*unit->blocks))) {
        return false;
    }

    unit->blocks[unit->block_count++] = block;
    return true;
}

static bool append_instruction(LirBasicBlock *block, LirInstruction instruction) {
    if (!reserve_items((void **)&block->instructions,
                       &block->instruction_capacity,
                       block->instruction_count + 1,
                       sizeof(*block->instructions))) {
        return false;
    }

    block->instructions[block->instruction_count++] = instruction;
    return true;
}

static LirSlotKind slot_kind_from_mir_kind(MirLocalKind kind) {
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

static LirUnitKind unit_kind_from_mir_kind(MirUnitKind kind) {
    switch (kind) {
    case MIR_UNIT_START:
        return LIR_UNIT_START;
    case MIR_UNIT_BINDING:
        return LIR_UNIT_BINDING;
    case MIR_UNIT_INIT:
        return LIR_UNIT_INIT;
    case MIR_UNIT_LAMBDA:
        return LIR_UNIT_LAMBDA;
    }

    return LIR_UNIT_BINDING;
}

static bool emit_entry_abi(LirBuildContext *context,
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
        if (!append_instruction(block, instruction)) {
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
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
        if (!append_instruction(block, instruction)) {
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR incoming arguments.");
            return false;
        }
    }

    return true;
}

static bool lower_mir_instruction(LirBuildContext *context,
                                  const MirUnit *mir_unit,
                                  const LirUnit *unit,
                                  LirBasicBlock *block,
                                  const MirInstruction *instruction) {
    LirInstruction lowered;
    size_t i;

    (void)mir_unit;

    if (!context || !unit || !block || !instruction) {
        return false;
    }

    switch (instruction->kind) {
    case MIR_INSTR_BINARY:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_BINARY;
        lowered.as.binary.dest_vreg = instruction->as.binary.dest_temp;
        lowered.as.binary.operator = instruction->as.binary.operator;
        if (!lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.binary.left,
                                        &lowered.as.binary.left) ||
            !lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.binary.right,
                                        &lowered.as.binary.right)) {
            lir_instruction_free(&lowered);
            return false;
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR binary instructions.");
            return false;
        }
        return true;

    case MIR_INSTR_UNARY:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_UNARY;
        lowered.as.unary.dest_vreg = instruction->as.unary.dest_temp;
        lowered.as.unary.operator = instruction->as.unary.operator;
        if (!lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.unary.operand,
                                        &lowered.as.unary.operand)) {
            lir_instruction_free(&lowered);
            return false;
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR unary instructions.");
            return false;
        }
        return true;

    case MIR_INSTR_CLOSURE:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_CLOSURE;
        lowered.as.closure.dest_vreg = instruction->as.closure.dest_temp;
        lowered.as.closure.unit_name = ast_copy_text(instruction->as.closure.unit_name);
        if (!lowered.as.closure.unit_name) {
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR closures.");
            return false;
        }
        if (instruction->as.closure.capture_count > 0) {
            lowered.as.closure.captures = calloc(instruction->as.closure.capture_count,
                                                 sizeof(*lowered.as.closure.captures));
            if (!lowered.as.closure.captures) {
                lir_instruction_free(&lowered);
                lir_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while lowering LIR closures.");
                return false;
            }
        }
        lowered.as.closure.capture_count = instruction->as.closure.capture_count;
        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            if (!lir_operand_from_mir_value(context,
                                            unit,
                                            instruction->as.closure.captures[i],
                                            &lowered.as.closure.captures[i])) {
                lir_instruction_free(&lowered);
                return false;
            }
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR closures.");
            return false;
        }
        return true;

    case MIR_INSTR_CALL:
        for (i = 0; i < instruction->as.call.argument_count; i++) {
            memset(&lowered, 0, sizeof(lowered));
            lowered.kind = LIR_INSTR_OUTGOING_ARG;
            lowered.as.outgoing_arg.argument_index = i;
            if (!lir_operand_from_mir_value(context,
                                            unit,
                                            instruction->as.call.arguments[i],
                                            &lowered.as.outgoing_arg.value)) {
                lir_instruction_free(&lowered);
                return false;
            }
            if (!append_instruction(block, lowered)) {
                lir_instruction_free(&lowered);
                lir_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while lowering LIR call arguments.");
                return false;
            }
        }

        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_CALL;
        lowered.as.call.has_result = instruction->as.call.has_result;
        lowered.as.call.dest_vreg = instruction->as.call.dest_temp;
        lowered.as.call.argument_count = instruction->as.call.argument_count;
        if (!lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.call.callee,
                                        &lowered.as.call.callee)) {
            lir_instruction_free(&lowered);
            return false;
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR calls.");
            return false;
        }
        return true;

    case MIR_INSTR_CAST:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_CAST;
        lowered.as.cast.dest_vreg = instruction->as.cast.dest_temp;
        lowered.as.cast.target_type = instruction->as.cast.target_type;
        if (!lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.cast.operand,
                                        &lowered.as.cast.operand)) {
            lir_instruction_free(&lowered);
            return false;
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR casts.");
            return false;
        }
        return true;

    case MIR_INSTR_MEMBER:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_MEMBER;
        lowered.as.member.dest_vreg = instruction->as.member.dest_temp;
        lowered.as.member.member = ast_copy_text(instruction->as.member.member);
        if (!lowered.as.member.member ||
            !lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.member.target,
                                        &lowered.as.member.target)) {
            lir_instruction_free(&lowered);
            if (!context->program->has_error) {
                lir_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while lowering LIR member instructions.");
            }
            return false;
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR member instructions.");
            return false;
        }
        return true;

    case MIR_INSTR_INDEX_LOAD:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_INDEX_LOAD;
        lowered.as.index_load.dest_vreg = instruction->as.index_load.dest_temp;
        if (!lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.index_load.target,
                                        &lowered.as.index_load.target) ||
            !lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.index_load.index,
                                        &lowered.as.index_load.index)) {
            lir_instruction_free(&lowered);
            return false;
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR index loads.");
            return false;
        }
        return true;

    case MIR_INSTR_ARRAY_LITERAL:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_ARRAY_LITERAL;
        lowered.as.array_literal.dest_vreg = instruction->as.array_literal.dest_temp;
        if (instruction->as.array_literal.element_count > 0) {
            lowered.as.array_literal.elements = calloc(instruction->as.array_literal.element_count,
                                                       sizeof(*lowered.as.array_literal.elements));
            if (!lowered.as.array_literal.elements) {
                lir_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while lowering LIR array literals.");
                return false;
            }
        }
        lowered.as.array_literal.element_count = instruction->as.array_literal.element_count;
        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            if (!lir_operand_from_mir_value(context,
                                            unit,
                                            instruction->as.array_literal.elements[i],
                                            &lowered.as.array_literal.elements[i])) {
                lir_instruction_free(&lowered);
                return false;
            }
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR array literals.");
            return false;
        }
        return true;

    case MIR_INSTR_TEMPLATE:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_TEMPLATE;
        lowered.as.template_literal.dest_vreg = instruction->as.template_literal.dest_temp;
        if (instruction->as.template_literal.part_count > 0) {
            lowered.as.template_literal.parts = calloc(instruction->as.template_literal.part_count,
                                                       sizeof(*lowered.as.template_literal.parts));
            if (!lowered.as.template_literal.parts) {
                lir_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while lowering LIR templates.");
                return false;
            }
        }
        lowered.as.template_literal.part_count = instruction->as.template_literal.part_count;
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            lowered.as.template_literal.parts[i].kind =
                instruction->as.template_literal.parts[i].kind == MIR_TEMPLATE_PART_TEXT
                    ? LIR_TEMPLATE_PART_TEXT
                    : LIR_TEMPLATE_PART_VALUE;
            if (lowered.as.template_literal.parts[i].kind == LIR_TEMPLATE_PART_TEXT) {
                lowered.as.template_literal.parts[i].as.text =
                    ast_copy_text(instruction->as.template_literal.parts[i].as.text);
                if (!lowered.as.template_literal.parts[i].as.text) {
                    lir_instruction_free(&lowered);
                    lir_set_error(context,
                                  (AstSourceSpan){0},
                                  NULL,
                                  "Out of memory while lowering LIR templates.");
                    return false;
                }
            } else if (!lir_operand_from_mir_value(context,
                                                   unit,
                                                   instruction->as.template_literal.parts[i].as.value,
                                                   &lowered.as.template_literal.parts[i].as.value)) {
                lir_instruction_free(&lowered);
                return false;
            }
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR templates.");
            return false;
        }
        return true;

    case MIR_INSTR_STORE_LOCAL:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_STORE_SLOT;
        lowered.as.store_slot.slot_index = instruction->as.store_local.local_index;
        if (!lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.store_local.value,
                                        &lowered.as.store_slot.value)) {
            lir_instruction_free(&lowered);
            return false;
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR slot stores.");
            return false;
        }
        return true;

    case MIR_INSTR_STORE_GLOBAL:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_STORE_GLOBAL;
        lowered.as.store_global.global_name = ast_copy_text(instruction->as.store_global.global_name);
        if (!lowered.as.store_global.global_name ||
            !lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.store_global.value,
                                        &lowered.as.store_global.value)) {
            lir_instruction_free(&lowered);
            if (!context->program->has_error) {
                lir_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while lowering LIR global stores.");
            }
            return false;
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR global stores.");
            return false;
        }
        return true;

    case MIR_INSTR_STORE_INDEX:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_STORE_INDEX;
        if (!lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.store_index.target,
                                        &lowered.as.store_index.target) ||
            !lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.store_index.index,
                                        &lowered.as.store_index.index) ||
            !lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.store_index.value,
                                        &lowered.as.store_index.value)) {
            lir_instruction_free(&lowered);
            return false;
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR index stores.");
            return false;
        }
        return true;

    case MIR_INSTR_STORE_MEMBER:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_STORE_MEMBER;
        lowered.as.store_member.member = ast_copy_text(instruction->as.store_member.member);
        if (!lowered.as.store_member.member ||
            !lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.store_member.target,
                                        &lowered.as.store_member.target) ||
            !lir_operand_from_mir_value(context,
                                        unit,
                                        instruction->as.store_member.value,
                                        &lowered.as.store_member.value)) {
            lir_instruction_free(&lowered);
            if (!context->program->has_error) {
                lir_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while lowering LIR member stores.");
            }
            return false;
        }
        if (!append_instruction(block, lowered)) {
            lir_instruction_free(&lowered);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR member stores.");
            return false;
        }
        return true;
    }

    return false;
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
        return lir_operand_from_mir_value(context,
                                          unit,
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
        return lir_operand_from_mir_value(context,
                                          unit,
                                          terminator->as.branch_term.condition,
                                          &block->terminator.as.branch_term.condition);

    case MIR_TERM_THROW:
        block->terminator.kind = LIR_TERM_THROW;
        return lir_operand_from_mir_value(context,
                                          unit,
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
        lir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering LIR blocks.");
        return false;
    }

    if (is_entry_block && !emit_entry_abi(context, unit, block)) {
        lir_basic_block_free(block);
        return false;
    }

    for (i = 0; i < mir_block->instruction_count; i++) {
        if (!lower_mir_instruction(context,
                                   mir_unit,
                                   unit,
                                   block,
                                   &mir_block->instructions[i])) {
            lir_basic_block_free(block);
            return false;
        }
    }

    if (!lower_mir_terminator(context, unit, block, &mir_block->terminator)) {
        lir_basic_block_free(block);
        return false;
    }

    return true;
}

static bool lower_mir_unit(LirBuildContext *context,
                           const MirUnit *mir_unit,
                           LirUnit *unit) {
    size_t i;

    if (!context || !mir_unit || !unit) {
        return false;
    }

    memset(unit, 0, sizeof(*unit));
    unit->kind = unit_kind_from_mir_kind(mir_unit->kind);
    unit->name = ast_copy_text(mir_unit->name);
    unit->symbol = mir_unit->symbol;
    unit->return_type = mir_unit->return_type;
    unit->parameter_count = mir_unit->parameter_count;
    unit->virtual_register_count = mir_unit->next_temp_index;
    if (!unit->name) {
        lir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering LIR units.");
        return false;
    }

    for (i = 0; i < mir_unit->local_count; i++) {
        LirSlot slot;

        memset(&slot, 0, sizeof(slot));
        slot.kind = slot_kind_from_mir_kind(mir_unit->locals[i].kind);
        slot.name = ast_copy_text(mir_unit->locals[i].name);
        slot.symbol = mir_unit->locals[i].symbol;
        slot.type = mir_unit->locals[i].type;
        slot.is_final = mir_unit->locals[i].is_final;
        slot.index = mir_unit->locals[i].index;
        if (!slot.name || !append_slot(unit, slot)) {
            free(slot.name);
            lir_unit_free(unit);
            lir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while lowering LIR slots.");
            return false;
        }
        if (slot.kind == LIR_SLOT_CAPTURE) {
            unit->capture_count++;
        }
    }

    for (i = 0; i < mir_unit->block_count; i++) {
        LirBasicBlock block;

        if (!lower_mir_block(context,
                             mir_unit,
                             unit,
                             &mir_unit->blocks[i],
                             i == 0,
                             &block) ||
            !append_block(unit, block)) {
            lir_basic_block_free(&block);
            lir_unit_free(unit);
            if (!context->program->has_error) {
                lir_set_error(context,
                              (AstSourceSpan){0},
                              NULL,
                              "Out of memory while assembling LIR blocks.");
            }
            return false;
        }
    }

    return true;
}

bool lir_build_program(LirProgram *program, const MirProgram *mir_program) {
    LirBuildContext context;
    size_t i;

    if (!program || !mir_program) {
        return false;
    }

    lir_program_free(program);
    lir_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.mir_program = mir_program;

    if (mir_get_error(mir_program) != NULL) {
        lir_set_error(&context,
                      (AstSourceSpan){0},
                      NULL,
                      "Cannot lower program to LIR while the MIR reports errors.");
        return false;
    }

    for (i = 0; i < mir_program->unit_count; i++) {
        LirUnit unit;

        if (!lower_mir_unit(&context, &mir_program->units[i], &unit)) {
            return false;
        }
        if (!append_unit(program, unit)) {
            lir_unit_free(&unit);
            lir_set_error(&context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while assembling LIR units.");
            return false;
        }
    }

    return true;
}