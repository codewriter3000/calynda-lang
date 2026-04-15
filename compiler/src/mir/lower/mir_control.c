#include "mir_internal.h"

bool mr_lower_ternary_expression(MirUnitBuildContext *context,
                                 const HirExpression *expression,
                                 MirValue *value) {
    MirBasicBlock *origin_block;
    MirValue condition_value;
    MirValue branch_value;
    size_t result_local_index;
    size_t then_block_index;
    size_t else_block_index;
    size_t merge_block_index;

    if (!mr_append_synthetic_local(context,
                                "ternary",
                                expression->type,
                                expression->source_span,
                                &result_local_index)) {
        return false;
    }

    if (!mr_lower_expression(context, expression->as.ternary.condition, &condition_value)) {
        return false;
    }

    if (!mr_create_block(context, &then_block_index) ||
        !mr_create_block(context, &else_block_index) ||
        !mr_create_block(context, &merge_block_index)) {
        mr_value_free(&condition_value);
        return false;
    }

    origin_block = mr_current_block(context);
    if (!origin_block) {
        mr_value_free(&condition_value);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: missing current MIR block for ternary branch.");
        return false;
    }

    origin_block->terminator.kind = MIR_TERM_BRANCH;
    origin_block->terminator.as.branch_term.condition = condition_value;
    origin_block->terminator.as.branch_term.true_block = then_block_index;
    origin_block->terminator.as.branch_term.false_block = else_block_index;

    context->current_block_index = then_block_index;
    if (!mr_lower_expression(context, expression->as.ternary.then_branch, &branch_value)) {
        return false;
    }
    if (branch_value.kind == MIR_VALUE_INVALID) {
        mr_set_error(context->build,
                      expression->as.ternary.then_branch->source_span,
                      NULL,
                      "Internal error: ternary true branch did not produce a MIR value.");
        return false;
    }
    if (!mr_append_store_local_instruction(context,
                                        result_local_index,
                                        branch_value,
                                        expression->as.ternary.then_branch->source_span)) {
        return false;
    }
    mr_set_goto_terminator(context, merge_block_index);

    context->current_block_index = else_block_index;
    if (!mr_lower_expression(context, expression->as.ternary.else_branch, &branch_value)) {
        return false;
    }
    if (branch_value.kind == MIR_VALUE_INVALID) {
        mr_set_error(context->build,
                      expression->as.ternary.else_branch->source_span,
                      NULL,
                      "Internal error: ternary false branch did not produce a MIR value.");
        return false;
    }
    if (!mr_append_store_local_instruction(context,
                                        result_local_index,
                                        branch_value,
                                        expression->as.ternary.else_branch->source_span)) {
        return false;
    }
    mr_set_goto_terminator(context, merge_block_index);

    context->current_block_index = merge_block_index;
    value->kind = MIR_VALUE_LOCAL;
    value->type = expression->type;
    value->as.local_index = result_local_index;
    return true;
}

bool mr_lower_short_circuit_binary(MirUnitBuildContext *context,
                                   const HirExpression *expression,
                                   MirValue *value) {
    AstBinaryOperator operator;
    MirBasicBlock *origin_block;
    MirValue left_value;
    MirValue right_value;
    MirValue short_circuit_value;
    size_t result_local_index;
    size_t true_block_index;
    size_t false_block_index;
    size_t rhs_block_index;
    size_t short_circuit_block_index;
    size_t merge_block_index;
    bool short_circuit_bool_value;

    operator = expression->as.binary.operator;
    short_circuit_bool_value = (operator == AST_BINARY_OP_LOGICAL_OR);

    if (!mr_append_synthetic_local(context,
                                "logical",
                                expression->type,
                                expression->source_span,
                                &result_local_index)) {
        return false;
    }

    if (!mr_lower_expression(context, expression->as.binary.left, &left_value)) {
        return false;
    }

    if (!mr_create_block(context, &true_block_index) ||
        !mr_create_block(context, &false_block_index) ||
        !mr_create_block(context, &merge_block_index)) {
        mr_value_free(&left_value);
        return false;
    }

    rhs_block_index = (operator == AST_BINARY_OP_LOGICAL_AND)
                          ? true_block_index
                          : false_block_index;
    short_circuit_block_index = (operator == AST_BINARY_OP_LOGICAL_AND)
                                    ? false_block_index
                                    : true_block_index;

    origin_block = mr_current_block(context);
    if (!origin_block) {
        mr_value_free(&left_value);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: missing current MIR block for short-circuit branch.");
        return false;
    }

    origin_block->terminator.kind = MIR_TERM_BRANCH;
    origin_block->terminator.as.branch_term.condition = left_value;
    origin_block->terminator.as.branch_term.true_block = true_block_index;
    origin_block->terminator.as.branch_term.false_block = false_block_index;

    context->current_block_index = short_circuit_block_index;
    short_circuit_value = mr_invalid_value();
    short_circuit_value.kind = MIR_VALUE_LITERAL;
    short_circuit_value.type = expression->type;
    short_circuit_value.as.literal.kind = AST_LITERAL_BOOL;
    short_circuit_value.as.literal.bool_value = short_circuit_bool_value;
    if (!mr_append_store_local_instruction(context,
                                        result_local_index,
                                        short_circuit_value,
                                        expression->source_span)) {
        return false;
    }
    mr_set_goto_terminator(context, merge_block_index);

    context->current_block_index = rhs_block_index;
    if (!mr_lower_expression(context, expression->as.binary.right, &right_value)) {
        return false;
    }
    if (right_value.kind == MIR_VALUE_INVALID) {
        mr_set_error(context->build,
                      expression->as.binary.right->source_span,
                      NULL,
                      "Internal error: logical right operand did not produce a MIR value.");
        return false;
    }
    if (!mr_append_store_local_instruction(context,
                                        result_local_index,
                                        right_value,
                                        expression->as.binary.right->source_span)) {
        return false;
    }
    mr_set_goto_terminator(context, merge_block_index);

    context->current_block_index = merge_block_index;
    value->kind = MIR_VALUE_LOCAL;
    value->type = expression->type;
    value->as.local_index = result_local_index;
    return true;
}
