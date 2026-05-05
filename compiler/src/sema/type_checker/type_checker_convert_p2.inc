    /* One side is external (dynamically typed): yield the concrete type */
    if (left.kind == CHECKED_TYPE_EXTERNAL && right.kind != CHECKED_TYPE_VOID) {
        *merged = right;
        return true;
    }
    if (right.kind == CHECKED_TYPE_EXTERNAL && left.kind != CHECKED_TYPE_VOID) {
        *merged = left;
        return true;
    }

    /* num + any numeric = num */
    if (tc_checked_type_is_num(left) && tc_checked_type_is_numeric(right)) {
        *merged = left;
        return true;
    }
    if (tc_checked_type_is_num(right) && tc_checked_type_is_numeric(left)) {
        *merged = right;
        return true;
    }

    if (left.kind == CHECKED_TYPE_VALUE && right.kind == CHECKED_TYPE_VALUE &&
        left.array_depth == 0 && right.array_depth == 0 &&
        tc_checked_type_is_numeric(left) && tc_checked_type_is_numeric(right)) {
        *merged = tc_promote_numeric_types(left, right);
        return true;
    }

    return false;
}

bool tc_merge_return_types(CheckedType left, CheckedType right,
                           CheckedType *merged) {
    if ((left.kind == CHECKED_TYPE_VOID && right.kind == CHECKED_TYPE_NULL) ||
        (left.kind == CHECKED_TYPE_NULL && right.kind == CHECKED_TYPE_VOID)) {
        *merged = tc_checked_type_void();
        return true;
    }

    return tc_merge_types_for_inference(left, right, merged);
}

TypeCheckInfo tc_type_check_info_make(CheckedType type) {
    TypeCheckInfo info;

    memset(&info, 0, sizeof(info));
    info.type = type;
    return info;
}

TypeCheckInfo tc_type_check_info_make_callable(CheckedType return_type,
                                               const AstParameterList *parameters) {
    TypeCheckInfo info = tc_type_check_info_make(return_type);
    info.is_callable = true;
    info.callable_return_type = return_type;
    info.parameters = parameters;
    return info;
}

TypeCheckInfo tc_type_check_info_make_external_value(void) {
    return tc_type_check_info_make(tc_checked_type_external());
}

TypeCheckInfo tc_type_check_info_make_external_callable(void) {
    TypeCheckInfo info = tc_type_check_info_make_callable(tc_checked_type_external(), NULL);
    info.type = tc_checked_type_external();
    return info;
}

void tc_type_check_info_set_first_generic_arg(TypeCheckInfo *info,
                                              CheckedType generic_arg_type) {
    if (!info) {
        return;
    }

    info->has_first_generic_arg = true;
    info->first_generic_arg_type = generic_arg_type;
}

bool tc_checked_type_first_generic_arg_from_ast_type(TypeChecker *checker,
                                                     const AstType *type,
                                                     CheckedType *generic_arg_type) {
    bool supported_generic_host = false;

    if (!checker || !type || !generic_arg_type ||
        type->generic_args.count == 0 ||
        type->generic_args.items[0].kind != AST_GENERIC_ARG_TYPE ||
        !type->generic_args.items[0].type) {
        return false;
    }

    supported_generic_host = type->kind == AST_TYPE_FUTURE ||
                             type->kind == AST_TYPE_ATOMIC ||
                             (type->kind == AST_TYPE_NAMED &&
                              type->name != NULL &&
                              (strcmp(type->name, "Future") == 0 ||
                               strcmp(type->name, "Atomic") == 0));
    if (!supported_generic_host) {
        return false;
    }

    *generic_arg_type = tc_checked_type_from_generic_ast(type->generic_args.items[0].type);
    return generic_arg_type->kind != CHECKED_TYPE_INVALID;
}

bool tc_type_check_info_first_generic_arg(TypeChecker *checker,
                                          const TypeCheckInfo *info,
                                          const AstType *fallback_type,
                                          CheckedType *generic_arg_type) {
    if (!generic_arg_type) {
        return false;
    }

    if (info && info->has_first_generic_arg) {
        *generic_arg_type = info->first_generic_arg_type;
        return true;
    }

    return tc_checked_type_first_generic_arg_from_ast_type(checker,
                                                           fallback_type,
                                                           generic_arg_type);
}

CheckedType tc_type_check_source_type(const TypeCheckInfo *info) {
    if (!info) {
        return tc_checked_type_invalid();
    }

    return info->is_callable ? info->callable_return_type : info->type;
}

const AstSourceSpan *tc_block_context_related_span(const BlockContext *context,
                                                   AstSourceSpan primary_span) {
    if (!context || !context->has_related_span ||
        !tc_source_span_is_valid(context->related_span)) {
        return NULL;
    }

    if (tc_source_span_is_valid(primary_span) &&
        primary_span.start_line == context->related_span.start_line &&
        primary_span.start_column == context->related_span.start_column) {
        return NULL;
    }

    return &context->related_span;
}

bool tc_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}
