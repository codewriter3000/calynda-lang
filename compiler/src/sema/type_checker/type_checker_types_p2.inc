
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
