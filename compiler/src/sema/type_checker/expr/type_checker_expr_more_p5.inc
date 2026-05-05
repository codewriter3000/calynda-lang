    case AST_EXPR_POST_INCREMENT:
    case AST_EXPR_POST_DECREMENT:
        {
            const AstExpression *operand = (expression->kind == AST_EXPR_POST_INCREMENT)
                ? expression->as.post_increment.operand
                : expression->as.post_decrement.operand;
            const TypeCheckInfo *operand_info = tc_check_expression(checker, operand);
            CheckedType operand_type;
            if (!operand_info) {
                return NULL;
            }
            operand_type = tc_type_check_source_type(operand_info);
            if (!tc_checked_type_is_numeric(operand_type)) {
                char operand_text[64];
                checked_type_to_string(operand_type, operand_text, sizeof(operand_text));
                tc_set_error_at(checker,
                                expression->source_span,
                                NULL,
                                "Postfix operator requires a numeric operand but got %s.",
                                operand_text);
                return NULL;
            }
            info = *operand_info;
        }
        break;

    case AST_EXPR_MEMORY_OP:
        return tc_check_memory_operation_expression(checker, expression);

    case AST_EXPR_SPAWN:
        {
            const TypeCheckInfo *callable_info = tc_check_expression(checker,
                                                                     expression->as.spawn.callable);
            TcVisitedSymbolSet visited;
            if (!callable_info) {
                return NULL;
            }
            if (!callable_info->is_callable ||
                (callable_info->parameters && callable_info->parameters->count != 0)) {
                tc_set_error_at(checker,
                                expression->as.spawn.callable->source_span,
                                NULL,
                                "spawn requires a zero-argument callable.");
                return NULL;
            }
            memset(&visited, 0, sizeof(visited));
            if (!tc_analyze_spawn_callable_races(checker,
                                                 expression->as.spawn.callable,
                                                 &visited)) {
                tc_visited_free(&visited);
                return NULL;
            }
            tc_visited_free(&visited);
            if (callable_info->callable_return_type.kind == CHECKED_TYPE_VOID) {
                info = tc_type_check_info_make(tc_checked_type_named("Thread", 0, 0));
            } else {
                info = tc_type_check_info_make(tc_checked_type_named("Future", 1, 0));
                tc_type_check_info_set_first_generic_arg(&info,
                                                         callable_info->callable_return_type);
            }
        }
        break;

    default:
        break;

    case AST_EXPR_NONLOCAL_RETURN:
        {
            /* `return expr` used as a |var argument.
               Validate the inner value (if any) against the outer return type.
               The expression itself has EXTERNAL type (callable semantics at the call site). */
            if (expression->as.nonlocal_return_value) {
                const TypeCheckInfo *val_info =
                    tc_check_expression(checker, expression->as.nonlocal_return_value);
                if (!val_info) {
                    return NULL;
                }
                if (checker->has_outer_return_type) {
                    CheckedType val_type = tc_type_check_source_type(val_info);
                    if (!tc_checked_type_assignable(checker->outer_return_type, val_type)) {
                        char expected_text[64];
                        char actual_text[64];

                        checked_type_to_string(checker->outer_return_type,
                                               expected_text, sizeof(expected_text));
                        checked_type_to_string(val_type, actual_text, sizeof(actual_text));
                        tc_set_error_at(checker,
                                        expression->as.nonlocal_return_value->source_span,
                                        NULL,
                                        "Non-local return value must produce %s but got %s.",
                                        expected_text,
                                        actual_text);
                        return NULL;
                    }
                }
            }
            /* The |var block itself is a zero-arg callable of external type. */
            info = tc_type_check_info_make(tc_checked_type_external());
        }
        break;
    }

    return tc_store_expression_info(checker, expression, info);
}
