#include "codegen.h"

#include <stdio.h>

void codegen_dump_write_indent(FILE *out, int indent) {
    int i;

    for (i = 0; i < indent; i++) {
        fputc(' ', out);
    }
}

bool codegen_dump_write_checked_type(FILE *out, CheckedType type) {
    char buffer[64];

    if (!checked_type_to_string(type, buffer, sizeof(buffer))) {
        return false;
    }

    fputs(buffer, out);
    return true;
}

const char *codegen_dump_unit_kind_name(LirUnitKind kind) {
    switch (kind) {
    case LIR_UNIT_START:
        return "start";
    case LIR_UNIT_BINDING:
        return "binding";
    case LIR_UNIT_INIT:
        return "init";
    case LIR_UNIT_LAMBDA:
        return "lambda";
    case LIR_UNIT_ASM:
        return "asm";
    }

    return "unknown";
}

const char *codegen_dump_slot_kind_name(LirSlotKind kind) {
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

const char *codegen_dump_instruction_kind_name(LirInstructionKind kind) {
    switch (kind) {
    case LIR_INSTR_INCOMING_ARG:
        return "incoming-arg";
    case LIR_INSTR_INCOMING_CAPTURE:
        return "incoming-capture";
    case LIR_INSTR_OUTGOING_ARG:
        return "outgoing-arg";
    case LIR_INSTR_BINARY:
        return "binary";
    case LIR_INSTR_UNARY:
        return "unary";
    case LIR_INSTR_CLOSURE:
        return "closure";
    case LIR_INSTR_CALL:
        return "call";
    case LIR_INSTR_CAST:
        return "cast";
    case LIR_INSTR_MEMBER:
        return "member";
    case LIR_INSTR_INDEX_LOAD:
        return "index-load";
    case LIR_INSTR_ARRAY_LITERAL:
        return "array-literal";
    case LIR_INSTR_TEMPLATE:
        return "template";
    case LIR_INSTR_STORE_SLOT:
        return "store-slot";
    case LIR_INSTR_STORE_GLOBAL:
        return "store-global";
    case LIR_INSTR_STORE_INDEX:
        return "store-index";
    case LIR_INSTR_STORE_MEMBER:
        return "store-member";
    case LIR_INSTR_HETERO_ARRAY_NEW:
        return "hetero-array-new";
    case LIR_INSTR_UNION_NEW:
        return "union-new";
    }

    return "unknown";
}

const char *codegen_dump_terminator_kind_name(LirTerminatorKind kind) {
    switch (kind) {
    case LIR_TERM_NONE:
        return "none";
    case LIR_TERM_RETURN:
        return "return";
    case LIR_TERM_JUMP:
        return "jump";
    case LIR_TERM_BRANCH:
        return "branch";
    case LIR_TERM_THROW:
        return "throw";
    }

    return "unknown";
}
