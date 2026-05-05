    for (i = 0; i < expression->as.call.arguments.count; i++) {
        const TypeCheckInfo *argument_info =
            tc_check_expression(checker, expression->as.call.arguments.items[i]);

        if (!argument_info) {
            free(argument_types);
            return true;
        }
        if (tc_type_check_source_type(argument_info).kind == CHECKED_TYPE_VOID) {
            tc_set_error_at(checker,
                            expression->as.call.arguments.items[i]->source_span,
                            NULL,
                            "Call arguments cannot have type void.");
            free(argument_types);
            return true;
        }
        argument_types[i] = tc_type_check_source_type(argument_info);
    }

    {
        const Symbol *selected_symbol = tc_select_overload_symbol(checker,
                                                                  callee_identifier,
                                                                  resolution->overload_set,
                                                                  argument_types,
                                                                  expression->as.call.arguments.count);
        free(argument_types);
        if (!selected_symbol) {
            return true;
        }
        if (!symbol_table_select_resolution((SymbolTable *)checker->symbols,
                                            callee_identifier,
                                            selected_symbol)) {
            tc_set_error_at(checker,
                            callee_identifier->source_span,
                            NULL,
                            "Internal error: failed to record overload resolution.");
            return true;
        }
    }

    return true;
}

static bool tc_check_builtin_type_query_call(TypeChecker *checker,
                                             const AstExpression *expression,
                                             TypeCheckInfo *info) {
    const AstExpression *callee;
    const char *builtin_name;
    size_t expected_argument_count;
    size_t i;

    if (!checker || !expression || expression->kind != AST_EXPR_CALL ||
        !expression->as.call.callee || !info) {
        return false;
    }

    callee = expression->as.call.callee;
    if (callee->kind != AST_EXPR_IDENTIFIER ||
        tc_identifier_has_symbol_or_overload_resolution(checker, callee) ||
        !callee->as.identifier) {
        return false;
    }

    builtin_name = callee->as.identifier;
    if (strcmp(builtin_name, "typeof") != 0 &&
        strcmp(builtin_name, "isint") != 0 &&
        strcmp(builtin_name, "isfloat") != 0 &&
        strcmp(builtin_name, "isbool") != 0 &&
        strcmp(builtin_name, "isstring") != 0 &&
        strcmp(builtin_name, "isarray") != 0 &&
        strcmp(builtin_name, "issametype") != 0) {
        return false;
    }

    expected_argument_count = strcmp(builtin_name, "issametype") == 0 ? 2 : 1;
    if (expression->as.call.arguments.count != expected_argument_count) {
        tc_set_error_at(checker,
                        expression->source_span,
                        NULL,
                        "%s expects exactly %zu argument%s but got %zu.",
                        builtin_name,
                        expected_argument_count,
                        expected_argument_count == 1 ? "" : "s",
                        expression->as.call.arguments.count);
        return true;
    }

    for (i = 0; i < expression->as.call.arguments.count; i++) {
        const TypeCheckInfo *argument_info = tc_check_expression(
            checker,
            expression->as.call.arguments.items[i]);

        if (!argument_info) {
            return true;
        }

        if (tc_type_check_source_type(argument_info).kind == CHECKED_TYPE_VOID) {
            tc_set_error_at(checker,
                            expression->as.call.arguments.items[i]->source_span,
                            NULL,
                            "%s cannot be applied to a void expression.",
                            builtin_name);
            return true;
        }
    }

    *info = tc_type_check_info_make(
        strcmp(builtin_name, "typeof") == 0
            ? tc_checked_type_value(AST_PRIMITIVE_STRING, 0)
            : tc_checked_type_value(AST_PRIMITIVE_BOOL, 0));
    return true;
}

static bool tc_check_builtin_array_call(TypeChecker *checker,
                                        const AstExpression *expression,
                                        TypeCheckInfo *info) {
    const AstExpression *callee;
    const AstExpression *argument;
    const TypeCheckInfo *argument_info;
    CheckedType argument_type;
    const char *builtin_name;
    char argument_text[64];

    if (!checker || !expression || expression->kind != AST_EXPR_CALL ||
        !expression->as.call.callee || !info) {
        return false;
    }

    callee = expression->as.call.callee;
    if (callee->kind != AST_EXPR_IDENTIFIER ||
        tc_identifier_has_symbol_or_overload_resolution(checker, callee) ||
        !callee->as.identifier) {
        return false;
    }

    if (strcmp(callee->as.identifier, "car") != 0 &&
        strcmp(callee->as.identifier, "cdr") != 0) {
        return false;
    }

    builtin_name = callee->as.identifier;
    if (expression->as.call.arguments.count != 1) {
        tc_set_error_at(checker,
                        expression->source_span,
                        NULL,
                        "%s expects exactly 1 argument but got %zu.",
                        builtin_name,
                        expression->as.call.arguments.count);
        return true;
    }

    argument = expression->as.call.arguments.items[0];
    argument_info = tc_check_expression(checker, argument);
    if (!argument_info) {
        return true;
    }

    argument_type = tc_type_check_source_type(argument_info);
    if (tc_checked_type_is_string(argument_type)) {
        *info = tc_type_check_info_make(
            strcmp(builtin_name, "car") == 0
                ? tc_checked_type_value(AST_PRIMITIVE_CHAR, 0)
                : argument_type);
        return true;
    }

    /* generic arr<T> / arr<?> — element type is unknown */
    if (argument_type.kind == CHECKED_TYPE_NAMED &&
        argument_type.name &&
        strcmp(argument_type.name, "arr") == 0) {
        *info = tc_type_check_info_make(
            strcmp(builtin_name, "car") == 0
                ? tc_checked_type_external()
                : argument_type);
        return true;
    }

    if (argument_type.kind != CHECKED_TYPE_VALUE || argument_type.array_depth == 0) {
        checked_type_to_string(argument_type, argument_text, sizeof(argument_text));
        tc_set_error_at(checker,
                        argument->source_span,
                        NULL,
                        "%s expects a homogeneous array (T[]) or string argument but got %s.",
                        builtin_name,
                        argument_text);
        return true;
    }

    *info = tc_type_check_info_make(
        strcmp(builtin_name, "car") == 0
            ? tc_checked_type_value(argument_type.primitive, argument_type.array_depth - 1)
            : argument_type);
    return true;
}

static bool tc_check_builtin_concurrency_call(TypeChecker *checker,
                                              const AstExpression *expression,
                                              TypeCheckInfo *info) {
    const AstExpression *target;
    const char *member_name;

    if (!expression || expression->kind != AST_EXPR_CALL ||
        !expression->as.call.callee ||
        expression->as.call.callee->kind != AST_EXPR_MEMBER ||
        !info) {
        return false;
    }

    target = expression->as.call.callee->as.member.target;
    member_name = expression->as.call.callee->as.member.member;
    if (!target || !member_name) {
        return false;
    }

    if (target->kind == AST_EXPR_IDENTIFIER &&
        target->as.identifier &&
        strcmp(target->as.identifier, "Atomic") == 0 &&
        strcmp(member_name, "new") == 0) {
        const TypeCheckInfo *argument_info;

        if (expression->as.call.arguments.count != 1) {
            tc_set_error_at(checker,
                            expression->source_span,
                            NULL,
                            "Atomic.new expects exactly 1 argument but got %zu.",
                            expression->as.call.arguments.count);
            return true;
        }

        argument_info = tc_check_expression(checker,
                                            expression->as.call.arguments.items[0]);
        if (!argument_info) {
            return true;
        }
        if (tc_type_check_source_type(argument_info).kind == CHECKED_TYPE_VOID) {
            tc_set_error_at(checker,
                            expression->as.call.arguments.items[0]->source_span,
                            NULL,
                            "Atomic.new cannot be initialized from a void expression.");
            return true;
        }

#include "type_checker_expr_ext_p3.inc"
