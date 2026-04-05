#include "type_checker_internal.h"

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
                                                const AstSourceSpan *related_span) {
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

    memset(&return_span, 0, sizeof(return_span));
    if (expression->as.lambda.body.kind == AST_LAMBDA_BODY_BLOCK) {
        BlockContext context;

        memset(&context, 0, sizeof(context));
        context.kind = BLOCK_CONTEXT_LAMBDA;
        context.has_expected_return_type = (expected_return_type != NULL);
        if (expected_return_type) {
            context.expected_return_type = *expected_return_type;
        }
        context.owner_span = expression->source_span;
        if (related_span && tc_source_span_is_valid(*related_span)) {
            context.related_span = *related_span;
            context.has_related_span = true;
        }

        if (!tc_check_block(checker,
                            expression->as.lambda.body.as.block,
                            &context,
                            &return_type,
                            &return_span)) {
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
    CheckedType expected_type = tc_checked_type_value(AST_PRIMITIVE_INT32, 0);
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

    if (start_decl->is_boot) {
        if (start_decl->parameters.count != 0) {
            tc_set_error_at(checker,
                            start_decl->start_span,
                            NULL,
                            "boot must declare zero parameters.");
            return false;
        }
    } else {
        if (start_decl->parameters.count != 1) {
            tc_set_error_at(checker,
                            start_decl->start_span,
                            NULL,
                            "start must declare exactly one parameter of type string[].");
            return false;
        }

        if (start_decl->parameters.items[0].type.kind != AST_TYPE_PRIMITIVE ||
            start_decl->parameters.items[0].type.primitive != AST_PRIMITIVE_STRING ||
            start_decl->parameters.items[0].type.dimension_count != 1 ||
            start_decl->parameters.items[0].type.dimensions[0].has_size) {
            tc_set_error_at(checker,
                            start_decl->parameters.items[0].name_span,
                            &start_decl->start_span,
                            "start parameter must have type string[].");
            return false;
        }
    }

    body_span = start_decl->start_span;
    if (start_decl->body.kind == AST_LAMBDA_BODY_BLOCK) {
        BlockContext context;

        memset(&context, 0, sizeof(context));
        context.kind = BLOCK_CONTEXT_START;
        context.has_expected_return_type = true;
        context.expected_return_type = expected_type;
        context.owner_span = start_decl->start_span;
        context.related_span = start_decl->start_span;
        context.has_related_span = true;

        if (!tc_check_block(checker,
                            start_decl->body.as.block,
                            &context,
                            &body_type,
                            &body_span)) {
            return false;
        }
    } else {
        const TypeCheckInfo *body_info = tc_check_expression(checker, start_decl->body.as.expression);

        if (!body_info) {
            return false;
        }

        body_type = tc_type_check_source_type(body_info);
        body_span = start_decl->body.as.expression->source_span;
    }

    if (!tc_checked_type_assignable(expected_type, body_type)) {
        checked_type_to_string(body_type, body_text, sizeof(body_text));
        tc_set_error_at(checker,
                        body_span,
                        &start_decl->start_span,
                        "%s body must produce int32 but got %s.",
                        entry_name, body_text);
        return false;
    }

    return true;
}
