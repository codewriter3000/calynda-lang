#include "mir_internal.h"

static bool mr_append_memory_call(MirUnitBuildContext *context,
                                  const HirExpression *expression,
                                  const char *function_name,
                                  size_t extra_argument_count,
                                  size_t element_size,
                                  MirValue *value) {
    MirInstruction instruction;
    size_t memory_index;

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;
    if (!mr_value_from_global(context->build,
                              function_name,
                              expression->type,
                              &instruction.as.call.callee)) {
        return false;
    }

    instruction.as.call.has_result =
        expression->as.memory_op.kind != HIR_MEMORY_FREE &&
        expression->as.memory_op.kind != HIR_MEMORY_STORE;
    if (instruction.as.call.has_result) {
        instruction.as.call.dest_temp = context->unit->next_temp_index++;
    }

    instruction.as.call.argument_count =
        expression->as.memory_op.argument_count + extra_argument_count;
    if (instruction.as.call.argument_count > 0) {
        instruction.as.call.arguments = calloc(instruction.as.call.argument_count,
                                               sizeof(*instruction.as.call.arguments));
        if (!instruction.as.call.arguments) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                         expression->source_span,
                         NULL,
                         "Out of memory while lowering MIR memory operation.");
            return false;
        }
    }

    for (memory_index = 0;
         memory_index < expression->as.memory_op.argument_count;
         memory_index++) {
        if (!mr_lower_expression(context,
                                 expression->as.memory_op.arguments[memory_index],
                                 &instruction.as.call.arguments[memory_index])) {
            mr_instruction_free(&instruction);
            return false;
        }
    }

    if (extra_argument_count > 0) {
        char element_size_text[24];
        CheckedType int64_type;
        MirValue *extra_argument =
            &instruction.as.call.arguments[expression->as.memory_op.argument_count];

        memset(&int64_type, 0, sizeof(int64_type));
        int64_type.kind = CHECKED_TYPE_VALUE;
        int64_type.primitive = AST_PRIMITIVE_INT64;
        extra_argument->kind = MIR_VALUE_LITERAL;
        extra_argument->type = int64_type;
        extra_argument->as.literal.kind = AST_LITERAL_INTEGER;
        snprintf(element_size_text, sizeof(element_size_text), "%zu", element_size);
        extra_argument->as.literal.text = ast_copy_text(element_size_text);
        if (!extra_argument->as.literal.text) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                         expression->source_span,
                         NULL,
                         "Out of memory while lowering typed pointer operation.");
            return false;
        }
    }

    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                     expression->source_span,
                     NULL,
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

bool mr_lower_memory_op_expression(MirUnitBuildContext *context,
                                   const HirExpression *expression,
                                   MirValue *value) {
    const char *function_name;
    const char *stackalloc_cleanup_name;
    MirValue argument_value;
    MirValue callee_value;
    size_t element_size;
    size_t extra_argument_count;
    bool use_bounds_checks;

    use_bounds_checks = context->in_checked_manual || expression->as.memory_op.is_checked_ptr;
    element_size = expression->as.memory_op.element_size;
    extra_argument_count = 0;

    if (expression->as.memory_op.kind == HIR_MEMORY_CLEANUP) {
        if (!mr_lower_expression(context, expression->as.memory_op.arguments[0], &argument_value) ||
            !mr_lower_expression(context, expression->as.memory_op.arguments[1], &callee_value)) {
            return false;
        }

        if (!mr_register_manual_cleanup(context,
                        &argument_value,
                        &callee_value,
                        expression->source_span,
                        value)) {
            mr_value_free(&argument_value);
            mr_value_free(&callee_value);
            return false;
        }

        mr_value_free(&argument_value);
        mr_value_free(&callee_value);
        return true;
    }

    switch (expression->as.memory_op.kind) {
    case HIR_MEMORY_MALLOC:
        function_name = use_bounds_checks ? "__calynda_bc_malloc" : "malloc";
        break;
    case HIR_MEMORY_CALLOC:
        function_name = use_bounds_checks ? "__calynda_bc_calloc" : "calloc";
        break;
    case HIR_MEMORY_REALLOC:
        function_name = use_bounds_checks ? "__calynda_bc_realloc" : "realloc";
        break;
    case HIR_MEMORY_FREE:
        function_name = use_bounds_checks ? "__calynda_bc_free" : "free";
        break;
    case HIR_MEMORY_ADDR:
        function_name = "__calynda_addr";
        break;
    case HIR_MEMORY_STACKALLOC:
        function_name = use_bounds_checks ? "__calynda_bc_stackalloc" : "__calynda_stackalloc";
        break;
    case HIR_MEMORY_DEREF:
        if (use_bounds_checks) {
            function_name = "__calynda_bc_deref";
        } else if (element_size != 0 && element_size != 8) {
            function_name = "__calynda_deref_sized";
            extra_argument_count = 1;
        } else {
            function_name = "__calynda_deref";
        }
        break;
    case HIR_MEMORY_OFFSET:
        if (use_bounds_checks) {
            function_name = "__calynda_bc_offset";
            if (element_size != 0) {
                extra_argument_count = 1;
            }
        } else if (element_size != 0) {
            function_name = "__calynda_offset_stride";
            extra_argument_count = 1;
        } else {
            function_name = "__calynda_offset";
        }
        break;
    case HIR_MEMORY_STORE:
        if (use_bounds_checks) {
            function_name = "__calynda_bc_store";
        } else if (element_size != 0 && element_size != 8) {
            function_name = "__calynda_store_sized";
            extra_argument_count = 1;
        } else {
            function_name = "__calynda_store";
        }
        break;
    default:
        function_name = "malloc";
        break;
    }

    if (!mr_append_memory_call(context,
                               expression,
                               function_name,
                               extra_argument_count,
                               element_size,
                               value)) {
        return false;
    }

    if (expression->as.memory_op.kind != HIR_MEMORY_STACKALLOC ||
        context->manual_cleanup_depth == 0) {
        return true;
    }

    stackalloc_cleanup_name = use_bounds_checks ? "__calynda_bc_free" : "free";
    if (!mr_value_from_global(context->build,
                              stackalloc_cleanup_name,
                              mr_checked_type_void_value(),
                              &callee_value)) {
        mr_value_free(value);
        return false;
    }

    argument_value = *value;
    if (!mr_register_manual_cleanup(context,
                                    &argument_value,
                                    &callee_value,
                                    expression->source_span,
                                    value)) {
        mr_value_free(&callee_value);
        mr_value_free(value);
        return false;
    }

    mr_value_free(&callee_value);
    return true;
}