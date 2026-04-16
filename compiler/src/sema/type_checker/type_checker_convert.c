#include "type_checker_internal.h"

static CheckedType tc_checked_type_from_generic_ast(const AstType *type) {
    if (!type) {
        return tc_checked_type_invalid();
    }

    switch (type->kind) {
    case AST_TYPE_VOID:
        return tc_checked_type_void();
    case AST_TYPE_PRIMITIVE:
        return tc_checked_type_value(type->primitive, type->dimension_count);
    case AST_TYPE_ARR:
        return tc_checked_type_named("arr", type->generic_args.count, type->dimension_count);
    case AST_TYPE_PTR:
        return tc_checked_type_named("ptr", 1, type->dimension_count);
    case AST_TYPE_NAMED:
        return tc_checked_type_named(type->name, type->generic_args.count, type->dimension_count);
    case AST_TYPE_THREAD:
        return tc_checked_type_named("Thread", 0, type->dimension_count);
    case AST_TYPE_MUTEX:
        return tc_checked_type_named("Mutex", 0, type->dimension_count);
    case AST_TYPE_FUTURE:
        return tc_checked_type_named("Future", 1, type->dimension_count);
    case AST_TYPE_ATOMIC:
        return tc_checked_type_named("Atomic", 1, type->dimension_count);
    }

    return tc_checked_type_invalid();
}

CheckedType tc_promote_numeric_types(CheckedType left, CheckedType right) {
    int width;

    if (!tc_checked_type_is_numeric(left) || !tc_checked_type_is_numeric(right)) {
        return tc_checked_type_invalid();
    }

    if (tc_primitive_is_float(left.primitive) || tc_primitive_is_float(right.primitive)) {
        if (left.primitive == AST_PRIMITIVE_FLOAT64 || right.primitive == AST_PRIMITIVE_FLOAT64) {
            return tc_checked_type_value(AST_PRIMITIVE_FLOAT64, 0);
        }
        return tc_checked_type_value(AST_PRIMITIVE_FLOAT32, 0);
    }

    width = tc_primitive_width(left.primitive);
    if (tc_primitive_width(right.primitive) > width) {
        width = tc_primitive_width(right.primitive);
    }

    if (tc_primitive_is_signed(left.primitive) == tc_primitive_is_signed(right.primitive)) {
        return tc_primitive_is_signed(left.primitive)
                   ? tc_checked_type_value(tc_signed_primitive_for_width(width), 0)
                   : tc_checked_type_value(tc_unsigned_primitive_for_width(width), 0);
    }

    if (width < 64) {
        return tc_checked_type_value(AST_PRIMITIVE_INT64, 0);
    }

    if (!tc_primitive_is_signed(left.primitive) || !tc_primitive_is_signed(right.primitive)) {
        return tc_checked_type_value(AST_PRIMITIVE_UINT64, 0);
    }

    return tc_checked_type_value(AST_PRIMITIVE_INT64, 0);
}

CheckedType tc_checked_type_from_resolved_type(ResolvedType type) {
    switch (type.kind) {
    case RESOLVED_TYPE_INVALID:
        return tc_checked_type_invalid();
    case RESOLVED_TYPE_VOID:
        return tc_checked_type_void();
    case RESOLVED_TYPE_VALUE:
        return tc_checked_type_value(type.primitive, type.array_depth);
    case RESOLVED_TYPE_NAMED:
        return tc_checked_type_named(type.name, type.generic_arg_count, type.array_depth);
    }

    return tc_checked_type_invalid();
}

CheckedType tc_checked_type_from_ast_type(TypeChecker *checker, const AstType *type) {
    const ResolvedType *resolved_type;
    CheckedType result;

    if (!checker || !type) {
        return tc_checked_type_invalid();
    }

    resolved_type = type_resolver_get_type(&checker->resolver, type);
    if (!resolved_type) {
        tc_set_error(checker, "Internal error: missing resolved source type.");
        return tc_checked_type_invalid();
    }

    result = tc_checked_type_from_resolved_type(*resolved_type);

    /* Detect ptr<T, checked> — the "checked" second arg is a bounds-check qualifier. */
    if (type->kind == AST_TYPE_PTR &&
        type->generic_args.count == 2 &&
        type->generic_args.items[1].kind == AST_GENERIC_ARG_TYPE &&
        type->generic_args.items[1].type != NULL &&
        type->generic_args.items[1].type->kind == AST_TYPE_NAMED &&
        type->generic_args.items[1].type->name != NULL &&
        strcmp(type->generic_args.items[1].type->name, "checked") == 0) {
        result.is_bounds_checked = true;
        result.generic_arg_count = 1; /* treat "checked" as a qualifier, not a type arg */
    }

    return result;
}

CheckedType tc_checked_type_from_cast_target(TypeChecker *checker,
                                             const AstExpression *expression) {
    const ResolvedType *resolved_type;

    if (!checker || !expression) {
        return tc_checked_type_invalid();
    }

    resolved_type = type_resolver_get_cast_target_type(&checker->resolver, expression);
    if (!resolved_type) {
        tc_set_error(checker,
                     "Internal error: missing resolved cast target type.");
        return tc_checked_type_invalid();
    }

    return tc_checked_type_from_resolved_type(*resolved_type);
}

bool tc_checked_type_assignable(CheckedType target, CheckedType source) {
    if (target.kind == CHECKED_TYPE_INVALID || source.kind == CHECKED_TYPE_INVALID) {
        return false;
    }

    if (tc_checked_type_equals(target, source)) {
        return true;
    }

    if (target.kind == CHECKED_TYPE_FUNCTION && source.kind == CHECKED_TYPE_FUNCTION) {
        return target.generic_arg_count == source.generic_arg_count;
    }

    if (target.kind == CHECKED_TYPE_VOID) {
        return source.kind == CHECKED_TYPE_VOID ||
               source.kind == CHECKED_TYPE_NULL ||
               source.kind == CHECKED_TYPE_EXTERNAL;
    }

    if (source.kind == CHECKED_TYPE_VOID) {
        return false;
    }

    if (target.kind == CHECKED_TYPE_EXTERNAL) {
        return source.kind != CHECKED_TYPE_VOID;
    }

    if (source.kind == CHECKED_TYPE_EXTERNAL) {
        return target.kind != CHECKED_TYPE_VOID;
    }

    if (target.kind == CHECKED_TYPE_VALUE && source.kind == CHECKED_TYPE_NULL) {
        return tc_checked_type_is_reference_like(target);
    }

    if (target.kind == CHECKED_TYPE_VALUE && source.kind == CHECKED_TYPE_VALUE) {
        if (target.array_depth != source.array_depth) {
            return false;
        }

        if (target.array_depth == 0 &&
            tc_checked_type_is_numeric(target) && tc_checked_type_is_numeric(source)) {
            return true;
        }
    }

    /* arr<?> accepts any single-dimension array or another arr<?> */
    if (tc_checked_type_is_hetero_array(target)) {
        if (source.kind == CHECKED_TYPE_VALUE && source.array_depth == 1) {
            return true;
        }
        if (tc_checked_type_is_hetero_array(source)) {
            return true;
        }
    }

    /* ptr<T> is a typed int64 alias — int64 is assignable to ptr<T> and back */
    {
        bool src_ptr = source.kind == CHECKED_TYPE_NAMED && source.name != NULL &&
                       strcmp(source.name, "ptr") == 0;
        bool tgt_ptr = target.kind == CHECKED_TYPE_NAMED && target.name != NULL &&
                       strcmp(target.name, "ptr") == 0;
        bool src_i64 = source.kind == CHECKED_TYPE_VALUE && source.array_depth == 0 &&
                       tc_checked_type_is_integral(source);
        bool tgt_i64 = target.kind == CHECKED_TYPE_VALUE && target.array_depth == 0 &&
                       tc_checked_type_is_integral(target);
        if ((src_ptr && tgt_i64) || (tgt_ptr && src_i64) || (src_ptr && tgt_ptr)) {
            return true;
        }
    }

    return false;
}

bool tc_merge_types_for_inference(CheckedType left, CheckedType right,
                                  CheckedType *merged) {
    if (!merged) {
        return false;
    }

    if (tc_checked_type_equals(left, right)) {
        *merged = left;
        return true;
    }

    if (left.kind == CHECKED_TYPE_EXTERNAL && right.kind == CHECKED_TYPE_EXTERNAL) {
        *merged = tc_checked_type_external();
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
