#include "bytecode.h"
#include "bytecode_internal.h"

#include <stdlib.h>
#include <string.h>

static BytecodeInstructionKind bc_builtin_type_query_kind_for_helper(const char *name) {
    if (!name) {
        return BYTECODE_INSTR_CALL;
    }
    if (strcmp(name, CALYNDA_TYPEOF) == 0) {
        return BYTECODE_INSTR_TYPEOF;
    }
    if (strcmp(name, CALYNDA_ISINT) == 0) {
        return BYTECODE_INSTR_ISINT;
    }
    if (strcmp(name, CALYNDA_ISFLOAT) == 0) {
        return BYTECODE_INSTR_ISFLOAT;
    }
    if (strcmp(name, CALYNDA_ISBOOL) == 0) {
        return BYTECODE_INSTR_ISBOOL;
    }
    if (strcmp(name, CALYNDA_ISSTRING) == 0) {
        return BYTECODE_INSTR_ISSTRING;
    }
    if (strcmp(name, CALYNDA_ISARRAY) == 0) {
        return BYTECODE_INSTR_ISARRAY;
    }
    if (strcmp(name, CALYNDA_ISSAMETYPE) == 0) {
        return BYTECODE_INSTR_ISSAMETYPE;
    }
    return BYTECODE_INSTR_CALL;
}

static bool bc_try_lower_builtin_type_query(BytecodeBuildContext *context,
                                            const MirInstruction *instruction,
                                            BytecodeInstruction *lowered) {
    BytecodeInstructionKind builtin_kind;

    if (!context || !instruction || !lowered ||
        instruction->kind != MIR_INSTR_CALL ||
        instruction->as.call.callee.kind != MIR_VALUE_GLOBAL ||
        !instruction->as.call.has_result) {
        return false;
    }

    builtin_kind = bc_builtin_type_query_kind_for_helper(
        instruction->as.call.callee.as.global_name);
    if (builtin_kind == BYTECODE_INSTR_CALL) {
        return false;
    }

    lowered->kind = builtin_kind;
    if (builtin_kind == BYTECODE_INSTR_ISSAMETYPE) {
        if (instruction->as.call.argument_count != 4) {
            bc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Bytecode lowering expected issametype helper to have 4 arguments.");
            return true;
        }

        lowered->as.same_type_query.dest_temp = instruction->as.call.dest_temp;
        return bc_value_from_mir_value(context,
                                       instruction->as.call.arguments[0],
                                       &lowered->as.same_type_query.left) &&
               bc_value_from_mir_value(context,
                                       instruction->as.call.arguments[1],
                                       &lowered->as.same_type_query.left_type_text) &&
               bc_value_from_mir_value(context,
                                       instruction->as.call.arguments[2],
                                       &lowered->as.same_type_query.right) &&
               bc_value_from_mir_value(context,
                                       instruction->as.call.arguments[3],
                                       &lowered->as.same_type_query.right_type_text);
    }

    if (instruction->as.call.argument_count != 2) {
        bc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Bytecode lowering expected builtin type helper to have 2 arguments.");
        return true;
    }

    lowered->as.builtin_type_query.dest_temp = instruction->as.call.dest_temp;
    return bc_value_from_mir_value(context,
                                   instruction->as.call.arguments[0],
                                   &lowered->as.builtin_type_query.operand) &&
           bc_value_from_mir_value(context,
                                   instruction->as.call.arguments[1],
                                   &lowered->as.builtin_type_query.type_text);
}

bool bc_lower_instruction(BytecodeBuildContext *context,
                          const MirInstruction *instruction,
                          BytecodeInstruction *lowered) {
    size_t i;

    if (!instruction || !lowered) {
        return false;
    }

    memset(lowered, 0, sizeof(*lowered));
    lowered->kind = (BytecodeInstructionKind)instruction->kind;

    switch (instruction->kind) {
    case MIR_INSTR_BINARY:
        lowered->as.binary.dest_temp = instruction->as.binary.dest_temp;
        lowered->as.binary.operator = instruction->as.binary.operator;
        return bc_value_from_mir_value(context,
                                       instruction->as.binary.left,
                                       &lowered->as.binary.left) &&
               bc_value_from_mir_value(context,
                                       instruction->as.binary.right,
                                       &lowered->as.binary.right);

    case MIR_INSTR_UNARY:
        lowered->as.unary.dest_temp = instruction->as.unary.dest_temp;
        lowered->as.unary.operator = instruction->as.unary.operator;
        return bc_value_from_mir_value(context,
                                       instruction->as.unary.operand,
                                       &lowered->as.unary.operand);

    case MIR_INSTR_CLOSURE:
        lowered->as.closure.dest_temp = instruction->as.closure.dest_temp;
        lowered->as.closure.unit_index = bc_find_unit_index(context->mir_program,
                                                            instruction->as.closure.unit_name);
        lowered->as.closure.capture_count = instruction->as.closure.capture_count;
        if (lowered->as.closure.unit_index == (size_t)-1) {
            bc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Bytecode lowering could not find MIR unit %s for closure construction.",
                         instruction->as.closure.unit_name);
            return false;
        }
        if (lowered->as.closure.capture_count > 0) {
            lowered->as.closure.captures = calloc(lowered->as.closure.capture_count,
                                                  sizeof(*lowered->as.closure.captures));
            if (!lowered->as.closure.captures) {
                return false;
            }
        }
        for (i = 0; i < lowered->as.closure.capture_count; i++) {
            if (!bc_value_from_mir_value(context,
                                         instruction->as.closure.captures[i],
                                         &lowered->as.closure.captures[i])) {
                return false;
            }
        }
        return true;

    case MIR_INSTR_CALL:
        if (bc_try_lower_builtin_type_query(context, instruction, lowered)) {
            return !context->program->has_error;
        }

        lowered->as.call.has_result = instruction->as.call.has_result;
        lowered->as.call.dest_temp = instruction->as.call.dest_temp;
        lowered->as.call.argument_count = instruction->as.call.argument_count;
        if (!bc_value_from_mir_value(context,
                                     instruction->as.call.callee,
                                     &lowered->as.call.callee)) {
            return false;
        }
        if (lowered->as.call.argument_count > 0) {
            lowered->as.call.arguments = calloc(lowered->as.call.argument_count,
                                                sizeof(*lowered->as.call.arguments));
            if (!lowered->as.call.arguments) {
                return false;
            }
        }
        for (i = 0; i < lowered->as.call.argument_count; i++) {
            if (!bc_value_from_mir_value(context,
                                         instruction->as.call.arguments[i],
                                         &lowered->as.call.arguments[i])) {
                return false;
            }
        }
        return true;

    case MIR_INSTR_CAST:
        lowered->as.cast.dest_temp = instruction->as.cast.dest_temp;
        lowered->as.cast.target_type = instruction->as.cast.target_type;
        return bc_value_from_mir_value(context,
                                       instruction->as.cast.operand,
                                       &lowered->as.cast.operand);

    case MIR_INSTR_MEMBER:
        lowered->as.member.dest_temp = instruction->as.member.dest_temp;
        lowered->as.member.member_index = bc_intern_text_constant(context,
                                                                  BYTECODE_CONSTANT_SYMBOL,
                                                                  instruction->as.member.member);
        return lowered->as.member.member_index != (size_t)-1 &&
               bc_value_from_mir_value(context,
                                       instruction->as.member.target,
                                       &lowered->as.member.target);

    case MIR_INSTR_UNION_GET_TAG:
        lowered->kind = BYTECODE_INSTR_UNION_GET_TAG;
        lowered->as.union_get_tag.dest_temp = instruction->as.union_get_tag.dest_temp;
        return bc_value_from_mir_value(context,
                                       instruction->as.union_get_tag.target,
                                       &lowered->as.union_get_tag.target);

    case MIR_INSTR_UNION_GET_PAYLOAD:
        lowered->kind = BYTECODE_INSTR_UNION_GET_PAYLOAD;
        lowered->as.union_get_payload.dest_temp = instruction->as.union_get_payload.dest_temp;
        return bc_value_from_mir_value(context,
                                       instruction->as.union_get_payload.target,
                                       &lowered->as.union_get_payload.target);

    case MIR_INSTR_INDEX_LOAD:
        lowered->as.index_load.dest_temp = instruction->as.index_load.dest_temp;
        return bc_value_from_mir_value(context,
                                       instruction->as.index_load.target,
                                       &lowered->as.index_load.target) &&
               bc_value_from_mir_value(context,
                                       instruction->as.index_load.index,
                                       &lowered->as.index_load.index);

    case MIR_INSTR_ARRAY_LITERAL:
        lowered->as.array_literal.dest_temp = instruction->as.array_literal.dest_temp;
        lowered->as.array_literal.element_count = instruction->as.array_literal.element_count;
        if (lowered->as.array_literal.element_count > 0) {
            lowered->as.array_literal.elements = calloc(lowered->as.array_literal.element_count,
                                                        sizeof(*lowered->as.array_literal.elements));
            if (!lowered->as.array_literal.elements) {
                return false;
            }
        }
        for (i = 0; i < lowered->as.array_literal.element_count; i++) {
            if (!bc_value_from_mir_value(context,
                                         instruction->as.array_literal.elements[i],
                                         &lowered->as.array_literal.elements[i])) {
                return false;
            }
        }
        return true;

#include "bytecode_lower_instr_p2.inc"
