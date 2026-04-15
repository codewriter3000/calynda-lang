#include "mir_internal.h"

bool mr_lower_assignment_expression(MirUnitBuildContext *context,
                                    const HirExpression *expression,
                                    MirValue *value) {
    MirInstruction instruction;
    MirLValue lvalue;
    MirValue current_value;
    MirValue rhs_value;
    MirValue stored_value;
    AstBinaryOperator binary_operator;

    memset(&lvalue, 0, sizeof(lvalue));
    current_value = mr_invalid_value();
    rhs_value = mr_invalid_value();
    stored_value = mr_invalid_value();
    *value = mr_invalid_value();

    if (!mr_lower_assignment_target(context, expression->as.assignment.target, &lvalue)) {
        return false;
    }

    if (expression->as.assignment.operator == AST_ASSIGN_OP_ASSIGN) {
        if (!mr_lower_expression(context, expression->as.assignment.value, value)) {
            mr_lvalue_free(&lvalue);
            return false;
        }
    } else {
        if (!mr_map_compound_assignment(expression->as.assignment.operator, &binary_operator) ||
            !mr_load_lvalue_value(context,
                               &lvalue,
                               expression->as.assignment.target->source_span,
                               &current_value) ||
            !mr_lower_expression(context, expression->as.assignment.value, &rhs_value)) {
            mr_lvalue_free(&lvalue);
            mr_value_free(&current_value);
            mr_value_free(&rhs_value);
            return false;
        }
        if (!mr_current_block(context)) {
            mr_lvalue_free(&lvalue);
            mr_value_free(&current_value);
            mr_value_free(&rhs_value);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Internal error: missing current MIR block after lowering assignment operands.");
            return false;
        }

        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_BINARY;
        instruction.as.binary.dest_temp = context->unit->next_temp_index++;
        instruction.as.binary.operator = binary_operator;
        instruction.as.binary.left = current_value;
        instruction.as.binary.right = rhs_value;
        if (!mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_lvalue_free(&lvalue);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR assignment expressions.");
            return false;
        }

        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.binary.dest_temp;
    }

    if (!mr_value_clone(context->build, value, &stored_value) ||
        !mr_store_lvalue_value(context,
                            &lvalue,
                            stored_value,
                            expression->source_span)) {
        mr_lvalue_free(&lvalue);
        mr_value_free(value);
        return false;
    }

    mr_lvalue_free(&lvalue);
    return true;
}

bool mr_lower_postfix_increment(MirUnitBuildContext *context,
                                const HirExpression *expression,
                                MirValue *value,
                                bool is_increment) {
    MirLValue lvalue;
    MirValue current_value;
    MirValue stored_value;
    MirValue one_literal;
    MirInstruction instruction;

    memset(&lvalue, 0, sizeof(lvalue));
    current_value = mr_invalid_value();
    stored_value = mr_invalid_value();

    /* The operand is the HIR expression for the variable (e.g. `c`). */
    {
        const HirExpression *operand = is_increment
            ? expression->as.post_increment.operand
            : expression->as.post_decrement.operand;

        if (!mr_lower_assignment_target(context, operand, &lvalue)) {
            return false;
        }

        if (!mr_load_lvalue_value(context, &lvalue,
                               expression->source_span, &current_value)) {
            mr_lvalue_free(&lvalue);
            return false;
        }
    }

    /* Save the original value into a temporary (the result of the expression). */
    {
        MirValue saved;

        if (!mr_value_clone(context->build, &current_value, &saved)) {
            mr_lvalue_free(&lvalue);
            mr_value_free(&current_value);
            return false;
        }

        if (!mr_append_synthetic_local(context, "postfix_old",
                                    expression->type,
                                    expression->source_span,
                                    NULL)) {
            mr_lvalue_free(&lvalue);
            mr_value_free(&current_value);
            mr_value_free(&saved);
            return false;
        }

        /* We need the old value in a temp so the binary instruction doesn't
           lose it when the store overwrites the variable. */
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_BINARY;
        instruction.as.binary.dest_temp = context->unit->next_temp_index++;
        instruction.as.binary.operator = AST_BINARY_OP_ADD;
        instruction.as.binary.left = saved;

        /* literal 0 so that old_temp = current + 0 = current */
        one_literal.kind = MIR_VALUE_LITERAL;
        one_literal.type = expression->type;
        one_literal.as.literal.kind = AST_LITERAL_INTEGER;
        one_literal.as.literal.text = ast_copy_text("0");
        one_literal.as.literal.bool_value = false;
        if (!one_literal.as.literal.text) {
            mr_lvalue_free(&lvalue);
            mr_value_free(&current_value);
            mr_set_error(context->build, expression->source_span, NULL,
                          "Out of memory while lowering postfix expression.");
            return false;
        }
        instruction.as.binary.right = one_literal;
        if (!mr_current_block(context) ||
            !mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_lvalue_free(&lvalue);
            mr_value_free(&current_value);
            return false;
        }

        /* This temp holds the original value — it's the expression result. */
        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.binary.dest_temp;
    }

    /* Compute the incremented/decremented value: current +/- 1. */
    {
        MirValue new_value_literal;

        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_BINARY;
        instruction.as.binary.dest_temp = context->unit->next_temp_index++;
        instruction.as.binary.operator = is_increment
            ? AST_BINARY_OP_ADD
            : AST_BINARY_OP_SUBTRACT;
        instruction.as.binary.left = current_value;

        new_value_literal.kind = MIR_VALUE_LITERAL;
        new_value_literal.type = expression->type;
        new_value_literal.as.literal.kind = AST_LITERAL_INTEGER;
        new_value_literal.as.literal.text = ast_copy_text("1");
        new_value_literal.as.literal.bool_value = false;
        if (!new_value_literal.as.literal.text) {
            mr_lvalue_free(&lvalue);
            mr_set_error(context->build, expression->source_span, NULL,
                          "Out of memory while lowering postfix expression.");
            return false;
        }
        instruction.as.binary.right = new_value_literal;
        if (!mr_current_block(context) ||
            !mr_append_instruction(mr_current_block(context), instruction)) {
            mr_instruction_free(&instruction);
            mr_lvalue_free(&lvalue);
            return false;
        }

        stored_value.kind = MIR_VALUE_TEMP;
        stored_value.type = expression->type;
        stored_value.as.temp_index = instruction.as.binary.dest_temp;
    }

    /* Store the new value back to the variable. */
    if (!mr_store_lvalue_value(context, &lvalue, stored_value,
                            expression->source_span)) {
        mr_lvalue_free(&lvalue);
        return false;
    }

    mr_lvalue_free(&lvalue);
    return true;
}
