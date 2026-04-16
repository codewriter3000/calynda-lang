#include "lir.h"
#include "lir_internal.h"

#include <stdlib.h>
#include <string.h>

bool lr_lower_mir_instruction(LirBuildContext *context,
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
        if (!lr_operand_from_mir_value(context, unit,
                                       instruction->as.binary.left,
                                       &lowered.as.binary.left) ||
            !lr_operand_from_mir_value(context, unit,
                                       instruction->as.binary.right,
                                       &lowered.as.binary.right)) {
            lr_instruction_free(&lowered);
            return false;
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR binary instructions.");
            return false;
        }
        return true;

    case MIR_INSTR_UNARY:
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_UNARY;
        lowered.as.unary.dest_vreg = instruction->as.unary.dest_temp;
        lowered.as.unary.operator = instruction->as.unary.operator;
        if (!lr_operand_from_mir_value(context, unit,
                                       instruction->as.unary.operand,
                                       &lowered.as.unary.operand)) {
            lr_instruction_free(&lowered);
            return false;
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
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
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR closures.");
            return false;
        }
        if (instruction->as.closure.capture_count > 0) {
            lowered.as.closure.captures = calloc(instruction->as.closure.capture_count,
                                                 sizeof(*lowered.as.closure.captures));
            if (!lowered.as.closure.captures) {
                lr_instruction_free(&lowered);
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR closures.");
                return false;
            }
        }
        lowered.as.closure.capture_count = instruction->as.closure.capture_count;
        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            if (!lr_operand_from_mir_value(context, unit,
                                           instruction->as.closure.captures[i],
                                           &lowered.as.closure.captures[i])) {
                lr_instruction_free(&lowered);
                return false;
            }
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR closures.");
            return false;
        }
        return true;

    case MIR_INSTR_CALL:
        for (i = 0; i < instruction->as.call.argument_count; i++) {
            memset(&lowered, 0, sizeof(lowered));
            lowered.kind = LIR_INSTR_OUTGOING_ARG;
            lowered.as.outgoing_arg.argument_index = i;
            if (!lr_operand_from_mir_value(context, unit,
                                           instruction->as.call.arguments[i],
                                           &lowered.as.outgoing_arg.value)) {
                lr_instruction_free(&lowered);
                return false;
            }
            if (!lr_append_instruction(block, lowered)) {
                lr_instruction_free(&lowered);
                lr_set_error(context, (AstSourceSpan){0}, NULL,
                             "Out of memory while lowering LIR call arguments.");
                return false;
            }
        }
        memset(&lowered, 0, sizeof(lowered));
        lowered.kind = LIR_INSTR_CALL;
        lowered.as.call.has_result = instruction->as.call.has_result;
        lowered.as.call.dest_vreg = instruction->as.call.dest_temp;
        lowered.as.call.argument_count = instruction->as.call.argument_count;
        if (!lr_operand_from_mir_value(context, unit,
                                       instruction->as.call.callee,
                                       &lowered.as.call.callee)) {
            lr_instruction_free(&lowered);
            return false;
        }
        if (!lr_append_instruction(block, lowered)) {
            lr_instruction_free(&lowered);
            lr_set_error(context, (AstSourceSpan){0}, NULL,
                         "Out of memory while lowering LIR calls.");
            return false;
        }
        return true;

    case MIR_INSTR_CAST:
    case MIR_INSTR_MEMBER:
    case MIR_INSTR_UNION_GET_TAG:
    case MIR_INSTR_UNION_GET_PAYLOAD:
    case MIR_INSTR_INDEX_LOAD:
    case MIR_INSTR_ARRAY_LITERAL:
    case MIR_INSTR_TEMPLATE:
        return lr_lower_mir_instr_ext(context, mir_unit, unit, block, instruction);

    default:
        return lr_lower_mir_instr_stores(context, mir_unit, unit, block, instruction);
    }

    return false;
}
