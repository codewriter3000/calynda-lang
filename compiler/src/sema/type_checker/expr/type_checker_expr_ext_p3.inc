        *info = tc_type_check_info_make(tc_checked_type_named("Atomic", 1, 0));
        tc_type_check_info_set_first_generic_arg(info,
                                                 tc_type_check_source_type(argument_info));
        return true;
    }

    {
        const TypeCheckInfo *target_info = tc_check_expression(checker, target);
        CheckedType target_generic_arg;
        bool has_target_generic_arg;

        if (!target_info) {
            return true;
        }

        has_target_generic_arg = tc_type_check_info_first_generic_arg(checker,
                                                                      target_info,
                                                                      NULL,
                                                                      &target_generic_arg);
        if (checker->has_error) {
            return true;
        }

        if (!tc_checked_type_is_named_type(target_info->type, "Atomic")) {
            return false;
        }

        if (strcmp(member_name, "load") == 0) {
            if (expression->as.call.arguments.count != 0) {
                tc_set_error_at(checker,
                                expression->source_span,
                                NULL,
                                "Atomic.load expects 0 arguments but got %zu.",
                                expression->as.call.arguments.count);
                return true;
            }

            *info = tc_type_check_info_make(has_target_generic_arg
                                                ? target_generic_arg
                                                : tc_checked_type_external());
            return true;
        }

        if (strcmp(member_name, "store") == 0 ||
            strcmp(member_name, "exchange") == 0) {
            const TypeCheckInfo *argument_info;

            if (expression->as.call.arguments.count != 1) {
                tc_set_error_at(checker,
                                expression->source_span,
                                NULL,
                                "Atomic.%s expects exactly 1 argument but got %zu.",
                                member_name,
                                expression->as.call.arguments.count);
                return true;
            }

            argument_info = tc_check_expression(checker,
                                                expression->as.call.arguments.items[0]);
            if (!argument_info) {
                return true;
            }

            if (has_target_generic_arg &&
                !tc_checked_type_assignable(target_generic_arg,
                                            tc_type_check_source_type(argument_info))) {
                char expected_text[64];
                char actual_text[64];

                checked_type_to_string(target_generic_arg,
                                       expected_text,
                                       sizeof(expected_text));
                checked_type_to_string(tc_type_check_source_type(argument_info),
                                       actual_text,
                                       sizeof(actual_text));
                tc_set_error_at(checker,
                                expression->as.call.arguments.items[0]->source_span,
                                &target->source_span,
                                "Atomic.%s expects %s but got %s.",
                                member_name,
                                expected_text,
                                actual_text);
                return true;
            }

            *info = tc_type_check_info_make(
                strcmp(member_name, "store") == 0
                    ? tc_checked_type_void()
                    : (has_target_generic_arg
                           ? target_generic_arg
                           : tc_checked_type_external()));
            return true;
        }
    }

    return false;
}

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

#include "type_checker_expr_ext_p4.inc"
