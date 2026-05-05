HirExpression *hr_lower_expression(HirBuildContext *context,
                                   const AstExpression *expression) {
    const TypeCheckInfo *info;
    HirExpression *hir_expression;

    if (!expression) {
        return NULL;
    }

    if (expression->kind == AST_EXPR_GROUPING) {
        return hr_lower_expression(context, expression->as.grouping.inner);
    }

    info = type_checker_get_expression_info(context->checker, expression);
    if (!info) {
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Internal error: missing type info for expression during HIR lowering.");
        return NULL;
    }

    if (expression->kind == AST_EXPR_MEMORY_OP) {
        return hr_lower_memory_expression(context, expression, info);
    }

    if (expression->kind == AST_EXPR_CALL) {
        const char *helper_name = NULL;
        const char *builtin_name = hr_builtin_sequence_name(context, expression);

        if (hr_is_builtin_type_query_call(context, expression, &helper_name)) {
            HirExpression *arguments[4] = { NULL, NULL, NULL, NULL };
            size_t lowered_argument_count =
                strcmp(helper_name, "__calynda_issametype") == 0 ? 4 : 2;
            size_t argument_index;

            for (argument_index = 0;
                 argument_index < expression->as.call.arguments.count;
                 argument_index++) {
                char type_text[128];
                const TypeCheckInfo *argument_info =
                    type_checker_get_expression_info(context->checker,
                                                     expression->as.call.arguments.items[argument_index]);

                if (!argument_info ||
                    !hr_format_helper_type_text(argument_info,
                                               type_text,
                                               sizeof(type_text))) {
                    while (argument_index > 0) {
                        argument_index--;
                        hir_expression_free(arguments[argument_index * 2]);
                        hir_expression_free(arguments[argument_index * 2 + 1]);
                    }
                    hr_set_error(context,
                                 expression->source_span,
                                 NULL,
                                 "Internal error: missing helper type metadata during HIR lowering.");
                    return NULL;
                }

                arguments[argument_index * 2] =
                    hr_lower_expression(context, expression->as.call.arguments.items[argument_index]);
                if (!arguments[argument_index * 2]) {
                    while (argument_index > 0) {
                        argument_index--;
                        hir_expression_free(arguments[argument_index * 2]);
                        hir_expression_free(arguments[argument_index * 2 + 1]);
                    }
                    return NULL;
                }

                arguments[argument_index * 2 + 1] =
                    hr_make_string_literal_expression(context,
                                                      type_text,
                                                      expression->as.call.arguments.items[argument_index]->source_span);
                if (!arguments[argument_index * 2 + 1]) {
                    hir_expression_free(arguments[argument_index * 2]);
                    arguments[argument_index * 2] = NULL;
                    while (argument_index > 0) {
                        argument_index--;
                        hir_expression_free(arguments[argument_index * 2]);
                        hir_expression_free(arguments[argument_index * 2 + 1]);
                    }
                    return NULL;
                }
            }

            return hr_make_helper_call(context,
                                       expression,
                                       helper_name,
                                       info->type,
                                       arguments,
                                       lowered_argument_count);
        }

        if (builtin_name) {
            return hr_lower_builtin_sequence_call(context,
                                                  expression,
                                                  info,
                                                  builtin_name);
        }
    }

    if (expression->kind == AST_EXPR_SPAWN) {
        HirExpression *argument = hr_lower_expression(context, expression->as.spawn.callable);
        HirExpression *arguments[1];
        const char *helper_name = "__calynda_rt_thread_spawn";

        if (!argument) {
            return NULL;
        }
        if (info->type.kind == CHECKED_TYPE_NAMED &&
            info->type.name &&
            strcmp(info->type.name, "Future") == 0) {
            helper_name = "__calynda_rt_future_spawn";
        }
        arguments[0] = argument;
        return hr_make_helper_call(context,
                                   expression,
                                   helper_name,
                                   info->type,
                                   arguments,
                                   1);
    }

    if (hr_is_builtin_mutex_new_call(expression)) {
        return hr_make_helper_call(context,
                                   expression,
                                   "__calynda_rt_mutex_new",
                                   info->type,
                                   NULL,
                                   0);
    }

    if (hr_is_builtin_atomic_new_call(expression)) {
        HirExpression *argument =
            hr_lower_expression(context, expression->as.call.arguments.items[0]);
        HirExpression *arguments[1];

        if (!argument) {
            return NULL;
        }
        arguments[0] = argument;
        return hr_make_helper_call(context,
                                   expression,
                                   "__calynda_rt_atomic_new",
                                   info->type,
                                    arguments,
                                    1);
    }

    if (expression->kind == AST_EXPR_CALL) {
        HirExpression *freestanding_import_call = NULL;

        if (hr_try_lower_freestanding_import_member_call(context,
                                                         expression,
                                                         info,
                                                         &freestanding_import_call)) {
            return freestanding_import_call;
        }
    }

    if (expression->kind == AST_EXPR_CALL && expression->as.call.callee &&
        expression->as.call.callee->kind == AST_EXPR_MEMBER) {
        const char *helper_name = NULL;
        if (hr_is_builtin_thread_or_mutex_call(context, expression, &helper_name)) {
            /* Determine how many arguments the helper takes:
             *   - target value is always argument[0]
             *   - single extra call argument (if any) is argument[1] */
            size_t extra_arg_count = expression->as.call.arguments.count;
            HirExpression *arg_target;
            HirExpression *arguments[2];

            arg_target = hr_lower_expression(context,
                                             expression->as.call.callee->as.member.target);
            if (!arg_target) {
                return NULL;
            }
            arguments[0] = arg_target;

            if (extra_arg_count == 1) {
                HirExpression *extra_arg =
                    hr_lower_expression(context,
                                        expression->as.call.arguments.items[0]);
                if (!extra_arg) {
                    hir_expression_free(arg_target);
                    return NULL;
                }
                arguments[1] = extra_arg;
                return hr_make_helper_call(context,
                                           expression,
                                           helper_name,
                                           info->type,
                                           arguments,
                                           2);
            }

            return hr_make_helper_call(context,
                                       expression,
                                       helper_name,
                                       info->type,
                                       arguments,
                                       1);
        }
    }

#include "hir_lower_expr_p6.inc"
