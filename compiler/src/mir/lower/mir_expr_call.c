#include "mir_internal.h"

#include <stdlib.h>
#include <string.h>

bool mr_lower_call_expression(MirUnitBuildContext *context,
                              const HirExpression *expression,
                              MirValue *value) {
    MirInstruction instruction;
    size_t i;

    /* Payload union variant constructor: Option.Some(42) → union_new */
    {
        const char *call_union_name = NULL;
        const char *call_variant_name = NULL;

        if (mr_is_union_variant_call(expression, &call_union_name, &call_variant_name)) {
            size_t call_variant_index = 0;
            size_t call_variant_count = 0;

            if (!mr_find_hir_union_variant(context->build, call_union_name, call_variant_name,
                                        &call_variant_index, &call_variant_count)) {
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Internal error: union variant '%s.%s' not found in HIR.",
                              call_union_name, call_variant_name);
                return false;
            }

            memset(&instruction, 0, sizeof(instruction));
            instruction.kind = MIR_INSTR_UNION_NEW;
            instruction.as.union_new.dest_temp = context->unit->next_temp_index++;
            instruction.as.union_new.union_name = ast_copy_text(call_union_name);
            instruction.as.union_new.variant_index = call_variant_index;
            instruction.as.union_new.variant_count = call_variant_count;
            instruction.as.union_new.has_payload = (expression->as.call.argument_count > 0);

            if (!instruction.as.union_new.union_name) {
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering MIR union variant call.");
                return false;
            }

            if (instruction.as.union_new.has_payload) {
                if (!mr_lower_expression(context,
                                      expression->as.call.arguments[0],
                                      &instruction.as.union_new.payload)) {
                    mr_instruction_free(&instruction);
                    return false;
                }
            } else {
                memset(&instruction.as.union_new.payload, 0, sizeof(MirValue));
            }

            if (!mr_current_block(context) ||
                !mr_append_instruction(mr_current_block(context), instruction)) {
                mr_instruction_free(&instruction);
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering MIR union variant call.");
                return false;
            }

            value->kind = MIR_VALUE_TEMP;
            value->type = expression->type;
            value->as.temp_index = instruction.as.union_new.dest_temp;
            return true;
        }
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;
    if (!mr_lower_expression(context, expression->as.call.callee, &instruction.as.call.callee)) {
        return false;
    }
    instruction.as.call.has_result = (expression->type.kind != CHECKED_TYPE_VOID);
    if (instruction.as.call.has_result) {
        instruction.as.call.dest_temp = context->unit->next_temp_index++;
    }
    if (expression->as.call.argument_count > 0) {
        instruction.as.call.arguments = calloc(expression->as.call.argument_count,
                                               sizeof(*instruction.as.call.arguments));
        if (!instruction.as.call.arguments) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR calls.");
            return false;
        }
    }
    instruction.as.call.argument_count = expression->as.call.argument_count;
    for (i = 0; i < expression->as.call.argument_count; i++) {
        if (!mr_lower_expression(context,
                              expression->as.call.arguments[i],
                              &instruction.as.call.arguments[i])) {
            mr_instruction_free(&instruction);
            return false;
        }
    }
    if (!mr_current_block(context)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: missing current MIR block after lowering call arguments.");
        return false;
    }
    if (!mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR calls.");
        return false;
    }
    if (instruction.as.call.has_result) {
        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.call.dest_temp;
    } else {
        value->kind = MIR_VALUE_INVALID;
        value->type = expression->type;
    }
    return true;
}

bool mr_lower_memory_op_expression(MirUnitBuildContext *context,
                                   const HirExpression *expression,
                                   MirValue *value) {
    const char *func_name;
    MirInstruction instruction;
    size_t mem_i;

    switch (expression->as.memory_op.kind) {
    case HIR_MEMORY_MALLOC:  func_name = "malloc";  break;
    case HIR_MEMORY_CALLOC:  func_name = "calloc";  break;
    case HIR_MEMORY_REALLOC: func_name = "realloc"; break;
    case HIR_MEMORY_FREE:    func_name = "free";    break;
    default:                 func_name = "malloc";  break;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;

    if (!mr_value_from_global(context->build, func_name,
                               expression->type, &instruction.as.call.callee)) {
        return false;
    }

    instruction.as.call.has_result =
        (expression->as.memory_op.kind != HIR_MEMORY_FREE);
    if (instruction.as.call.has_result) {
        instruction.as.call.dest_temp = context->unit->next_temp_index++;
    }

    instruction.as.call.argument_count = expression->as.memory_op.argument_count;
    if (instruction.as.call.argument_count > 0) {
        instruction.as.call.arguments =
            calloc(instruction.as.call.argument_count,
                   sizeof(*instruction.as.call.arguments));
        if (!instruction.as.call.arguments) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build, expression->source_span, NULL,
                          "Out of memory while lowering MIR memory operation.");
            return false;
        }
        for (mem_i = 0; mem_i < expression->as.memory_op.argument_count; mem_i++) {
            if (!mr_lower_expression(context,
                                      expression->as.memory_op.arguments[mem_i],
                                      &instruction.as.call.arguments[mem_i])) {
                mr_instruction_free(&instruction);
                return false;
            }
        }
    }

    if (!mr_current_block(context)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build, expression->source_span, NULL,
                      "Internal error: missing current MIR block for memory operation.");
        return false;
    }
    if (!mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build, expression->source_span, NULL,
                      "Out of memory while lowering MIR memory operation.");
        return false;
    }

    if (instruction.as.call.has_result) {
        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.call.dest_temp;
    } else {
        value->kind = MIR_VALUE_INVALID;
        value->type = expression->type;
    }
    return true;
}
