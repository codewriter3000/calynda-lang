static bool hr_is_builtin_type_query_call(HirBuildContext *context,
                                          const AstExpression *expression,
                                          const char **helper_name_out) {
    const AstExpression *callee;

    if (!context || !expression || expression->kind != AST_EXPR_CALL ||
        !expression->as.call.callee) {
        return false;
    }

    callee = expression->as.call.callee;
    if (callee->kind != AST_EXPR_IDENTIFIER ||
        symbol_table_resolve_identifier(context->symbols, callee) != NULL ||
        !callee->as.identifier) {
        return false;
    }

    if (strcmp(callee->as.identifier, "typeof") == 0) {
        *helper_name_out = "__calynda_typeof";
        return true;
    }
    if (strcmp(callee->as.identifier, "isint") == 0) {
        *helper_name_out = "__calynda_isint";
        return true;
    }
    if (strcmp(callee->as.identifier, "isfloat") == 0) {
        *helper_name_out = "__calynda_isfloat";
        return true;
    }
    if (strcmp(callee->as.identifier, "isbool") == 0) {
        *helper_name_out = "__calynda_isbool";
        return true;
    }
    if (strcmp(callee->as.identifier, "isstring") == 0) {
        *helper_name_out = "__calynda_isstring";
        return true;
    }
    if (strcmp(callee->as.identifier, "isarray") == 0) {
        *helper_name_out = "__calynda_isarray";
        return true;
    }
    if (strcmp(callee->as.identifier, "issametype") == 0) {
        *helper_name_out = "__calynda_issametype";
        return true;
    }

    return false;
}

static HirExpression *hr_make_helper_call(HirBuildContext *context,
                                          const AstExpression *expression,
                                          const char *helper_name,
                                          CheckedType return_type,
                                          HirExpression **arguments,
                                          size_t argument_count) {
    HirExpression *call_expr;
    HirExpression *callee;
    size_t i;
    /* Support up to 4 parameters (current max for alpha.2 helpers). */
    CheckedType parameter_types[4];

    if (!expression || (!arguments && argument_count != 0)) {
        return NULL;
    }

    for (i = 0; i < argument_count && i < 4; i++) {
        parameter_types[i] = arguments[i]->type;
    }

    callee = hr_make_helper_symbol(context,
                                   helper_name,
                                   return_type,
                                   argument_count > 0 ? parameter_types : NULL,
                                   argument_count,
                                   expression->source_span);
    if (!callee) {
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering helper call.");
        return NULL;
    }

    call_expr = hr_expression_new(HIR_EXPR_CALL);
    if (!call_expr) {
        hir_expression_free(callee);
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering helper call.");
        return NULL;
    }

    call_expr->type = return_type;
    call_expr->source_span = expression->source_span;
    call_expr->as.call.callee = callee;
    for (i = 0; i < argument_count; i++) {
        if (!hr_append_argument(&call_expr->as.call, arguments[i])) {
            while (i > 0) {
                i--;
                call_expr->as.call.arguments[i] = NULL;
            }
            hir_expression_free(call_expr);
            hr_set_error(context,
                         expression->source_span,
                         NULL,
                         "Out of memory while lowering helper call.");
            return NULL;
        }
    }

    return call_expr;
}

static const char *hr_builtin_sequence_name(HirBuildContext *context,
                                            const AstExpression *expression) {
    const AstExpression *callee;

    if (!context || !expression || expression->kind != AST_EXPR_CALL ||
        !expression->as.call.callee || expression->as.call.arguments.count != 1) {
        return NULL;
    }

    callee = expression->as.call.callee;
    if (callee->kind != AST_EXPR_IDENTIFIER ||
        symbol_table_resolve_identifier(context->symbols, callee) != NULL ||
        !callee->as.identifier) {
        return NULL;
    }

    return (strcmp(callee->as.identifier, "car") == 0 ||
            strcmp(callee->as.identifier, "cdr") == 0)
               ? callee->as.identifier
               : NULL;
}

static HirExpression *hr_lower_builtin_sequence_call(HirBuildContext *context,
                                                     const AstExpression *expression,
                                                     const TypeCheckInfo *info,
                                                     const char *builtin_name) {
    const AstExpression *argument_expression;
    const TypeCheckInfo *argument_info;
    CheckedType argument_type;
    HirExpression *argument;
    HirExpression *arguments[1];

    if (!context || !expression || !info || !builtin_name ||
        expression->kind != AST_EXPR_CALL ||
        expression->as.call.arguments.count != 1) {
        return NULL;
    }

    argument_expression = expression->as.call.arguments.items[0];
    argument_info = type_checker_get_expression_info(context->checker, argument_expression);
    if (!argument_info) {
        hr_set_error(context,
                     argument_expression->source_span,
                     NULL,
                     "Internal error: missing type info for builtin sequence argument.");
        return NULL;
    }

    argument_type = argument_info->type;
    argument = hr_lower_expression(context, argument_expression);
    if (!argument) {
        return NULL;
    }

    if (argument_type.kind == CHECKED_TYPE_VALUE &&
        argument_type.primitive == AST_PRIMITIVE_STRING &&
        argument_type.array_depth == 0) {
        if (strcmp(builtin_name, "car") == 0) {
            HirExpression *zero = hr_make_zero_integer_literal_expression(
                context,
                argument_expression->source_span);
            HirExpression *index_expr;

            if (!zero) {
                hir_expression_free(argument);
                return NULL;
            }

            index_expr = hr_make_index_expression(context,
                                                  expression,
                                                  info->type,
                                                  argument,
                                                  zero);
            if (!index_expr) {
                hir_expression_free(argument);
                hir_expression_free(zero);
                return NULL;
            }

            return index_expr;
        }

        arguments[0] = argument;
        return hr_make_helper_call(context,
                                   expression,
                                   "__calynda_rt_string_cdr",
                                   info->type,
                                   arguments,
                                   1);
    }

    arguments[0] = argument;
    return hr_make_helper_call(context,
                               expression,
                               strcmp(builtin_name, "car") == 0
                                   ? "__calynda_rt_array_car"
                                   : "__calynda_rt_array_cdr",
                               info->type,
                               arguments,
                               1);
}

#include "hir_lower_expr_p3.inc"
