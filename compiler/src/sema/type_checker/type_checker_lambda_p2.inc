        if (parameter->default_expr == NULL) {
            if (saw_default) {
                tc_set_error_at(checker,
                                parameter->name_span,
                                &parameters->items[i - 1].name_span,
                                "Required parameter '%s' cannot follow a parameter with a default value.",
                                parameter->name);
                return false;
            }
            continue;
        }

        if (parameter->is_varargs) {
            tc_set_error_at(checker,
                            parameter->name_span,
                            NULL,
                            "Varargs parameter '%s' cannot have a default value.",
                            parameter->name);
            return false;
        }

        {
            const TypeCheckInfo *default_info = tc_check_expression(checker,
                                                                    parameter->default_expr);
            CheckedType default_type;

            if (!default_info) {
                return false;
            }

            parameter_type = parameter->is_untyped
                ? tc_checked_type_external()
                : tc_checked_type_from_ast_type(checker, &parameter->type);
            default_type = tc_type_check_source_type(default_info);
            if (checker->has_error) {
                return false;
            }

            if (!parameter->is_untyped &&
                !tc_checked_type_assignable(parameter_type, default_type)) {
                char expected_text[64];
                char actual_text[64];

                checked_type_to_string(parameter_type, expected_text, sizeof(expected_text));
                checked_type_to_string(default_type, actual_text, sizeof(actual_text));
                tc_set_error_at(checker,
                                parameter->default_expr->source_span,
                                &parameter->name_span,
                                "Default value for parameter '%s' expects %s but got %s.",
                                parameter->name,
                                expected_text,
                                actual_text);
                return false;
            }
        }

        if (!tc_validate_default_parameter_references(checker,
                                                      parameter->default_expr,
                                                      parameters,
                                                      i,
                                                      parameter)) {
            return false;
        }

        saw_default = true;
    }

    return true;
}

bool tc_resolve_parameters_in_scope(TypeChecker *checker,
                                    const AstParameterList *parameters,
                                    const Scope *scope) {
    size_t i;

    if (!parameters || !scope) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        const Symbol *symbol = scope_lookup_local(scope, parameters->items[i].name);

        if (!symbol) {
            tc_set_error_at(checker,
                            parameters->items[i].name_span,
                            NULL,
                            "Internal error: missing parameter symbol '%s'.",
                            parameters->items[i].name);
            return false;
        }

        if (!tc_resolve_symbol_info(checker, symbol)) {
            return false;
        }
    }

    return true;
}

const TypeCheckInfo *tc_check_lambda_expression(TypeChecker *checker,
                                                const AstExpression *expression,
                                                const CheckedType *expected_return_type,
                                                const AstSourceSpan *related_span,
                                                bool is_nlr_block) {
    const Scope *lambda_scope;
    CheckedType return_type;
    AstSourceSpan return_span;
    TypeCheckInfo info;

    if (!checker || !expression || expression->kind != AST_EXPR_LAMBDA) {
        return NULL;
    }

    if (!expected_return_type) {
        const TypeCheckInfo *cached_info = type_checker_get_expression_info(checker, expression);

        if (cached_info) {
            return cached_info;
        }
    }

    lambda_scope = symbol_table_find_scope(checker->symbols, expression, SCOPE_KIND_LAMBDA);
    if (!lambda_scope) {
        tc_set_error_at(checker,
                        expression->source_span,
                        NULL,
                        "Internal error: missing lambda scope.");
        return NULL;
    }

    if (!tc_resolve_parameters_in_scope(checker,
                                        &expression->as.lambda.parameters,
                                        lambda_scope)) {
        return NULL;
    }
    if (!tc_check_parameter_defaults(checker, &expression->as.lambda.parameters)) {
        return NULL;
    }

    memset(&return_span, 0, sizeof(return_span));
    if (expression->as.lambda.body.kind == AST_LAMBDA_BODY_BLOCK) {
        BlockContext context;
        CheckedType saved_outer_return_type = checker->outer_return_type;
        bool saved_has_outer = checker->has_outer_return_type;
        bool ok;

        memset(&context, 0, sizeof(context));
        context.kind = BLOCK_CONTEXT_LAMBDA;
        context.has_expected_return_type = (expected_return_type != NULL);
        context.enforce_expected_return_type = (expected_return_type != NULL) && !is_nlr_block;
        if (expected_return_type) {
            context.expected_return_type = *expected_return_type;
        }
        context.owner_span = expression->source_span;
        if (related_span && tc_source_span_is_valid(*related_span)) {
            context.related_span = *related_span;
            context.has_related_span = true;
        }

        /* Update outer_return_type so that |var argument type-checks can
           validate `return expr` arguments against this lambda's return type. */
        if (expected_return_type) {
            checker->outer_return_type = *expected_return_type;
            checker->has_outer_return_type = true;
        }

        ok = tc_check_block(checker,
                            expression->as.lambda.body.as.block,
                            &context,
                            &return_type,
                            &return_span);

        checker->outer_return_type = saved_outer_return_type;
        checker->has_outer_return_type = saved_has_outer;

        if (!ok) {
            return NULL;
        }
    } else {
        const TypeCheckInfo *body_info = tc_check_expression(checker,
                                                             expression->as.lambda.body.as.expression);

        if (!body_info) {
            return NULL;
        }

        return_type = tc_type_check_source_type(body_info);
        return_span = expression->as.lambda.body.as.expression->source_span;

        if (expected_return_type &&
            !tc_checked_type_assignable(*expected_return_type, return_type)) {
            char expected_text[64];
            char actual_text[64];

            checked_type_to_string(*expected_return_type,
                                   expected_text,
                                   sizeof(expected_text));
            checked_type_to_string(return_type, actual_text, sizeof(actual_text));
            tc_set_error_at(checker,
                            return_span,
                            related_span,
                            "Lambda body must produce %s but got %s.",
                            expected_text,
                            actual_text);
            return NULL;
        }
    }

    info = tc_type_check_info_make_callable(return_type, &expression->as.lambda.parameters);
    return tc_store_expression_info(checker, expression, info);
}

bool tc_check_start_decl(TypeChecker *checker, const AstStartDecl *start_decl) {
    const Scope *start_scope;
    CheckedType body_type;
    AstSourceSpan body_span;
    CheckedType int32_type = tc_checked_type_value(AST_PRIMITIVE_INT32, 0);
    char body_text[64];
    const char *entry_name = start_decl->is_boot ? "boot" : "start";

    start_scope = symbol_table_find_scope(checker->symbols, start_decl, SCOPE_KIND_START);
    if (!start_scope) {
        tc_set_error_at(checker, start_decl->start_span, NULL,
                        "Internal error: missing %s scope.", entry_name);
        return false;
    }

    if (!tc_resolve_parameters_in_scope(checker, &start_decl->parameters, start_scope)) {
        return false;
    }
    if (!tc_check_parameter_defaults(checker, &start_decl->parameters)) {
        return false;
    }

#include "type_checker_lambda_p3.inc"
