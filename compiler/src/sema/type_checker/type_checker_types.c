#include "type_checker_internal.h"

static bool tc_register_owned_array_extent_block(TypeChecker *checker,
                                                 ArrayExtent *array_extents) {
    if (!checker || !array_extents) {
        return false;
    }

    if (!tc_reserve_items((void **)&checker->owned_array_extent_blocks,
                          &checker->owned_array_extent_block_capacity,
                          checker->owned_array_extent_block_count + 1,
                          sizeof(*checker->owned_array_extent_blocks))) {
        tc_set_error(checker,
                     "Out of memory while storing checked array extent metadata.");
        return false;
    }

    checker->owned_array_extent_blocks[checker->owned_array_extent_block_count++] =
        array_extents;
    return true;
}

bool tc_allocate_owned_array_extents(TypeChecker *checker,
                                     size_t count,
                                     ArrayExtent **array_extents_out) {
    ArrayExtent *array_extents;

    if (!checker || !array_extents_out) {
        return false;
    }

    *array_extents_out = NULL;
    if (count == 0) {
        return true;
    }

    array_extents = calloc(count, sizeof(*array_extents));
    if (!array_extents) {
        tc_set_error(checker,
                     "Out of memory while allocating checked array extent metadata.");
        return false;
    }

    if (!tc_register_owned_array_extent_block(checker, array_extents)) {
        free(array_extents);
        return false;
    }

    *array_extents_out = array_extents;
    return true;
}

bool tc_checked_type_array_extent(CheckedType type,
                                  size_t index,
                                  bool *has_size_out,
                                  unsigned long long *size_out) {
    if (index >= type.array_depth) {
        return false;
    }

    if (has_size_out) {
        *has_size_out = false;
    }
    if (size_out) {
        *size_out = 0;
    }

    if (!type.array_extents) {
        return true;
    }

    if (has_size_out) {
        *has_size_out = type.array_extents[index].has_size;
    }
    if (size_out) {
        *size_out = type.array_extents[index].size;
    }
    return true;
}

CheckedType tc_checked_type_consume_array_prefix(CheckedType type,
                                                 size_t consumed_dimensions) {
    if (consumed_dimensions == 0) {
        return type;
    }

    if (consumed_dimensions >= type.array_depth) {
        type.array_depth = 0;
        type.array_extents = NULL;
        return type;
    }

    type.array_depth -= consumed_dimensions;
    if (type.array_extents) {
        type.array_extents += consumed_dimensions;
    }
    return type;
}

bool tc_checked_type_prepend_array_extent(TypeChecker *checker,
                                          CheckedType element_type,
                                          bool has_size,
                                          unsigned long long size,
                                          CheckedType *array_type_out) {
    ArrayExtent *array_extents;
    CheckedType array_type;
    size_t i;

    if (!checker || !array_type_out) {
        return false;
    }

    array_type = element_type;
    array_type.array_depth = element_type.array_depth + 1;

    if (!tc_allocate_owned_array_extents(checker,
                                         array_type.array_depth,
                                         &array_extents)) {
        return false;
    }

    array_extents[0].has_size = has_size;
    array_extents[0].size = size;
    for (i = 0; i < element_type.array_depth; i++) {
        bool element_has_size = false;
        unsigned long long element_size = 0;

        if (!tc_checked_type_array_extent(element_type,
                                          i,
                                          &element_has_size,
                                          &element_size)) {
            tc_set_error(checker,
                         "Internal error: invalid checked array extent metadata.");
            return false;
        }

        array_extents[i + 1].has_size = element_has_size;
        array_extents[i + 1].size = element_size;
    }

    array_type.array_extents = array_extents;
    *array_type_out = array_type;
    return true;
}

bool tc_checked_type_fill_missing_array_extents(TypeChecker *checker,
                                                CheckedType target,
                                                CheckedType source,
                                                CheckedType *merged_out) {
    ArrayExtent *array_extents;
    CheckedType merged_type;
    size_t i;
    bool needs_merge = false;

    if (!checker || !merged_out) {
        return false;
    }

    *merged_out = target;
    if (target.array_depth == 0 || target.array_depth != source.array_depth) {
        return true;
    }

    for (i = 0; i < target.array_depth; i++) {
        bool target_has_size = false;
        bool source_has_size = false;
        unsigned long long ignored_size = 0;

        if (!tc_checked_type_array_extent(target, i, &target_has_size, &ignored_size) ||
            !tc_checked_type_array_extent(source, i, &source_has_size, &ignored_size)) {
            tc_set_error(checker,
                         "Internal error: invalid checked array extent metadata.");
            return false;
        }

        if (!target_has_size && source_has_size) {
            needs_merge = true;
            break;
        }
    }

    if (!needs_merge) {
        return true;
    }

    if (!tc_allocate_owned_array_extents(checker,
                                         target.array_depth,
                                         &array_extents)) {
        return false;
    }

    for (i = 0; i < target.array_depth; i++) {
        bool target_has_size = false;
        bool source_has_size = false;
        unsigned long long target_size = 0;
        unsigned long long source_size = 0;

        if (!tc_checked_type_array_extent(target, i, &target_has_size, &target_size) ||
            !tc_checked_type_array_extent(source, i, &source_has_size, &source_size)) {
            tc_set_error(checker,
                         "Internal error: invalid checked array extent metadata.");
            return false;
        }

        array_extents[i].has_size = target_has_size || source_has_size;
        array_extents[i].size = target_has_size ? target_size : source_size;
    }

    merged_type = target;
    merged_type.array_extents = array_extents;
    *merged_out = merged_type;
    return true;
}

bool tc_checked_type_has_runtime_omitted_array_extent(const AstType *declared_type,
                                                      CheckedType resolved_type) {
    size_t i;

    if (!declared_type || declared_type->dimension_count == 0) {
        return false;
    }

    for (i = 0; i < declared_type->dimension_count; i++) {
        bool has_size = false;
        unsigned long long size = 0;

        if (declared_type->dimensions[i].has_size) {
            continue;
        }

        if (!tc_checked_type_array_extent(resolved_type, i, &has_size, &size)) {
            return false;
        }

        if (!has_size) {
            return true;
        }
    }

    return false;
}

static bool tc_checked_type_array_extents_equal(CheckedType left, CheckedType right) {
    size_t i;

    if (left.array_depth != right.array_depth) {
        return false;
    }

    for (i = 0; i < left.array_depth; i++) {
        bool left_has_size = false;
        bool right_has_size = false;
        unsigned long long left_size = 0;
        unsigned long long right_size = 0;

        if (!tc_checked_type_array_extent(left, i, &left_has_size, &left_size) ||
            !tc_checked_type_array_extent(right, i, &right_has_size, &right_size)) {
            return false;
        }

        if (left_has_size != right_has_size) {
            return false;
        }
        if (left_has_size && left_size != right_size) {
            return false;
        }
    }

    return true;
}

CheckedType tc_checked_type_invalid(void) {
    CheckedType type;

    memset(&type, 0, sizeof(type));
    type.kind = CHECKED_TYPE_INVALID;
    return type;
}

CheckedType tc_checked_type_void(void) {
    CheckedType type = tc_checked_type_invalid();
    type.kind = CHECKED_TYPE_VOID;
    return type;
}

CheckedType tc_checked_type_null(void) {
    CheckedType type = tc_checked_type_invalid();
    type.kind = CHECKED_TYPE_NULL;
    return type;
}

CheckedType tc_checked_type_external(void) {
    CheckedType type = tc_checked_type_invalid();
    type.kind = CHECKED_TYPE_EXTERNAL;
    return type;
}

CheckedType tc_checked_type_value(AstPrimitiveType primitive, size_t array_depth) {
    CheckedType type = tc_checked_type_invalid();
    type.kind = CHECKED_TYPE_VALUE;
    type.primitive = primitive;
    type.array_depth = array_depth;
    return type;
}

CheckedType tc_checked_type_named(const char *name, size_t generic_arg_count,
                                  size_t array_depth) {
    CheckedType type = tc_checked_type_invalid();
    type.kind = CHECKED_TYPE_NAMED;
    type.name = name;
    type.generic_arg_count = generic_arg_count;
    type.array_depth = array_depth;
    return type;
}

CheckedType tc_checked_type_type_param(const char *name) {
    CheckedType type = tc_checked_type_invalid();
    type.kind = CHECKED_TYPE_TYPE_PARAM;
    type.name = name;
    return type;
}

CheckedType tc_checked_type_function(size_t param_count) {
    CheckedType type = tc_checked_type_invalid();
    type.kind = CHECKED_TYPE_FUNCTION;
    type.generic_arg_count = param_count;
    return type;
}

bool tc_checked_type_is_hetero_array(CheckedType type) {
    return type.kind == CHECKED_TYPE_NAMED &&
           type.name != NULL &&
           strcmp(type.name, "arr") == 0 &&
           type.generic_arg_count >= 1 &&
           type.array_depth == 0;
}

bool tc_checked_type_is_num(CheckedType type) {
    return type.kind == CHECKED_TYPE_NAMED &&
           type.name != NULL &&
           strcmp(type.name, "num") == 0 &&
           type.array_depth == 0;
}

AstPrimitiveType tc_primitive_canonical(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_BYTE:   return AST_PRIMITIVE_UINT8;
    case AST_PRIMITIVE_SBYTE:  return AST_PRIMITIVE_INT8;
    case AST_PRIMITIVE_SHORT:  return AST_PRIMITIVE_INT16;
    case AST_PRIMITIVE_INT:    return AST_PRIMITIVE_INT32;
    case AST_PRIMITIVE_UINT:   return AST_PRIMITIVE_UINT32;
    case AST_PRIMITIVE_LONG:   return AST_PRIMITIVE_INT64;
    case AST_PRIMITIVE_ULONG:  return AST_PRIMITIVE_UINT64;
    case AST_PRIMITIVE_FLOAT:  return AST_PRIMITIVE_FLOAT32;
    case AST_PRIMITIVE_DOUBLE: return AST_PRIMITIVE_FLOAT64;
    default:                   return primitive;
    }
}

bool tc_checked_type_equals(CheckedType left, CheckedType right) {
    if (left.kind != right.kind) {
        return false;
    }

    if (left.kind == CHECKED_TYPE_NAMED || left.kind == CHECKED_TYPE_TYPE_PARAM) {
        if (!left.name || !right.name) {
            return left.name == right.name;
        }
        return strcmp(left.name, right.name) == 0 &&
               left.generic_arg_count == right.generic_arg_count &&
               tc_checked_type_array_extents_equal(left, right);
    }

    if (left.kind == CHECKED_TYPE_FUNCTION) {
        return left.generic_arg_count == right.generic_arg_count;
    }

    return tc_primitive_canonical(left.primitive) == tc_primitive_canonical(right.primitive) &&
           tc_checked_type_array_extents_equal(left, right);
}

bool tc_checked_type_is_scalar_value(CheckedType type) {
    return type.kind == CHECKED_TYPE_VALUE && type.array_depth == 0;
}

bool tc_checked_type_is_bool(CheckedType type) {
    return tc_checked_type_is_scalar_value(type) && type.primitive == AST_PRIMITIVE_BOOL;
}

bool tc_checked_type_is_string(CheckedType type) {
    return tc_checked_type_is_scalar_value(type) && type.primitive == AST_PRIMITIVE_STRING;
}

bool tc_checked_type_has_length_member(CheckedType type) {
    return tc_checked_type_is_string(type) ||
           (type.kind == CHECKED_TYPE_VALUE && type.array_depth > 0) ||
           tc_checked_type_is_hetero_array(type);
}

bool tc_checked_type_is_numeric(CheckedType type) {
    return tc_checked_type_is_num(type) ||
           (tc_checked_type_is_scalar_value(type) && tc_primitive_is_integral(type.primitive));
}

bool tc_checked_type_is_integral(CheckedType type) {
    return tc_checked_type_is_scalar_value(type) &&
           tc_primitive_is_integral(type.primitive) &&
           !tc_primitive_is_float(type.primitive);
}

bool tc_checked_type_is_reference_like(CheckedType type) {
    return (type.kind == CHECKED_TYPE_VALUE &&
            (type.array_depth > 0 || type.primitive == AST_PRIMITIVE_STRING)) ||
           type.kind == CHECKED_TYPE_EXTERNAL;
}

bool tc_primitive_is_float(AstPrimitiveType primitive) {
    return primitive == AST_PRIMITIVE_FLOAT32 || primitive == AST_PRIMITIVE_FLOAT64;
}

bool tc_primitive_is_integral(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_INT64:
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_UINT16:
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_UINT64:
    case AST_PRIMITIVE_FLOAT32:
    case AST_PRIMITIVE_FLOAT64:
    case AST_PRIMITIVE_CHAR:
    case AST_PRIMITIVE_BYTE:
    case AST_PRIMITIVE_SBYTE:
    case AST_PRIMITIVE_SHORT:
    case AST_PRIMITIVE_INT:
    case AST_PRIMITIVE_LONG:
    case AST_PRIMITIVE_ULONG:
    case AST_PRIMITIVE_UINT:
    case AST_PRIMITIVE_FLOAT:
    case AST_PRIMITIVE_DOUBLE:
        return true;
    case AST_PRIMITIVE_BOOL:
    case AST_PRIMITIVE_STRING:
        return false;
    }

    return false;
}

int tc_primitive_width(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_CHAR:
    case AST_PRIMITIVE_BYTE:
    case AST_PRIMITIVE_SBYTE:
        return 8;
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_UINT16:
    case AST_PRIMITIVE_SHORT:
        return 16;
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_FLOAT32:
    case AST_PRIMITIVE_INT:
    case AST_PRIMITIVE_UINT:
    case AST_PRIMITIVE_FLOAT:
        return 32;
    case AST_PRIMITIVE_INT64:
    case AST_PRIMITIVE_UINT64:
    case AST_PRIMITIVE_FLOAT64:
    case AST_PRIMITIVE_LONG:
    case AST_PRIMITIVE_ULONG:
    case AST_PRIMITIVE_DOUBLE:
        return 64;
    case AST_PRIMITIVE_BOOL:
    case AST_PRIMITIVE_STRING:
        return 0;
    }

    return 0;
}

bool tc_primitive_is_signed(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_INT64:
    case AST_PRIMITIVE_FLOAT32:
    case AST_PRIMITIVE_FLOAT64:
    case AST_PRIMITIVE_CHAR:
    case AST_PRIMITIVE_SBYTE:
    case AST_PRIMITIVE_SHORT:
    case AST_PRIMITIVE_INT:
    case AST_PRIMITIVE_LONG:
    case AST_PRIMITIVE_FLOAT:
    case AST_PRIMITIVE_DOUBLE:
        return true;
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_UINT16:
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_UINT64:
    case AST_PRIMITIVE_BOOL:
    case AST_PRIMITIVE_STRING:
    case AST_PRIMITIVE_BYTE:
    case AST_PRIMITIVE_ULONG:
    case AST_PRIMITIVE_UINT:
        return false;
    }

    return false;
}
#include "type_checker_types_p2.inc"
