#include "type_checker_internal.h"

static bool tc_find_parameter_index(const AstParameterList *parameters,
                                    const Symbol *symbol,
                                    size_t *out_index) {
    size_t i;

    if (!parameters || !symbol || !out_index) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        if (strcmp(parameters->items[i].name, symbol->name) == 0) {
            *out_index = i;
            return true;
        }
    }

    return false;
}

static bool tc_validate_default_parameter_references(TypeChecker *checker,
                                                     const AstExpression *expression,
                                                     const AstParameterList *parameters,
                                                     size_t parameter_index,
                                                     const AstParameter *parameter) {
    size_t i;

    if (!expression) {
        return true;
    }

    switch (expression->kind) {
    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *symbol = symbol_table_resolve_identifier(checker->symbols, expression);
            size_t referenced_index;

            if (symbol &&
                symbol->kind == SYMBOL_KIND_PARAMETER &&
                tc_find_parameter_index(parameters, symbol, &referenced_index) &&
                referenced_index >= parameter_index) {
                tc_set_error_at(checker,
                                expression->source_span,
                                &parameter->name_span,
                                "Default value for parameter '%s' can only reference earlier parameters.",
                                parameter->name);
                return false;
            }
        }
        return true;

    case AST_EXPR_LAMBDA:
        return true;

    case AST_EXPR_ASSIGNMENT:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.assignment.target,
                                                        parameters,
                                                        parameter_index,
                                                        parameter) &&
               tc_validate_default_parameter_references(checker,
                                                        expression->as.assignment.value,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_TERNARY:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.ternary.condition,
                                                        parameters,
                                                        parameter_index,
                                                        parameter) &&
               tc_validate_default_parameter_references(checker,
                                                        expression->as.ternary.then_branch,
                                                        parameters,
                                                        parameter_index,
                                                        parameter) &&
               tc_validate_default_parameter_references(checker,
                                                        expression->as.ternary.else_branch,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_BINARY:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.binary.left,
                                                        parameters,
                                                        parameter_index,
                                                        parameter) &&
               tc_validate_default_parameter_references(checker,
                                                        expression->as.binary.right,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_UNARY:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.unary.operand,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_CALL:
        if (!tc_validate_default_parameter_references(checker,
                                                      expression->as.call.callee,
                                                      parameters,
                                                      parameter_index,
                                                      parameter)) {
            return false;
        }
        for (i = 0; i < expression->as.call.arguments.count; i++) {
            if (!tc_validate_default_parameter_references(checker,
                                                          expression->as.call.arguments.items[i],
                                                          parameters,
                                                          parameter_index,
                                                          parameter)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_INDEX:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.index.target,
                                                        parameters,
                                                        parameter_index,
                                                        parameter) &&
               tc_validate_default_parameter_references(checker,
                                                        expression->as.index.index,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_MEMBER:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.member.target,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_CAST:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.cast.expression,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            if (!tc_validate_default_parameter_references(checker,
                                                          expression->as.array_literal.elements.items[i],
                                                          parameters,
                                                          parameter_index,
                                                          parameter)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                const AstTemplatePart *part =
                    &expression->as.literal.as.template_parts.items[i];

                if (part->kind == AST_TEMPLATE_PART_EXPRESSION &&
                    !tc_validate_default_parameter_references(checker,
                                                              part->as.expression,
                                                              parameters,
                                                              parameter_index,
                                                              parameter)) {
                    return false;
                }
            }
        }
        return true;

    case AST_EXPR_GROUPING:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.grouping.inner,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_POST_INCREMENT:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.post_increment.operand,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_POST_DECREMENT:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.post_decrement.operand,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);

    case AST_EXPR_DISCARD:
        return true;

    case AST_EXPR_MEMORY_OP:
        for (i = 0; i < expression->as.memory_op.arguments.count; i++) {
            if (!tc_validate_default_parameter_references(checker,
                                                          expression->as.memory_op.arguments.items[i],
                                                          parameters,
                                                          parameter_index,
                                                          parameter)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_SPAWN:
        return tc_validate_default_parameter_references(checker,
                                                        expression->as.spawn.callable,
                                                        parameters,
                                                        parameter_index,
                                                        parameter);
    }

    return true;
}

bool tc_check_parameter_defaults(TypeChecker *checker,
                                 const AstParameterList *parameters) {
    size_t i;
    bool saw_default = false;

    if (!checker || !parameters) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        const AstParameter *parameter = &parameters->items[i];
        CheckedType parameter_type;

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

            parameter_type = tc_checked_type_from_ast_type(checker, &parameter->type);
            default_type = tc_type_check_source_type(default_info);
            if (checker->has_error) {
                return false;
            }

            if (!tc_checked_type_assignable(parameter_type, default_type)) {
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
    if (!tc_check_parameter_defaults(checker, &expression->as.lambda.parameters)) {
        return NULL;
    }

    memset(&return_span, 0, sizeof(return_span));
    if (expression->as.lambda.body.kind == AST_LAMBDA_BODY_BLOCK) {
        BlockContext context;

        memset(&context, 0, sizeof(context));
        context.kind = BLOCK_CONTEXT_LAMBDA;
        context.has_expected_return_type = (expected_return_type != NULL);
        context.enforce_expected_return_type = (expected_return_type != NULL);
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

    if (start_decl->is_boot) {
        if (start_decl->parameters.count != 0) {
            tc_set_error_at(checker,
                            start_decl->start_span,
                            NULL,
                            "boot must declare zero parameters.");
            return false;
        }
    } else {
        if (start_decl->parameters.count > 1) {
            tc_set_error_at(checker,
                            start_decl->start_span,
                            NULL,
                            "start must declare zero parameters or one parameter of type string[].");
            return false;
        }

        if (start_decl->parameters.count == 1 &&
            (start_decl->parameters.items[0].type.kind != AST_TYPE_PRIMITIVE ||
             start_decl->parameters.items[0].type.primitive != AST_PRIMITIVE_STRING ||
             start_decl->parameters.items[0].type.dimension_count != 1 ||
             start_decl->parameters.items[0].type.dimensions[0].has_size)) {
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

    if (body_type.kind == CHECKED_TYPE_VOID) {
        return true;
    }

    if (!tc_checked_type_assignable(int32_type, body_type)) {
        checked_type_to_string(body_type, body_text, sizeof(body_text));
        tc_set_error_at(checker,
                        body_span,
                        &start_decl->start_span,
                        "%s body must produce int32 or void but got %s.",
                        entry_name,
                        body_text);
        return false;
    }

    return true;
}
