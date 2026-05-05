bool tc_resolve_binding_symbol(TypeChecker *checker,
                               const Symbol *symbol,
                               const AstType *declared_type,
                               bool is_inferred_type,
                               const AstExpression *initializer,
                               TypeCheckInfo *info) {
    const TypeCheckInfo *initializer_info;
    CheckedType target_type;
    CheckedType source_type;
    char source_text[64];
    char target_text[64];
    CheckedType target_generic_arg;
    CheckedType source_generic_arg;
    bool has_target_generic_arg = false;
    bool has_source_generic_arg = false;

    if (is_inferred_type) {
        initializer_info = tc_check_expression(checker, initializer);
        if (!initializer_info) {
            return false;
        }

        if (!initializer_info->is_callable &&
            (initializer_info->type.kind == CHECKED_TYPE_VOID ||
             initializer_info->type.kind == CHECKED_TYPE_NULL)) {
            tc_set_error_at(checker,
                            initializer ? initializer->source_span : symbol->declaration_span,
                            &symbol->declaration_span,
                            "Cannot infer a type for %s '%s' from %s.",
                            symbol_kind_name(symbol->kind),
                            symbol->name ? symbol->name : "<anonymous>",
                            initializer_info->type.kind == CHECKED_TYPE_VOID
                                ? "a void expression"
                                : "null");
            return false;
        }

        *info = *initializer_info;
        return true;
    }

    target_type = tc_checked_type_from_ast_type(checker, declared_type);
    if (checker->has_error) {
        return false;
    }
    has_target_generic_arg = tc_type_check_info_first_generic_arg(checker,
                                                                  NULL,
                                                                  declared_type,
                                                                  &target_generic_arg);
    if (checker->has_error) {
        return false;
    }

    if (initializer && initializer->kind == AST_EXPR_LAMBDA) {
        initializer_info = tc_check_lambda_expression(checker,
                                                      initializer,
                                                      &target_type,
                                                      &symbol->declaration_span,
                                                      false);
    } else if (tc_checked_type_is_hetero_array(target_type) &&
               initializer && initializer->kind == AST_EXPR_ARRAY_LITERAL) {
        initializer_info = tc_check_hetero_array_literal(checker, initializer);
    } else {
        initializer_info = tc_check_expression(checker, initializer);
    }
    if (!initializer_info) {
        return false;
    }

    if (target_type.kind == CHECKED_TYPE_VOID && !initializer_info->is_callable) {
        tc_set_error_at(checker, symbol->declaration_span, NULL,
                        "%s '%s' cannot have type void unless initialized with a callable expression.",
                        symbol_kind_name(symbol->kind),
                        symbol->name ? symbol->name : "<anonymous>");
        return false;
    }

    source_type = tc_type_check_source_type(initializer_info);
    has_source_generic_arg = tc_type_check_info_first_generic_arg(checker,
                                                                  initializer_info,
                                                                  NULL,
                                                                  &source_generic_arg);
    if (checker->has_error) {
        return false;
    }
    if (!(initializer_info->is_callable && source_type.kind == CHECKED_TYPE_EXTERNAL) &&
        !tc_checked_type_assignable(target_type, source_type)) {
        checked_type_to_string(source_type, source_text, sizeof(source_text));
        checked_type_to_string(target_type, target_text, sizeof(target_text));
        tc_set_error_at(checker,
                        initializer ? initializer->source_span : symbol->declaration_span,
                        &symbol->declaration_span,
                        "Cannot assign expression of type %s to %s '%s' of type %s.",
                        source_text,
                        symbol_kind_name(symbol->kind),
                        symbol->name ? symbol->name : "<anonymous>",
                        target_text);
        return false;
    }

    if (has_target_generic_arg && has_source_generic_arg &&
        !tc_checked_type_assignable(target_generic_arg, source_generic_arg)) {
        char source_generic_text[64];
        char target_generic_text[64];

        checked_type_to_string(source_generic_arg,
                               source_generic_text,
                               sizeof(source_generic_text));
        checked_type_to_string(target_generic_arg,
                               target_generic_text,
                               sizeof(target_generic_text));
        tc_set_error_at(checker,
                        initializer ? initializer->source_span : symbol->declaration_span,
                        &symbol->declaration_span,
                        "Cannot assign expression with generic value type %s to %s '%s' expecting %s.",
                        source_generic_text,
                        symbol_kind_name(symbol->kind),
                        symbol->name ? symbol->name : "<anonymous>",
                        target_generic_text);
        return false;
    }

    *info = *initializer_info;
    info->type = target_type;
    if (has_target_generic_arg) {
        tc_type_check_info_set_first_generic_arg(info, target_generic_arg);
    }
    if (initializer_info->is_callable) {
        info->callable_return_type = target_type;
    }

    return true;
}
