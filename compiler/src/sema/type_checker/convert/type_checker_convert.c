#include "type_checker_internal.h"

static bool tc_checked_type_is_pointer_like_named(CheckedType type) {
    if (type.kind != CHECKED_TYPE_NAMED || type.name == NULL) {
        return false;
    }

    return strcmp(type.name, "ptr") == 0 || strcmp(type.name, "mmio") == 0;
}

static bool tc_parse_array_size_literal(const char *text,
                                        unsigned long long *value_out) {
    char *end = NULL;
    unsigned long long value;

    if (!text || text[0] == '\0') {
        return false;
    }

    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno != 0 || !end || *end != '\0') {
        return false;
    }

    if (value_out) {
        *value_out = value;
    }
    return true;
}

static bool tc_copy_ast_array_extents(TypeChecker *checker,
                                      const AstType *type,
                                      CheckedType *checked_type) {
    ArrayExtent *array_extents;
    size_t i;

    if (!checker || !type || !checked_type) {
        return false;
    }

    if (checked_type->array_depth == 0) {
        checked_type->array_extents = NULL;
        return true;
    }

    if (!tc_allocate_owned_array_extents(checker,
                                         checked_type->array_depth,
                                         &array_extents)) {
        return false;
    }

    for (i = 0; i < checked_type->array_depth; i++) {
        if (type->dimensions[i].has_size) {
            unsigned long long size_value = 0;

            if (!tc_parse_array_size_literal(type->dimensions[i].size_literal, &size_value) ||
                size_value == 0) {
                tc_set_error(checker,
                             "Internal error: invalid checked array extent '%s'.",
                             type->dimensions[i].size_literal
                                 ? type->dimensions[i].size_literal
                                 : "<missing>");
                return false;
            }

            array_extents[i].has_size = true;
            array_extents[i].size = size_value;
        }
    }

    checked_type->array_extents = array_extents;
    return true;
}

static CheckedType tc_checked_type_from_generic_ast(TypeChecker *checker,
                                                    const AstType *type) {
    CheckedType result;

    if (!type) {
        return tc_checked_type_invalid();
    }

    switch (type->kind) {
    case AST_TYPE_VOID:
        result = tc_checked_type_void();
        break;
    case AST_TYPE_PRIMITIVE:
        result = tc_checked_type_value(type->primitive, type->dimension_count);
        break;
    case AST_TYPE_ARR:
        result = tc_checked_type_named("arr",
                                       type->generic_args.count,
                                       type->dimension_count);
        break;
    case AST_TYPE_PTR:
        result = tc_checked_type_named("ptr", 1, type->dimension_count);
        break;
    case AST_TYPE_NAMED:
        result = tc_checked_type_named(type->name,
                                       type->generic_args.count,
                                       type->dimension_count);
        break;
    case AST_TYPE_THREAD:
        result = tc_checked_type_named("Thread", 0, type->dimension_count);
        break;
    case AST_TYPE_MUTEX:
        result = tc_checked_type_named("Mutex", 0, type->dimension_count);
        break;
    case AST_TYPE_FUTURE:
        result = tc_checked_type_named("Future", 1, type->dimension_count);
        break;
    case AST_TYPE_ATOMIC:
        result = tc_checked_type_named("Atomic", 1, type->dimension_count);
        break;
    default:
        return tc_checked_type_invalid();
    }

    if (result.array_depth > 0 &&
        !tc_copy_ast_array_extents(checker, type, &result)) {
        return tc_checked_type_invalid();
    }

    return result;
}

static bool tc_checked_type_array_extents_assignable(CheckedType target,
                                                     CheckedType source) {
    size_t i;

    if (target.array_depth != source.array_depth) {
        return false;
    }

    for (i = 0; i < target.array_depth; i++) {
        bool target_has_size = false;
        bool source_has_size = false;
        unsigned long long target_size = 0;
        unsigned long long source_size = 0;

        if (!tc_checked_type_array_extent(target, i, &target_has_size, &target_size) ||
            !tc_checked_type_array_extent(source, i, &source_has_size, &source_size)) {
            return false;
        }

        if (target_has_size && (!source_has_size || target_size != source_size)) {
            return false;
        }
    }

    return true;
}

CheckedType tc_promote_numeric_types(CheckedType left, CheckedType right) {
    int width;

    if (!tc_checked_type_is_numeric(left) || !tc_checked_type_is_numeric(right)) {
        return tc_checked_type_invalid();
    }

    /* If either operand is the 'num' supertype, result stays 'num' */
    if (tc_checked_type_is_num(left) || tc_checked_type_is_num(right)) {
        return tc_checked_type_named("num", 0, 0);
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
    CheckedType checked;

    switch (type.kind) {
    case RESOLVED_TYPE_INVALID:
        return tc_checked_type_invalid();
    case RESOLVED_TYPE_VOID:
        return tc_checked_type_void();
    case RESOLVED_TYPE_VALUE:
        checked = tc_checked_type_value(type.primitive, type.array_depth);
        checked.array_extents = type.array_extents;
        return checked;
    case RESOLVED_TYPE_NAMED:
        checked = tc_checked_type_named(type.name,
                                        type.generic_arg_count,
                                        type.array_depth);
        checked.array_extents = type.array_extents;
        return checked;
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
        /*
         * Archive-import metadata carries standalone AstType nodes for exported
         * callable/value signatures. Those nodes are not part of the program AST
         * walked by the resolver, so they legitimately have no resolver entry.
         * Fall back to structural conversion in that case.
         */
        result = tc_checked_type_from_generic_ast(checker, type);
    } else {
        result = tc_checked_type_from_resolved_type(*resolved_type);
    }

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

    /* num accepts any numeric primitive; any numeric primitive accepts num */
    if (tc_checked_type_is_num(target)) {
        return tc_checked_type_is_numeric(source);
    }
    if (tc_checked_type_is_num(source)) {
        return tc_checked_type_is_numeric(target);
    }

    if (target.kind == CHECKED_TYPE_VALUE && source.kind == CHECKED_TYPE_NULL) {
        return tc_checked_type_is_reference_like(target);
    }

    if (target.kind == CHECKED_TYPE_VALUE && source.kind == CHECKED_TYPE_VALUE) {
        if (target.array_depth != source.array_depth) {
            return false;
        }

        if (!tc_checked_type_array_extents_assignable(target, source)) {
            return false;
        }

        if (target.array_depth == 0 &&
            tc_checked_type_is_numeric(target) && tc_checked_type_is_numeric(source)) {
            return true;
        }

        return tc_primitive_canonical(target.primitive) ==
               tc_primitive_canonical(source.primitive);
    }

    if (target.kind == CHECKED_TYPE_NAMED && source.kind == CHECKED_TYPE_NAMED &&
        target.generic_arg_count == source.generic_arg_count &&
        target.array_depth == source.array_depth &&
        tc_checked_type_array_extents_assignable(target, source)) {
        if (!target.name || !source.name) {
            return target.name == source.name;
        }
        if (strcmp(target.name, source.name) == 0) {
            return true;
        }
    }

    /* arr<?> accepts any single-dimension primitive array, string
     * (which is semantically an array of char), or another arr<?> */
    if (tc_checked_type_is_hetero_array(target)) {
        if (source.kind == CHECKED_TYPE_VALUE && source.array_depth == 1) {
            return true;
        }
        if (tc_checked_type_is_string(source)) {
            return true;
        }
        if (tc_checked_type_is_hetero_array(source)) {
            return true;
        }
    }

    /* ptr<T>/mmio<T> are typed integral aliases — integers and pointer-like values can cross-assign */
    {
        bool src_ptr = tc_checked_type_is_pointer_like_named(source);
        bool tgt_ptr = tc_checked_type_is_pointer_like_named(target);
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

#include "type_checker_convert_p2.inc"
