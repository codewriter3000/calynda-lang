#include "type_checker_internal.h"

bool tc_reserve_items(void **items, size_t *capacity,
                      size_t needed, size_t item_size) {
    size_t new_capacity;
    void *resized;

    if (needed <= *capacity) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 8 : *capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }

    resized = realloc(*items, new_capacity * item_size);
    if (!resized) {
        return false;
    }

    *items = resized;
    *capacity = new_capacity;
    return true;
}

const char *tc_primitive_type_name(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
        return "int8";
    case AST_PRIMITIVE_INT16:
        return "int16";
    case AST_PRIMITIVE_INT32:
        return "int32";
    case AST_PRIMITIVE_INT64:
        return "int64";
    case AST_PRIMITIVE_UINT8:
        return "uint8";
    case AST_PRIMITIVE_UINT16:
        return "uint16";
    case AST_PRIMITIVE_UINT32:
        return "uint32";
    case AST_PRIMITIVE_UINT64:
        return "uint64";
    case AST_PRIMITIVE_FLOAT32:
        return "float32";
    case AST_PRIMITIVE_FLOAT64:
        return "float64";
    case AST_PRIMITIVE_BOOL:
        return "bool";
    case AST_PRIMITIVE_CHAR:
        return "char";
    case AST_PRIMITIVE_STRING:
        return "string";
    case AST_PRIMITIVE_BYTE:
        return "byte";
    case AST_PRIMITIVE_SBYTE:
        return "sbyte";
    case AST_PRIMITIVE_SHORT:
        return "short";
    case AST_PRIMITIVE_INT:
        return "int";
    case AST_PRIMITIVE_LONG:
        return "long";
    case AST_PRIMITIVE_ULONG:
        return "ulong";
    case AST_PRIMITIVE_UINT:
        return "uint";
    case AST_PRIMITIVE_FLOAT:
        return "float";
    case AST_PRIMITIVE_DOUBLE:
        return "double";
    }

    return "unknown";
}

const char *tc_binary_operator_name(AstBinaryOperator operator) {
    switch (operator) {
    case AST_BINARY_OP_LOGICAL_OR:
        return "||";
    case AST_BINARY_OP_LOGICAL_AND:
        return "&&";
    case AST_BINARY_OP_BIT_OR:
        return "|";
    case AST_BINARY_OP_BIT_NAND:
        return "~&";
    case AST_BINARY_OP_BIT_XOR:
        return "^";
    case AST_BINARY_OP_BIT_XNOR:
        return "~^";
    case AST_BINARY_OP_BIT_AND:
        return "&";
    case AST_BINARY_OP_EQUAL:
        return "==";
    case AST_BINARY_OP_NOT_EQUAL:
        return "!=";
    case AST_BINARY_OP_LESS:
        return "<";
    case AST_BINARY_OP_GREATER:
        return ">";
    case AST_BINARY_OP_LESS_EQUAL:
        return "<=";
    case AST_BINARY_OP_GREATER_EQUAL:
        return ">=";
    case AST_BINARY_OP_SHIFT_LEFT:
        return "<<";
    case AST_BINARY_OP_SHIFT_RIGHT:
        return ">>";
    case AST_BINARY_OP_ADD:
        return "+";
    case AST_BINARY_OP_SUBTRACT:
        return "-";
    case AST_BINARY_OP_MULTIPLY:
        return "*";
    case AST_BINARY_OP_DIVIDE:
        return "/";
    case AST_BINARY_OP_MODULO:
        return "%";
    }

    return "?";
}

const char *tc_assignment_operator_name(AstAssignmentOperator operator) {
    switch (operator) {
    case AST_ASSIGN_OP_ASSIGN:
        return "=";
    case AST_ASSIGN_OP_ADD:
        return "+=";
    case AST_ASSIGN_OP_SUBTRACT:
        return "-=";
    case AST_ASSIGN_OP_MULTIPLY:
        return "*=";
    case AST_ASSIGN_OP_DIVIDE:
        return "/=";
    case AST_ASSIGN_OP_MODULO:
        return "%=";
    case AST_ASSIGN_OP_BIT_AND:
        return "&=";
    case AST_ASSIGN_OP_BIT_OR:
        return "|=";
    case AST_ASSIGN_OP_BIT_XOR:
        return "^=";
    case AST_ASSIGN_OP_SHIFT_LEFT:
        return "<<=";
    case AST_ASSIGN_OP_SHIFT_RIGHT:
        return ">>=";
    }

    return "?=";
}

const char *tc_block_context_name(BlockContextKind kind) {
    switch (kind) {
    case BLOCK_CONTEXT_LAMBDA:
        return "lambda body";
    case BLOCK_CONTEXT_START:
        return "start";
    }

    return "block";
}
