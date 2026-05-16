#include "mir_internal.h"

static bool mr_append_memory_deref_size_argument(MirUnitBuildContext *context,
                                                 MirValue *argument,
                                                 size_t element_size,
                                                 AstSourceSpan source_span) {
    char element_size_text[24];
    CheckedType int64_type;

    memset(&int64_type, 0, sizeof(int64_type));
    int64_type.kind = CHECKED_TYPE_VALUE;
    int64_type.primitive = AST_PRIMITIVE_INT64;
    argument->kind = MIR_VALUE_LITERAL;
    argument->type = int64_type;
    argument->as.literal.kind = AST_LITERAL_INTEGER;
    snprintf(element_size_text, sizeof(element_size_text), "%zu", element_size);
    argument->as.literal.text = ast_copy_text(element_size_text);
    if (!argument->as.literal.text) {
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "Out of memory while lowering MIR memory access size.");
        return false;
    }
    return true;
}

static bool mr_load_memory_deref_lvalue(MirUnitBuildContext *context,
                                        const MirLValue *lvalue,
                                        AstSourceSpan source_span,
                                        MirValue *value) {
    MirInstruction instruction;
    const char *function_name;
    size_t argument_count = 1;

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;
    if (lvalue->as.memory_deref.is_mmio) {
        if (lvalue->as.memory_deref.element_size != 0 &&
            lvalue->as.memory_deref.element_size != 8) {
            function_name = "__calynda_mmio_deref_sized";
            argument_count = 2;
        } else {
            function_name = "__calynda_mmio_deref";
        }
    } else if (lvalue->as.memory_deref.is_checked_ptr) {
        function_name = "__calynda_bc_deref";
    } else if (lvalue->as.memory_deref.element_size != 0 &&
               lvalue->as.memory_deref.element_size != 8) {
        function_name = "__calynda_deref_sized";
        argument_count = 2;
    } else {
        function_name = "__calynda_deref";
    }

    if (!mr_value_from_global(context->build,
                              function_name,
                              lvalue->type,
                              &instruction.as.call.callee)) {
        return false;
    }

    instruction.as.call.has_result = true;
    instruction.as.call.dest_temp = context->unit->next_temp_index++;
    instruction.as.call.argument_count = argument_count;
    instruction.as.call.arguments = calloc(argument_count,
                                           sizeof(*instruction.as.call.arguments));
    if (!instruction.as.call.arguments) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "Out of memory while lowering MIR memory dereference.");
        return false;
    }

    if (!mr_value_clone(context->build,
                        &lvalue->as.memory_deref.target,
                        &instruction.as.call.arguments[0])) {
        mr_instruction_free(&instruction);
        return false;
    }

    if (argument_count == 2 &&
        !mr_append_memory_deref_size_argument(context,
                                              &instruction.as.call.arguments[1],
                                              lvalue->as.memory_deref.element_size,
                                              source_span)) {
        mr_instruction_free(&instruction);
        return false;
    }

    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "Out of memory while lowering MIR memory dereference.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = lvalue->type;
    value->as.temp_index = instruction.as.call.dest_temp;
    return true;
}

static bool mr_store_memory_deref_lvalue(MirUnitBuildContext *context,
                                         const MirLValue *lvalue,
                                         MirValue value,
                                         AstSourceSpan source_span) {
    MirInstruction instruction;
    const char *function_name;
    size_t argument_count = 2;

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;
    if (lvalue->as.memory_deref.is_mmio) {
        if (lvalue->as.memory_deref.element_size != 0 &&
            lvalue->as.memory_deref.element_size != 8) {
            function_name = "__calynda_mmio_store_sized";
            argument_count = 3;
        } else {
            function_name = "__calynda_mmio_store";
        }
    } else if (lvalue->as.memory_deref.is_checked_ptr) {
        function_name = "__calynda_bc_store";
    } else if (lvalue->as.memory_deref.element_size != 0 &&
               lvalue->as.memory_deref.element_size != 8) {
        function_name = "__calynda_store_sized";
        argument_count = 3;
    } else {
        function_name = "__calynda_store";
    }

    if (!mr_value_from_global(context->build,
                              function_name,
                              mr_checked_type_void_value(),
                              &instruction.as.call.callee)) {
        mr_value_free(&value);
        return false;
    }

    instruction.as.call.has_result = false;
    instruction.as.call.argument_count = argument_count;
    instruction.as.call.arguments = calloc(argument_count,
                                           sizeof(*instruction.as.call.arguments));
    if (!instruction.as.call.arguments) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "Out of memory while lowering MIR memory store.");
        mr_value_free(&value);
        return false;
    }

    if (!mr_value_clone(context->build,
                        &lvalue->as.memory_deref.target,
                        &instruction.as.call.arguments[0])) {
        instruction.as.call.arguments[1] = value;
        mr_instruction_free(&instruction);
        return false;
    }
    instruction.as.call.arguments[1] = value;

    if (argument_count == 3 &&
        !mr_append_memory_deref_size_argument(context,
                                              &instruction.as.call.arguments[2],
                                              lvalue->as.memory_deref.element_size,
                                              source_span)) {
        mr_instruction_free(&instruction);
        return false;
    }

    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                     source_span,
                     NULL,
                     "Out of memory while lowering MIR memory store.");
        return false;
    }

    return true;
}

bool mr_map_compound_assignment(AstAssignmentOperator operator,
                                AstBinaryOperator *binary_operator) {
    if (!binary_operator) {
        return false;
    }
    switch (operator) {
    case AST_ASSIGN_OP_ADD:
        *binary_operator = AST_BINARY_OP_ADD;
        return true;
    case AST_ASSIGN_OP_SUBTRACT:
        *binary_operator = AST_BINARY_OP_SUBTRACT;
        return true;
    case AST_ASSIGN_OP_MULTIPLY:
        *binary_operator = AST_BINARY_OP_MULTIPLY;
        return true;
    case AST_ASSIGN_OP_DIVIDE:
        *binary_operator = AST_BINARY_OP_DIVIDE;
        return true;
    case AST_ASSIGN_OP_MODULO:
        *binary_operator = AST_BINARY_OP_MODULO;
        return true;
    case AST_ASSIGN_OP_BIT_AND:
        *binary_operator = AST_BINARY_OP_BIT_AND;
        return true;
    case AST_ASSIGN_OP_BIT_OR:
        *binary_operator = AST_BINARY_OP_BIT_OR;
        return true;
    case AST_ASSIGN_OP_BIT_XOR:
        *binary_operator = AST_BINARY_OP_BIT_XOR;
        return true;
    case AST_ASSIGN_OP_SHIFT_LEFT:
        *binary_operator = AST_BINARY_OP_SHIFT_LEFT;
        return true;
    case AST_ASSIGN_OP_SHIFT_RIGHT:
        *binary_operator = AST_BINARY_OP_SHIFT_RIGHT;
        return true;
    case AST_ASSIGN_OP_ASSIGN:
        return false;
    }

    return false;
}

bool mr_lower_assignment_target(MirUnitBuildContext *context,
                                const HirExpression *expression,
                                MirLValue *lvalue) {
    if (!context || !expression || !lvalue) {
        return false;
    }
    memset(lvalue, 0, sizeof(*lvalue));
    lvalue->type = expression->type;
    switch (expression->kind) {
    case HIR_EXPR_MEMORY_OP:
        if (expression->as.memory_op.kind != HIR_MEMORY_DEREF ||
            expression->as.memory_op.argument_count != 1) {
            mr_set_error(context->build,
                         expression->source_span,
                         NULL,
                         "Internal error: unsupported MIR assignment target memory op.");
            return false;
        }

        lvalue->kind = MIR_LVALUE_MEMORY_DEREF;
        lvalue->as.memory_deref.element_size = expression->as.memory_op.element_size;
        lvalue->as.memory_deref.is_checked_ptr = expression->as.memory_op.is_checked_ptr;
    lvalue->as.memory_deref.is_mmio = expression->as.memory_op.is_mmio;
        return mr_lower_expression(context,
                                   expression->as.memory_op.arguments[0],
                                   &lvalue->as.memory_deref.target);

    case HIR_EXPR_SYMBOL:
        if (expression->as.symbol.kind == SYMBOL_KIND_PARAMETER ||
            expression->as.symbol.kind == SYMBOL_KIND_LOCAL) {
            size_t local_index = mr_find_local_index(context->unit,
                                                      expression->as.symbol.symbol);

            if (local_index == (size_t)-1) {
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Internal error: missing MIR assignment local for symbol '%s'.",
                              expression->as.symbol.name);
                return false;
            }

            lvalue->kind = MIR_LVALUE_LOCAL;
            lvalue->as.local_index = local_index;
            lvalue->is_cell = context->unit->locals[local_index].is_cell;
            return true;
        }

        lvalue->kind = MIR_LVALUE_GLOBAL;
        lvalue->as.global_name = ast_copy_text(
            mr_hir_symbol_global_name(&expression->as.symbol));
        if (!lvalue->as.global_name) {
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR assignment targets.");
            return false;
        }
        return true;

    case HIR_EXPR_INDEX:
        lvalue->kind = MIR_LVALUE_INDEX;
        if (!mr_lower_expression(context,
                              expression->as.index.target,
                              &lvalue->as.index.target) ||
            !mr_lower_expression(context,
                              expression->as.index.index,
                              &lvalue->as.index.index)) {
            mr_lvalue_free(lvalue);
            return false;
        }
        return true;

    case HIR_EXPR_MEMBER:
        lvalue->kind = MIR_LVALUE_MEMBER;
        if (!mr_lower_expression(context,
                              expression->as.member.target,
                              &lvalue->as.member.target)) {
            mr_lvalue_free(lvalue);
            return false;
        }
        lvalue->as.member.member = ast_copy_text(expression->as.member.member);
        if (!lvalue->as.member.member) {
            mr_lvalue_free(lvalue);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR member targets.");
            return false;
        }
        return true;

    default:
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: unsupported MIR assignment target kind %d.",
                      expression->kind);
        return false;
    }
}

bool mr_load_lvalue_value(MirUnitBuildContext *context,
                          const MirLValue *lvalue,
                          AstSourceSpan source_span,
                          MirValue *value) {
    MirInstruction instruction;
    if (!context || !lvalue || !value) {
        return false;
    }

    *value = mr_invalid_value();
    switch (lvalue->kind) {
    case MIR_LVALUE_LOCAL:
        if (lvalue->is_cell) {
            return mr_emit_cell_read(context, lvalue->as.local_index,
                                      lvalue->type, source_span, value);
        }
        value->kind = MIR_VALUE_LOCAL;
        value->type = lvalue->type;
        value->as.local_index = lvalue->as.local_index;
        return true;

    case MIR_LVALUE_GLOBAL:
        return mr_value_from_global(context->build,
                                     lvalue->as.global_name,
                                     lvalue->type,
                                     value);
    case MIR_LVALUE_INDEX:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_INDEX_LOAD;
        instruction.as.index_load.dest_temp = context->unit->next_temp_index++;
        if (!mr_value_clone(context->build,
                             &lvalue->as.index.target,
                             &instruction.as.index_load.target) ||
            !mr_value_clone(context->build,
                             &lvalue->as.index.index,
                             &instruction.as.index_load.index)) {
            mr_instruction_free(&instruction);
            return false;
        }
        if (!mr_current_block(context) ||
            !mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          source_span,
                          NULL,
                          "Out of memory while lowering MIR index loads.");
            return false;
        }
        value->kind = MIR_VALUE_TEMP;
        value->type = lvalue->type;
        value->as.temp_index = instruction.as.index_load.dest_temp;
        return true;

    case MIR_LVALUE_MEMBER:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_MEMBER;
        instruction.as.member.dest_temp = context->unit->next_temp_index++;
        if (!mr_value_clone(context->build,
                             &lvalue->as.member.target,
                             &instruction.as.member.target)) {
            mr_instruction_free(&instruction);
            return false;
        }
        instruction.as.member.member = ast_copy_text(lvalue->as.member.member);
        if (!instruction.as.member.member) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          source_span,
                          NULL,
                          "Out of memory while lowering MIR member loads.");
            return false;
        }
        if (!mr_current_block(context) ||
            !mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          source_span,
                          NULL,
                          "Out of memory while lowering MIR member loads.");
            return false;
        }
        value->kind = MIR_VALUE_TEMP;
        value->type = lvalue->type;
        value->as.temp_index = instruction.as.member.dest_temp;
        return true;

    case MIR_LVALUE_MEMORY_DEREF:
        return mr_load_memory_deref_lvalue(context,
                                           lvalue,
                                           source_span,
                                           value);
    }

    return false;
}

bool mr_store_lvalue_value(MirUnitBuildContext *context,
                           const MirLValue *lvalue,
                           MirValue value,
                           AstSourceSpan source_span) {
    if (!context || !lvalue) {
        mr_value_free(&value);
        return false;
    }

#include "mir_lvalue_p2.inc"
