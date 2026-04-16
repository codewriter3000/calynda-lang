#include "bytecode_dump_internal.h"

const char *bytecode_dump_local_kind_name(BytecodeLocalKind kind) {
    switch (kind) {
    case BYTECODE_LOCAL_PARAMETER:
        return "parameter";
    case BYTECODE_LOCAL_LOCAL:
        return "local";
    case BYTECODE_LOCAL_CAPTURE:
        return "capture";
    case BYTECODE_LOCAL_SYNTHETIC:
        return "synthetic";
    }

    return "unknown";
}

const char *bytecode_dump_unit_kind_name(BytecodeUnitKind kind) {
    switch (kind) {
    case BYTECODE_UNIT_START:
        return "start";
    case BYTECODE_UNIT_BINDING:
        return "binding";
    case BYTECODE_UNIT_INIT:
        return "init";
    case BYTECODE_UNIT_LAMBDA:
        return "lambda";
    }

    return "unknown";
}

const char *bytecode_dump_binary_operator_name_text(AstBinaryOperator operator) {
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

const char *bytecode_dump_unary_operator_name_text(AstUnaryOperator operator) {
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

const char *bytecode_dump_literal_kind_name(AstLiteralKind kind) {
    switch (kind) {
    case AST_LITERAL_INTEGER:
        return "integer";
    case AST_LITERAL_FLOAT:
        return "float";
    case AST_LITERAL_BOOL:
        return "bool";
    case AST_LITERAL_CHAR:
        return "char";
    case AST_LITERAL_STRING:
        return "string";
    case AST_LITERAL_TEMPLATE:
        return "template";
    case AST_LITERAL_NULL:
        return "null";
    }

    return "unknown";
}

const char *bytecode_dump_type_tag_name(CalyndaRtTypeTag tag) {
    switch (tag) {
    case CALYNDA_RT_TYPE_VOID:
        return "void";
    case CALYNDA_RT_TYPE_BOOL:
        return "bool";
    case CALYNDA_RT_TYPE_INT32:
        return "int32";
    case CALYNDA_RT_TYPE_INT64:
        return "int64";
    case CALYNDA_RT_TYPE_STRING:
        return "string";
    case CALYNDA_RT_TYPE_ARRAY:
        return "array";
    case CALYNDA_RT_TYPE_CLOSURE:
        return "closure";
    case CALYNDA_RT_TYPE_EXTERNAL:
        return "external";
    case CALYNDA_RT_TYPE_RAW_WORD:
        return "raw_word";
    case CALYNDA_RT_TYPE_UNION:
        return "union";
    case CALYNDA_RT_TYPE_HETERO_ARRAY:
        return "hetero_array";
    }

    return "unknown";
}
