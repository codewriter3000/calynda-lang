#include "mir_internal.h"

/* mir_expr_call.c */
bool mr_lower_call_expression(MirUnitBuildContext *context,
                              const HirExpression *expression,
                              MirValue *value);
bool mr_lower_memory_op_expression(MirUnitBuildContext *context,
                                   const HirExpression *expression,
                                   MirValue *value);

bool mr_lower_expression(MirUnitBuildContext *context,
                         const HirExpression *expression,
                         MirValue *value) {
    MirInstruction instruction;
    MirValue left;
    MirValue right;

    if (!context || !expression || !value) {
        return false;
    }

    *value = mr_invalid_value();
    if (!mr_current_block(context)) {
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering expression.");
        return false;
    }

    switch (expression->kind) {
    case HIR_EXPR_LITERAL:
        return mr_value_from_literal(context->build,
                                      &expression->as.literal,
                                      expression->type,
                                      value);

    case HIR_EXPR_TEMPLATE:
        return mr_lower_template_expression(context, expression, value);

    case HIR_EXPR_SYMBOL:
        if (expression->as.symbol.kind == SYMBOL_KIND_PARAMETER ||
            expression->as.symbol.kind == SYMBOL_KIND_LOCAL) {
            size_t local_index = mr_find_local_index(context->unit,
                                                      expression->as.symbol.symbol);

            if (local_index == (size_t)-1) {
                mr_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Internal error: missing MIR local for symbol '%s'.",
                              expression->as.symbol.name);
                return false;
            }

            value->kind = MIR_VALUE_LOCAL;
            value->type = expression->type;
            value->as.local_index = local_index;
            return true;
        }

        return mr_value_from_global(context->build,
                                     expression->as.symbol.name,
                                     expression->type,
                                     value);

    case HIR_EXPR_BINARY:
        if (expression->as.binary.operator == AST_BINARY_OP_LOGICAL_AND ||
            expression->as.binary.operator == AST_BINARY_OP_LOGICAL_OR) {
            return mr_lower_short_circuit_binary(context, expression, value);
        }

        if (!mr_lower_expression(context, expression->as.binary.left, &left) ||
            !mr_lower_expression(context, expression->as.binary.right, &right)) {
            return false;
        }

        if (!mr_current_block(context)) {
            mr_value_free(&left);
            mr_value_free(&right);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Internal error: missing current MIR block after lowering binary operands.");
            return false;
        }

        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_BINARY;
        instruction.as.binary.dest_temp = context->unit->next_temp_index++;
        instruction.as.binary.operator = expression->as.binary.operator;
        instruction.as.binary.left = left;
        instruction.as.binary.right = right;
        if (!mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR instructions.");
            return false;
        }

        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.binary.dest_temp;
        return true;

    case HIR_EXPR_UNARY:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_UNARY;
        instruction.as.unary.dest_temp = context->unit->next_temp_index++;
        instruction.as.unary.operator = expression->as.unary.operator;
        if (!mr_lower_expression(context, expression->as.unary.operand, &instruction.as.unary.operand)) {
            return false;
        }
        if (!mr_current_block(context) ||
            !mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR unary expressions.");
            return false;
        }
        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.unary.dest_temp;
        return true;

    case HIR_EXPR_CALL:
        return mr_lower_call_expression(context, expression, value);

    case HIR_EXPR_CAST:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_CAST;
        instruction.as.cast.dest_temp = context->unit->next_temp_index++;
        instruction.as.cast.target_type = expression->as.cast.target_type;
        if (!mr_lower_expression(context, expression->as.cast.expression, &instruction.as.cast.operand)) {
            return false;
        }
        if (!mr_current_block(context)) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Internal error: missing current MIR block after lowering cast operand.");
            return false;
        }
        if (!mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR casts.");
            return false;
        }
        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.cast.dest_temp;
        return true;

    case HIR_EXPR_LAMBDA:
        return mr_lower_lambda_expression(context, expression, value);
    case HIR_EXPR_ASSIGNMENT:
        return mr_lower_assignment_expression(context, expression, value);
    case HIR_EXPR_TERNARY:
        return mr_lower_ternary_expression(context, expression, value);
    case HIR_EXPR_INDEX:
        return mr_lower_index_expression(context, expression, value);
    case HIR_EXPR_MEMBER:
        return mr_lower_member_expression(context, expression, value);
    case HIR_EXPR_ARRAY_LITERAL:
        return mr_lower_array_literal(context, expression, value);
    case HIR_EXPR_DISCARD:
        value->kind = MIR_VALUE_LITERAL;
        value->type = expression->type;
        value->as.literal.kind = AST_LITERAL_NULL;
        value->as.literal.text = NULL;
        value->as.literal.bool_value = false;
        return true;
    case HIR_EXPR_POST_INCREMENT:
        return mr_lower_postfix_increment(context, expression, value, true);
    case HIR_EXPR_POST_DECREMENT:
        return mr_lower_postfix_increment(context, expression, value, false);
    case HIR_EXPR_MEMORY_OP:
        return mr_lower_memory_op_expression(context, expression, value);
    }

    return false;
}
