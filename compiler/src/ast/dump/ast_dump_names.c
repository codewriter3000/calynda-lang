#include "ast_dump_internal.h"

const char *ast_dump_modifier_name(AstModifier modifier) {
    switch (modifier) {
    case AST_MODIFIER_PUBLIC:
        return "public";
    case AST_MODIFIER_PRIVATE:
        return "private";
    case AST_MODIFIER_FINAL:
        return "final";
    case AST_MODIFIER_EXPORT:
        return "export";
    case AST_MODIFIER_STATIC:
        return "static";
    case AST_MODIFIER_INTERNAL:
        return "internal";
    }

    return "unknown";
}

const char *ast_dump_primitive_type_name(AstPrimitiveType primitive) {
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

const char *ast_dump_assignment_operator_name(AstAssignmentOperator operator) {
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

const char *ast_dump_binary_operator_name(AstBinaryOperator operator) {
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

const char *ast_dump_unary_operator_name(AstUnaryOperator operator) {
    switch (operator) {
    case AST_UNARY_OP_LOGICAL_NOT:
        return "!";
    case AST_UNARY_OP_BITWISE_NOT:
        return "~";
    case AST_UNARY_OP_NEGATE:
        return "-";
    case AST_UNARY_OP_PLUS:
        return "+";
    case AST_UNARY_OP_PRE_INCREMENT:
        return "++";
    case AST_UNARY_OP_PRE_DECREMENT:
        return "--";
    case AST_UNARY_OP_DEREF:
        return "*";
    case AST_UNARY_OP_ADDRESS_OF:
        return "&";
    }

    return "?";
}
