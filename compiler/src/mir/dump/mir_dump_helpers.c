#include "mir_dump_internal.h"

void mir_dump_write_indent(FILE *out, int indent) {
    int i;

    for (i = 0; i < indent; i++) {
        fputc(' ', out);
    }
}

bool mir_dump_write_checked_type(FILE *out, CheckedType type) {
    char buffer[64];

    if (!checked_type_to_string(type, buffer, sizeof(buffer))) {
        return false;
    }

    fputs(buffer, out);
    return true;
}

const char *mir_dump_binary_operator_name_text(AstBinaryOperator operator) {
    switch (operator) {
    case AST_BINARY_OP_LOGICAL_OR:
        return "||";
    case AST_BINARY_OP_LOGICAL_AND:
        return "&&";
    case AST_BINARY_OP_BIT_OR:
        return "|";
    case AST_BINARY_OP_BIT_NAND:
        return "!&";
    case AST_BINARY_OP_BIT_XOR:
        return "^";
    case AST_BINARY_OP_BIT_XNOR:
        return "!^";
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

const char *mir_dump_unary_operator_name_text(AstUnaryOperator operator) {
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

const char *mir_dump_local_kind_name(MirLocalKind kind) {
    switch (kind) {
    case MIR_LOCAL_PARAMETER:
        return "param";
    case MIR_LOCAL_LOCAL:
        return "local";
    case MIR_LOCAL_CAPTURE:
        return "capture";
    case MIR_LOCAL_SYNTHETIC:
        return "synthetic";
    }

    return "unknown";
}

bool mir_dump_template_part(FILE *out, const MirUnit *unit, const MirTemplatePart *part) {
    if (!out || !part) {
        return false;
    }

    if (part->kind == MIR_TEMPLATE_PART_TEXT) {
        return fprintf(out, "text(%s)", part->as.text) >= 0;
    }

    return fputs("value(", out) != EOF &&
           mir_dump_value(out, unit, part->as.value) &&
           fputc(')', out) != EOF;
}

bool mir_dump_value(FILE *out, const MirUnit *unit, MirValue value) {
    char type_buffer[64];

    switch (value.kind) {
    case MIR_VALUE_INVALID:
        return fputs("<void>", out) != EOF;
    case MIR_VALUE_TEMP:
        return fprintf(out, "temp(%zu)", value.as.temp_index) >= 0;
    case MIR_VALUE_LOCAL:
        return fprintf(out,
                       "local(%zu:%s)",
                       value.as.local_index,
                       unit->locals[value.as.local_index].name) >= 0;
    case MIR_VALUE_GLOBAL:
        return fprintf(out, "global(%s)", value.as.global_name) >= 0;
    case MIR_VALUE_LITERAL:
        if (value.as.literal.kind == AST_LITERAL_BOOL) {
            return fprintf(out, "bool(%s)", value.as.literal.bool_value ? "true" : "false") >= 0;
        }
        if (value.as.literal.kind == AST_LITERAL_NULL) {
            return fputs("null", out) != EOF;
        }
        if (!checked_type_to_string(value.type, type_buffer, sizeof(type_buffer))) {
            return false;
        }
        return fprintf(out, "%s(%s)", type_buffer, value.as.literal.text) >= 0;
    }

    return false;
}
