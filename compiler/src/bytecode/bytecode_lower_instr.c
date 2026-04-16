#include "bytecode.h"
#include "bytecode_internal.h"

#include <stdlib.h>
#include <string.h>

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

    case MIR_INSTR_TEMPLATE:
        lowered->as.template_literal.dest_temp = instruction->as.template_literal.dest_temp;
        lowered->as.template_literal.part_count = instruction->as.template_literal.part_count;
        if (lowered->as.template_literal.part_count > 0) {
            lowered->as.template_literal.parts = calloc(lowered->as.template_literal.part_count,
                                                        sizeof(*lowered->as.template_literal.parts));
            if (!lowered->as.template_literal.parts) {
                return false;
            }
        }
        for (i = 0; i < lowered->as.template_literal.part_count; i++) {
            if (!bc_lower_template_part(context,
                                        &instruction->as.template_literal.parts[i],
                                        &lowered->as.template_literal.parts[i])) {
                return false;
            }
        }
        return true;

    case MIR_INSTR_STORE_LOCAL:
        lowered->as.store_local.local_index = instruction->as.store_local.local_index;
        return bc_value_from_mir_value(context,
                                       instruction->as.store_local.value,
                                       &lowered->as.store_local.value);

    case MIR_INSTR_STORE_GLOBAL:
        lowered->as.store_global.global_index = bc_intern_text_constant(context,
                                                                        BYTECODE_CONSTANT_SYMBOL,
                                                                        instruction->as.store_global.global_name);
        return lowered->as.store_global.global_index != (size_t)-1 &&
               bc_value_from_mir_value(context,
                                       instruction->as.store_global.value,
                                       &lowered->as.store_global.value);

    case MIR_INSTR_STORE_INDEX:
        return bc_value_from_mir_value(context,
                                       instruction->as.store_index.target,
                                       &lowered->as.store_index.target) &&
               bc_value_from_mir_value(context,
                                       instruction->as.store_index.index,
                                       &lowered->as.store_index.index) &&
               bc_value_from_mir_value(context,
                                       instruction->as.store_index.value,
                                       &lowered->as.store_index.value);

    case MIR_INSTR_STORE_MEMBER:
        lowered->as.store_member.member_index = bc_intern_text_constant(context,
                                                                        BYTECODE_CONSTANT_SYMBOL,
                                                                        instruction->as.store_member.member);
        return lowered->as.store_member.member_index != (size_t)-1 &&
               bc_value_from_mir_value(context,
                                       instruction->as.store_member.target,
                                       &lowered->as.store_member.target) &&
               bc_value_from_mir_value(context,
                                       instruction->as.store_member.value,
                                       &lowered->as.store_member.value);

    case MIR_INSTR_HETERO_ARRAY_NEW:
        return bc_lower_hetero_array_instruction(context, instruction, lowered);

    case MIR_INSTR_UNION_NEW:
        return bc_lower_union_new_instruction(context, instruction, lowered);
    }

    return false;
}
