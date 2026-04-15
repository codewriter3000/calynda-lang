#include "type_checker_internal.h"

const TypeCheckInfo *tc_check_expression_ext(TypeChecker *checker,
                                             const AstExpression *expression) {
    TypeCheckInfo info = tc_type_check_info_make(tc_checked_type_invalid());

    switch (expression->kind) {
    case AST_EXPR_TERNARY:
        {
            const TypeCheckInfo *condition_info = tc_check_expression(checker,
                                                                      expression->as.ternary.condition);
            const TypeCheckInfo *then_info;
            const TypeCheckInfo *else_info;
            CheckedType merged_type;
            char then_text[64];
            char else_text[64];

            if (!condition_info) {
                return NULL;
            }

            if (!tc_checked_type_is_bool(tc_type_check_source_type(condition_info))) {
                char condition_text[64];

                checked_type_to_string(tc_type_check_source_type(condition_info),
                                       condition_text,
                                       sizeof(condition_text));
                tc_set_error_at(checker,
                                expression->as.ternary.condition->source_span,
                                NULL,
                                "Ternary condition must have type bool but got %s.",
                                condition_text);
                return NULL;
            }

            then_info = tc_check_expression(checker, expression->as.ternary.then_branch);
            else_info = tc_check_expression(checker, expression->as.ternary.else_branch);
            if (!then_info || !else_info) {
                return NULL;
            }

            if (!tc_merge_types_for_inference(tc_type_check_source_type(then_info),
                                              tc_type_check_source_type(else_info),
                                              &merged_type)) {
                checked_type_to_string(tc_type_check_source_type(then_info),
                                       then_text,
                                       sizeof(then_text));
                checked_type_to_string(tc_type_check_source_type(else_info),
                                       else_text,
                                       sizeof(else_text));
                tc_set_error_at(checker,
                                expression->as.ternary.else_branch->source_span,
                                &expression->as.ternary.then_branch->source_span,
                                "Ternary branches must have compatible types, but got %s and %s.",
                                then_text,
                                else_text);
                return NULL;
            }

            info = tc_type_check_info_make(merged_type);
        }
        break;

    case AST_EXPR_BINARY:
        {
            const TypeCheckInfo *left_info = tc_check_expression(checker, expression->as.binary.left);
            const TypeCheckInfo *right_info = tc_check_expression(checker, expression->as.binary.right);
            CheckedType result_type;

            if (!left_info || !right_info) {
                return NULL;
            }

            if (!tc_check_binary_operator(checker,
                                          expression,
                                          expression->as.binary.operator,
                                          tc_type_check_source_type(left_info),
                                          tc_type_check_source_type(right_info),
                                          &result_type)) {
                return NULL;
            }

            info = tc_type_check_info_make(result_type);
        }
        break;

    case AST_EXPR_UNARY:
        {
            const TypeCheckInfo *operand_info = tc_check_expression(checker,
                                                                     expression->as.unary.operand);
            CheckedType operand_type;
            char operand_text[64];

            if (!operand_info) {
                return NULL;
            }

            operand_type = tc_type_check_source_type(operand_info);
            switch (expression->as.unary.operator) {
            case AST_UNARY_OP_LOGICAL_NOT:
                if (!tc_checked_type_is_bool(operand_type)) {
                    checked_type_to_string(operand_type, operand_text, sizeof(operand_text));
                    tc_set_error_at(checker,
                                    expression->source_span,
                                    NULL,
                                    "Operator '!' requires bool but got %s.",
                                    operand_text);
                    return NULL;
                }
                info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_BOOL, 0));
                break;

            case AST_UNARY_OP_BITWISE_NOT:
                if (!tc_checked_type_is_integral(operand_type)) {
                    checked_type_to_string(operand_type, operand_text, sizeof(operand_text));
                    tc_set_error_at(checker,
                                    expression->source_span,
                                    NULL,
                                    "Operator '~' requires an integral operand but got %s.",
                                    operand_text);
                    return NULL;
                }
                info = tc_type_check_info_make(operand_type);
                break;

            case AST_UNARY_OP_NEGATE:
            case AST_UNARY_OP_PLUS:
            case AST_UNARY_OP_PRE_INCREMENT:
            case AST_UNARY_OP_PRE_DECREMENT:
            case AST_UNARY_OP_DEREF:
            case AST_UNARY_OP_ADDRESS_OF:
                if (!tc_checked_type_is_numeric(operand_type)) {
                    checked_type_to_string(operand_type, operand_text, sizeof(operand_text));
                    tc_set_error_at(checker,
                                    expression->source_span,
                                    NULL,
                                    "Unary operator requires a numeric operand but got %s.",
                                    operand_text);
                    return NULL;
                }
                info = tc_type_check_info_make(operand_type);
                break;
            }
        }
        break;

    case AST_EXPR_CALL:
        {
            const TypeCheckInfo *callee_info = tc_check_expression(checker,
                                                                    expression->as.call.callee);
            size_t i;

            if (!callee_info) {
                return NULL;
            }

            if (!callee_info->is_callable) {
                char callee_text[64];

                checked_type_to_string(callee_info->type, callee_text, sizeof(callee_text));
                tc_set_error_at(checker,
                                expression->as.call.callee->source_span,
                                NULL,
                                "Expression of type %s is not callable.",
                                callee_text);
                return NULL;
            }

            if (callee_info->parameters) {
                bool has_varargs = callee_info->parameters->count > 0 &&
                    callee_info->parameters->items[callee_info->parameters->count - 1].is_varargs;
                size_t required_count = has_varargs
                    ? callee_info->parameters->count - 1
                    : callee_info->parameters->count;

                if (has_varargs
                        ? expression->as.call.arguments.count < required_count
                        : expression->as.call.arguments.count != required_count) {
                    tc_set_error_at(checker,
                                    expression->source_span,
                                    NULL,
                                    has_varargs
                                        ? "Call expects at least %zu argument%s but got %zu."
                                        : "Call expects %zu argument%s but got %zu.",
                                    required_count,
                                    required_count == 1 ? "" : "s",
                                    expression->as.call.arguments.count);
                    return NULL;
                }
            }

            for (i = 0; i < expression->as.call.arguments.count; i++) {
                const TypeCheckInfo *argument_info = tc_check_expression(checker,
                                                                         expression->as.call.arguments.items[i]);

                if (!argument_info) {
                    return NULL;
                }

                if (callee_info->parameters) {
                    size_t param_index = (i < callee_info->parameters->count)
                        ? i
                        : callee_info->parameters->count - 1;
                    CheckedType parameter_type = tc_checked_type_from_ast_type(checker,
                        &callee_info->parameters->items[param_index].type);
                    CheckedType argument_type = tc_type_check_source_type(argument_info);

                    if (checker->has_error) {
                        return NULL;
                    }

                    if (!tc_checked_type_assignable(parameter_type, argument_type)) {
                        char expected_text[64];
                        char actual_text[64];

                        checked_type_to_string(parameter_type,
                                               expected_text,
                                               sizeof(expected_text));
                        checked_type_to_string(argument_type,
                                               actual_text,
                                               sizeof(actual_text));
                        tc_set_error_at(checker,
                                        expression->as.call.arguments.items[i]->source_span,
                                        &callee_info->parameters->items[param_index].name_span,
                                        "Argument %zu to call expects %s but got %s.",
                                        i + 1,
                                        expected_text,
                                        actual_text);
                        return NULL;
                    }
                } else if (tc_type_check_source_type(argument_info).kind == CHECKED_TYPE_VOID) {
                    tc_set_error_at(checker,
                                    expression->as.call.arguments.items[i]->source_span,
                                    NULL,
                                    "Call arguments cannot have type void.");
                    return NULL;
                }
            }

            info = tc_type_check_info_make(callee_info->callable_return_type);
        }
        break;

    default:
        return tc_check_expression_more(checker, expression);
    }

    return tc_store_expression_info(checker, expression, info);
}
