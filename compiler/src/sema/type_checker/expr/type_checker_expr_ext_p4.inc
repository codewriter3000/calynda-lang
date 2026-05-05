    case AST_EXPR_CALL:
        {
            if (tc_check_builtin_type_query_call(checker, expression, &info)) {
                if (checker->has_error) {
                    return NULL;
                }
                break;
            }

            if (tc_check_builtin_array_call(checker, expression, &info)) {
                if (checker->has_error) {
                    return NULL;
                }
                break;
            }

            if (tc_check_builtin_concurrency_call(checker, expression, &info)) {
                if (checker->has_error) {
                    return NULL;
                }
                break;
            }

            if (tc_try_resolve_overload_call(checker, expression)) {
                if (checker->has_error) {
                    return NULL;
                }
            }

            const TypeCheckInfo *callee_info = tc_check_expression(checker,
                                                                      expression->as.call.callee);
            size_t i;

            if (!callee_info) {
                return NULL;
            }

            if (!callee_info->is_callable) {
                /* Allow calling an untyped (var) parameter: its type is EXTERNAL and we
                   treat it as a dynamic callable that returns EXTERNAL. */
                if (callee_info->type.kind == CHECKED_TYPE_EXTERNAL) {
                    for (i = 0; i < expression->as.call.arguments.count; i++) {
                        if (!tc_check_expression(checker,
                                                 expression->as.call.arguments.items[i])) {
                            return NULL;
                        }
                    }
                    info = tc_type_check_info_make_external_value();
                    return tc_store_expression_info(checker, expression, info);
                }

                {
                    char callee_text[64];

                    checked_type_to_string(callee_info->type, callee_text, sizeof(callee_text));
                    tc_set_error_at(checker,
                                    expression->as.call.callee->source_span,
                                    NULL,
                                    "Expression of type %s is not callable.",
                                    callee_text);
                    return NULL;
                }
            }

            if (callee_info->parameters) {
                bool has_varargs = callee_info->parameters->count > 0 &&
                    callee_info->parameters->items[callee_info->parameters->count - 1].is_varargs;
                size_t required_count = 0;
                size_t maximum_count = has_varargs
                    ? (size_t)-1
                    : callee_info->parameters->count;

                for (i = 0; i < callee_info->parameters->count; i++) {
                    if (callee_info->parameters->items[i].is_varargs) {
                        break;
                    }
                    if (!callee_info->parameters->items[i].default_expr) {
                        required_count++;
                    }
                }

                if (expression->as.call.arguments.count < required_count ||
                    expression->as.call.arguments.count > maximum_count) {
                    if (has_varargs) {
                        tc_set_error_at(checker,
                                        expression->source_span,
                                        NULL,
                                        "Call expects at least %zu argument%s but got %zu.",
                                        required_count,
                                        required_count == 1 ? "" : "s",
                                        expression->as.call.arguments.count);
                    } else {
                        tc_set_error_at(checker,
                                        expression->source_span,
                                        NULL,
                                        "Call expects between %zu and %zu argument%s but got %zu.",
                                        required_count,
                                        maximum_count,
                                        maximum_count == 1 ? "" : "s",
                                        expression->as.call.arguments.count);
                    }
                    return NULL;
                }
            }

            for (i = 0; i < expression->as.call.arguments.count; i++) {
                const AstExpression *arg_expr = expression->as.call.arguments.items[i];
                const TypeCheckInfo *argument_info;

                /* For |var (block) parameters: if the argument is an explicit lambda,
                   type-check its return statements against the outer function's return
                   type so that `return expr` inside the block is validated correctly. */
                if (callee_info->parameters) {
                    size_t param_index = (i < callee_info->parameters->count)
                        ? i
                        : callee_info->parameters->count - 1;

                    if (callee_info->parameters->items[param_index].is_block &&
                        arg_expr->kind == AST_EXPR_LAMBDA &&
                        checker->has_outer_return_type) {
                        argument_info = tc_check_lambda_expression(
                            checker, arg_expr,
                            &checker->outer_return_type,
                            &callee_info->parameters->items[param_index].name_span,
                            true);
                        if (!argument_info) {
                            return NULL;
                        }
                        continue;
                    }
                }

                argument_info = tc_check_expression(checker, arg_expr);

                if (!argument_info) {
                    return NULL;
                }

                if (callee_info->parameters) {
                    size_t param_index = (i < callee_info->parameters->count)
                        ? i
                        : callee_info->parameters->count - 1;

                    if (checker->has_error) {
                        return NULL;
                    }

                    /* Untyped (var) parameters accept any argument — skip type check. */
                    if (callee_info->parameters->items[param_index].is_untyped) {
                        continue;
                    }

                    {
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
                    } /* end typed-parameter block */
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
