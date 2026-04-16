#include "type_checker_internal.h"

static bool tc_memory_op_allows_ptr_argument(const AstExpression *expression,
                                             size_t argument_index,
                                             CheckedType argument_type) {
    if (argument_index != 0 || argument_type.kind != CHECKED_TYPE_NAMED ||
        argument_type.name == NULL || strcmp(argument_type.name, "ptr") != 0) {
        return false;
    }

    return expression->as.memory_op.kind == AST_MEMORY_DEREF ||
        expression->as.memory_op.kind == AST_MEMORY_ADDR ||
        expression->as.memory_op.kind == AST_MEMORY_OFFSET ||
        expression->as.memory_op.kind == AST_MEMORY_STORE ||
        expression->as.memory_op.kind == AST_MEMORY_FREE ||
        expression->as.memory_op.kind == AST_MEMORY_CLEANUP;
}

static bool tc_validate_cleanup_callable(TypeChecker *checker,
                                         const AstExpression *argument_expression,
                                         const TypeCheckInfo *callable_info,
                                         CheckedType cleanup_value_type) {
    CheckedType parameter_type;
    char expected_text[64];
    char actual_text[64];

    if (!callable_info->is_callable) {
        tc_set_error_at(checker,
                        argument_expression->source_span,
                        NULL,
                        "cleanup() expects a callable second argument.");
        return false;
    }

    if (!callable_info->parameters) {
        return true;
    }

    if (callable_info->parameters->count != 1 ||
        callable_info->parameters->items[0].is_varargs) {
        tc_set_error_at(checker,
                        argument_expression->source_span,
                        NULL,
                        "cleanup() expects a callable that takes exactly one argument.");
        return false;
    }

    parameter_type = tc_checked_type_from_ast_type(checker,
        &callable_info->parameters->items[0].type);
    if (checker->has_error) {
        return false;
    }

    if (tc_checked_type_assignable(parameter_type, cleanup_value_type)) {
        return true;
    }

    checked_type_to_string(parameter_type, expected_text, sizeof(expected_text));
    checked_type_to_string(cleanup_value_type, actual_text, sizeof(actual_text));
    tc_set_error_at(checker,
                    argument_expression->source_span,
                    &callable_info->parameters->items[0].name_span,
                    "cleanup() expects a callable accepting %s but got %s.",
                    expected_text,
                    actual_text);
    return false;
}

const TypeCheckInfo *tc_check_memory_operation_expression(TypeChecker *checker,
                                                          const AstExpression *expression) {
    CheckedType first_argument_type = tc_checked_type_invalid();
    TypeCheckInfo info;
    size_t argument_index;

    for (argument_index = 0;
         argument_index < expression->as.memory_op.arguments.count;
         argument_index++) {
        const AstExpression *argument_expression =
            expression->as.memory_op.arguments.items[argument_index];
        const TypeCheckInfo *argument_info = tc_check_expression(checker, argument_expression);
        CheckedType argument_type;

        if (!argument_info) {
            return NULL;
        }

        if (expression->as.memory_op.kind == AST_MEMORY_CLEANUP && argument_index == 1) {
            if (!tc_validate_cleanup_callable(checker,
                                              argument_expression,
                                              argument_info,
                                              first_argument_type)) {
                return NULL;
            }
            continue;
        }

        argument_type = tc_type_check_source_type(argument_info);
        if (!tc_checked_type_is_integral(argument_type) &&
            !tc_memory_op_allows_ptr_argument(expression, argument_index, argument_type)) {
            char argument_text[64];

            checked_type_to_string(argument_type, argument_text, sizeof(argument_text));
            tc_set_error_at(checker,
                            argument_expression->source_span,
                            NULL,
                            "Memory operation argument must have an integral type but got %s.",
                            argument_text);
            return NULL;
        }

        if (argument_index == 0) {
            first_argument_type = argument_type;
        }
    }

    if (expression->as.memory_op.kind == AST_MEMORY_FREE ||
        expression->as.memory_op.kind == AST_MEMORY_STORE) {
        info = tc_type_check_info_make(tc_checked_type_void());
    } else if (expression->as.memory_op.kind == AST_MEMORY_CLEANUP) {
        info = tc_type_check_info_make(first_argument_type);
    } else {
        info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_INT64, 0));
    }

    return tc_store_expression_info(checker, expression, info);
}