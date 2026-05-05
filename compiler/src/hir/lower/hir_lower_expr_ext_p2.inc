    case AST_EXPR_LAMBDA:
        {
            const Scope *scope = symbol_table_find_scope(context->symbols,
                                                         expression,
                                                         SCOPE_KIND_LAMBDA);

            if (!scope) {
                hir_expression_free(hir_expression);
                hr_set_error(context,
                             expression->source_span,
                             NULL,
                             "Internal error: missing lambda scope during HIR lowering.");
                return NULL;
            }

            if (!hr_lower_parameters(context,
                                     &hir_expression->as.lambda.parameters,
                                     &expression->as.lambda.parameters,
                                     scope)) {
                hir_expression_free(hir_expression);
                return NULL;
            }
            hir_expression->as.lambda.body = hr_lower_body_to_block(context,
                                                                    &expression->as.lambda.body);
            if (!hir_expression->as.lambda.body) {
                hir_expression_free(hir_expression);
                return NULL;
            }
        }
        return hir_expression;

    case AST_EXPR_CALL:
        {
            const TypeCheckInfo *callee_info =
                type_checker_get_expression_info(context->checker, expression->as.call.callee);
            const AstParameterList *parameters =
                callee_info ? callee_info->parameters : NULL;
            const Symbol **parameter_symbols = NULL;
            const AstExpression **argument_sources = NULL;
            size_t supplied_count = expression->as.call.arguments.count;
            size_t fixed_parameter_count = 0;
            size_t total_argument_count = supplied_count;

            hir_expression->as.call.callee = hr_lower_expression(context,
                                                                 expression->as.call.callee);
            if (!hir_expression->as.call.callee) {
                hir_expression_free(hir_expression);
                return NULL;
            }

            if (parameters) {
                fixed_parameter_count = parameters->count;
                if (fixed_parameter_count > 0 &&
                    parameters->items[fixed_parameter_count - 1].is_varargs) {
                    fixed_parameter_count--;
                }
                if (supplied_count < fixed_parameter_count) {
                    total_argument_count = fixed_parameter_count;
                    parameter_symbols = hr_build_inline_parameter_symbols(context,
                                                                         expression->as.call.callee,
                                                                         parameters);
                    if (parameters->count > 0 && !parameter_symbols) {
                        hir_expression_free(hir_expression);
                        return NULL;
                    }
                    argument_sources = calloc(fixed_parameter_count,
                                              sizeof(*argument_sources));
                    if (!argument_sources) {
                        free((void *)parameter_symbols);
                        hir_expression_free(hir_expression);
                        hr_set_error(context,
                                     expression->source_span,
                                     NULL,
                                     "Out of memory while lowering optional call arguments.");
                        return NULL;
                    }
                    for (i = 0; i < supplied_count; i++) {
                        argument_sources[i] = expression->as.call.arguments.items[i];
                    }
                    for (i = supplied_count; i < fixed_parameter_count; i++) {
                        argument_sources[i] = parameters->items[i].default_expr;
                    }
                }
            }

            for (i = 0; i < total_argument_count; i++) {
                const AstExpression *argument_source =
                    argument_sources ? argument_sources[i]
                                     : expression->as.call.arguments.items[i];
                HirExpression *argument = argument_sources
                    ? hr_lower_optional_argument(context,
                                                 argument_source,
                                                 parameters,
                                                 parameter_symbols,
                                                 argument_sources,
                                                 i)
                    : hr_lower_expression(context, argument_source);

                if (!argument) {
                    free((void *)argument_sources);
                    free((void *)parameter_symbols);
                    hir_expression_free(hir_expression);
                    return NULL;
                }

                /* Tag lambdas passed to |var block parameters as NLR blocks */
                if (argument->kind == HIR_EXPR_LAMBDA && parameters) {
                    size_t param_index = (i < parameters->count)
                        ? i : parameters->count - 1;
                    if (parameters->items[param_index].is_block) {
                        argument->as.lambda.is_nlr_block = true;
                    }
                }

                /* Auto-thunk: wrap a non-lambda primitive value into () -> { return expr; }
                   when passing to an untyped (var) parameter, so the callee can call it. */
                if (parameters) {
                    size_t param_index = (i < parameters->count)
                        ? i : parameters->count - 1;
                    if (parameters->items[param_index].is_untyped &&
                        argument->kind != HIR_EXPR_LAMBDA &&
                        argument->kind != HIR_EXPR_NONLOCAL_RETURN &&
                        argument->type.kind == CHECKED_TYPE_VALUE) {
                        HirExpression *thunk = hr_expression_new(HIR_EXPR_LAMBDA);
                        HirStatement  *ret   = hr_statement_new(HIR_STMT_RETURN);
                        if (!thunk || !ret) {
                            free(thunk);
                            free(ret);
                            free((void *)argument_sources);
                            free((void *)parameter_symbols);
                            hir_expression_free(argument);
                            hir_expression_free(hir_expression);
                            hr_set_error(context,
                                         expression->source_span,
                                         NULL,
                                         "Out of memory while auto-thunking argument.");
                            return NULL;
                        }
                        thunk->as.lambda.body = hr_block_new();
                        if (!thunk->as.lambda.body) {
                            hir_expression_free(thunk);
                            hir_statement_free(ret);
                            free((void *)argument_sources);
                            free((void *)parameter_symbols);
                            hir_expression_free(argument);
                            hir_expression_free(hir_expression);
                            hr_set_error(context,
                                         expression->source_span,
                                         NULL,
                                         "Out of memory while auto-thunking argument.");
                            return NULL;
                        }
                        ret->source_span = argument->source_span;
                        ret->as.return_expression = argument;
                        if (!hr_append_statement(thunk->as.lambda.body, ret)) {
                            hir_expression_free(thunk);
                            free((void *)argument_sources);
                            free((void *)parameter_symbols);
                            hir_expression_free(hir_expression);
                            hr_set_error(context,
                                         expression->source_span,
                                         NULL,
                                         "Out of memory while auto-thunking argument.");
                            return NULL;
                        }
                        thunk->type.kind = CHECKED_TYPE_FUNCTION;
                        thunk->is_callable = true;
                        thunk->callable_signature.return_type = argument->type;
                        thunk->source_span = argument->source_span;
                        argument = thunk;
                    }
                }
                if (!hr_append_argument(&hir_expression->as.call, argument)) {
                    free((void *)argument_sources);
                    free((void *)parameter_symbols);
                    hir_expression_free(argument);
                    hir_expression_free(hir_expression);
                    hr_set_error(context,
                                 expression->source_span,
                                 NULL,
                                 "Out of memory while lowering HIR calls.");
                    return NULL;
                }
            }

            free((void *)argument_sources);
            free((void *)parameter_symbols);
        }
        return hir_expression;

    case AST_EXPR_MEMBER:
        hir_expression->as.member.target = hr_lower_expression(context,
                                                               expression->as.member.target);
        hir_expression->as.member.member = ast_copy_text(expression->as.member.member);
        if (!hir_expression->as.member.member) {
            hir_expression_free(hir_expression);
            hr_set_error(context,
                         expression->source_span,
                         NULL,
                         "Out of memory while lowering HIR member access.");
            return NULL;
        }
        return hir_expression;

    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            HirExpression *element = hr_lower_expression(context,
                                                         expression->as.array_literal.elements.items[i]);

            if (!element) {
                hir_expression_free(hir_expression);
                return NULL;
            }
            if (!hr_append_array_element(&hir_expression->as.array_literal, element)) {
                hir_expression_free(element);
                hir_expression_free(hir_expression);
                hr_set_error(context,
                             expression->source_span,
                             NULL,
                             "Out of memory while lowering HIR array literals.");
                return NULL;
            }
        }
        return hir_expression;

    default:
        hir_expression_free(hir_expression);
        return NULL;
    }
}
