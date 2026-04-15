#include "type_checker_internal.h"

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

bool tc_checked_type_is_hetero_array(CheckedType type) {
    return type.kind == CHECKED_TYPE_NAMED &&
           type.name != NULL &&
           strcmp(type.name, "arr") == 0 &&
           type.generic_arg_count >= 1 &&
           type.array_depth == 0;
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
               left.array_depth == right.array_depth;
    }

    return left.primitive == right.primitive &&
           left.array_depth == right.array_depth;
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

bool tc_checked_type_is_numeric(CheckedType type) {
    return tc_checked_type_is_scalar_value(type) && tc_primitive_is_integral(type.primitive);
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

AstPrimitiveType tc_signed_primitive_for_width(int width) {
    if (width <= 8) {
        return AST_PRIMITIVE_INT8;
    }
    if (width <= 16) {
        return AST_PRIMITIVE_INT16;
    }
    if (width <= 32) {
        return AST_PRIMITIVE_INT32;
    }
    return AST_PRIMITIVE_INT64;
}

AstPrimitiveType tc_unsigned_primitive_for_width(int width) {
    if (width <= 8) {
        return AST_PRIMITIVE_UINT8;
    }
    if (width <= 16) {
        return AST_PRIMITIVE_UINT16;
    }
    if (width <= 32) {
        return AST_PRIMITIVE_UINT32;
    }
    return AST_PRIMITIVE_UINT64;
}
