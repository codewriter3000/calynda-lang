#include "lir_dump_internal.h"

void lir_dump_write_indent(FILE *out, int indent) {
    int i;

    for (i = 0; i < indent; i++) {
        fputc(' ', out);
    }
}

bool lir_dump_write_checked_type(FILE *out, CheckedType type) {
    char buffer[64];

    if (!checked_type_to_string(type, buffer, sizeof(buffer))) {
        return false;
    }

    fputs(buffer, out);
    return true;
}

const char *lir_dump_binary_operator_name_text(AstBinaryOperator operator) {
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

const char *lir_dump_unary_operator_name_text(AstUnaryOperator operator) {
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

const char *lir_dump_slot_kind_name(LirSlotKind kind) {
    switch (kind) {
    case LIR_SLOT_PARAMETER:
        return "param";
    case LIR_SLOT_LOCAL:
        return "local";
    case LIR_SLOT_CAPTURE:
        return "capture";
    case LIR_SLOT_SYNTHETIC:
        return "synthetic";
    }

    return "unknown";
}

const char *lir_dump_type_tag_name(CalyndaRtTypeTag tag) {
    return tag == CALYNDA_RT_TYPE_VOID ? "void" : tag == CALYNDA_RT_TYPE_BOOL ? "bool" :
        tag == CALYNDA_RT_TYPE_INT32 ? "int32" : tag == CALYNDA_RT_TYPE_INT64 ? "int64" :
        tag == CALYNDA_RT_TYPE_STRING ? "string" : tag == CALYNDA_RT_TYPE_ARRAY ? "array" :
        tag == CALYNDA_RT_TYPE_CLOSURE ? "closure" : tag == CALYNDA_RT_TYPE_EXTERNAL ? "external" :
        tag == CALYNDA_RT_TYPE_UNION ? "union" :
        tag == CALYNDA_RT_TYPE_HETERO_ARRAY ? "hetero_array" : "raw_word";
}

bool lir_dump_type_descriptor(FILE *out, const CalyndaRtTypeDescriptor *type_desc) {
    size_t index;

    if (fprintf(out, "typedesc(%s|%zu",
                type_desc->name ? type_desc->name : "?",
                type_desc->generic_param_count) < 0) {
        return false;
    }
    for (index = 0; index < type_desc->generic_param_count; index++) {
        CalyndaRtTypeTag tag = type_desc->generic_param_tags
            ? type_desc->generic_param_tags[index] : CALYNDA_RT_TYPE_RAW_WORD;

        if (fprintf(out, "|g%zu:%s", index, lir_dump_type_tag_name(tag)) < 0) {
            return false;
        }
    }
    for (index = 0; index < type_desc->variant_count; index++) {
        const char *variant_name =
            type_desc->variant_names ? type_desc->variant_names[index] : NULL;
        const char *tag_name = lir_dump_type_tag_name(type_desc->variant_payload_tags[index]);

        if ((variant_name ? fprintf(out, "|%s:%s", variant_name, tag_name)
                          : fprintf(out, "|%zu:%s", index, tag_name)) < 0) {
            return false;
        }
    }

    return fputc(')', out) != EOF;
}

bool lir_dump_template_part(FILE *out, const LirUnit *unit, const LirTemplatePart *part) {
    if (!out || !part) {
        return false;
    }

    if (part->kind == LIR_TEMPLATE_PART_TEXT) {
        return fprintf(out, "text(%s)", part->as.text) >= 0;
    }

    return fputs("value(", out) != EOF &&
           lir_dump_operand(out, unit, part->as.value) &&
           fputc(')', out) != EOF;
}

bool lir_dump_operand(FILE *out, const LirUnit *unit, LirOperand operand) {
    char type_buffer[64];

    switch (operand.kind) {
    case LIR_OPERAND_INVALID:
        return fputs("<void>", out) != EOF;
    case LIR_OPERAND_VREG:
        return fprintf(out, "vreg(%zu)", operand.as.vreg_index) >= 0;
    case LIR_OPERAND_SLOT:
        return fprintf(out,
                       "slot(%zu:%s)",
                       operand.as.slot_index,
                       unit->slots[operand.as.slot_index].name) >= 0;
    case LIR_OPERAND_GLOBAL:
        return fprintf(out, "global(%s)", operand.as.global_name) >= 0;
    case LIR_OPERAND_LITERAL:
        if (operand.as.literal.kind == AST_LITERAL_BOOL) {
            return fprintf(out,
                           "bool(%s)",
                           operand.as.literal.bool_value ? "true" : "false") >= 0;
        }
        if (operand.as.literal.kind == AST_LITERAL_NULL) {
            return fputs("null", out) != EOF;
        }
        if (!checked_type_to_string(operand.type, type_buffer, sizeof(type_buffer))) {
            return false;
        }
        return fprintf(out, "%s(%s)", type_buffer, operand.as.literal.text) >= 0;
    }

    return false;
}
