#include "lir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_indent(FILE *out, int indent);
static bool write_checked_type(FILE *out, CheckedType type);
static const char *binary_operator_name_text(AstBinaryOperator operator);
static const char *unary_operator_name_text(AstUnaryOperator operator);
static const char *slot_kind_name(LirSlotKind kind);
static bool dump_template_part(FILE *out, const LirUnit *unit, const LirTemplatePart *part);
static bool dump_operand(FILE *out, const LirUnit *unit, LirOperand operand);

bool lir_dump_program(FILE *out, const LirProgram *program) {
    size_t i;
    size_t j;

    if (!out || !program) {
        return false;
    }

    fprintf(out, "LirProgram\n");
    for (i = 0; i < program->unit_count; i++) {
        const LirUnit *unit = &program->units[i];
        const char *unit_kind = "lambda";

        switch (unit->kind) {
        case LIR_UNIT_START:
            unit_kind = "start";
            break;
        case LIR_UNIT_BINDING:
            unit_kind = "binding";
            break;
        case LIR_UNIT_INIT:
            unit_kind = "init";
            break;
        case LIR_UNIT_LAMBDA:
            unit_kind = "lambda";
            break;
        }

        fprintf(out, "  Unit name=%s kind=%s return=", unit->name, unit_kind);
        if (!write_checked_type(out, unit->return_type)) {
            return false;
        }
        fprintf(out,
                " captures=%zu params=%zu slots=%zu vregs=%zu blocks=%zu\n",
                unit->capture_count,
                unit->parameter_count,
                unit->slot_count,
                unit->virtual_register_count,
                unit->block_count);

        fprintf(out, "    Slots:\n");
        for (j = 0; j < unit->slot_count; j++) {
            fprintf(out, "      Slot index=%zu kind=%s name=%s type=",
                    unit->slots[j].index,
                    slot_kind_name(unit->slots[j].kind),
                    unit->slots[j].name);
            if (!write_checked_type(out, unit->slots[j].type)) {
                return false;
            }
            fprintf(out, " final=%s\n", unit->slots[j].is_final ? "true" : "false");
        }

        fprintf(out, "    Blocks:\n");
        for (j = 0; j < unit->block_count; j++) {
            size_t k;
            const LirBasicBlock *block = &unit->blocks[j];

            fprintf(out, "      Block %s:\n", block->label);
            for (k = 0; k < block->instruction_count; k++) {
                const LirInstruction *instruction = &block->instructions[k];

                write_indent(out, 8);
                switch (instruction->kind) {
                case LIR_INSTR_INCOMING_ARG:
                    fprintf(out,
                            "incoming arg%zu -> slot(%zu:%s)",
                            instruction->as.incoming_arg.argument_index,
                            instruction->as.incoming_arg.slot_index,
                            unit->slots[instruction->as.incoming_arg.slot_index].name);
                    break;
                case LIR_INSTR_INCOMING_CAPTURE:
                    fprintf(out,
                            "incoming capture%zu -> slot(%zu:%s)",
                            instruction->as.incoming_capture.capture_index,
                            instruction->as.incoming_capture.slot_index,
                            unit->slots[instruction->as.incoming_capture.slot_index].name);
                    break;
                case LIR_INSTR_OUTGOING_ARG:
                    fprintf(out,
                            "out arg%zu <- ",
                            instruction->as.outgoing_arg.argument_index);
                    if (!dump_operand(out, unit, instruction->as.outgoing_arg.value)) {
                        return false;
                    }
                    break;
                case LIR_INSTR_BINARY:
                    fprintf(out,
                            "v%zu = binary %s ",
                            instruction->as.binary.dest_vreg,
                            binary_operator_name_text(instruction->as.binary.operator));
                    if (!dump_operand(out, unit, instruction->as.binary.left) ||
                        fputs(" ", out) == EOF ||
                        !dump_operand(out, unit, instruction->as.binary.right)) {
                        return false;
                    }
                    break;
                case LIR_INSTR_UNARY:
                    fprintf(out,
                            "v%zu = unary %s ",
                            instruction->as.unary.dest_vreg,
                            unary_operator_name_text(instruction->as.unary.operator));
                    if (!dump_operand(out, unit, instruction->as.unary.operand)) {
                        return false;
                    }
                    break;
                case LIR_INSTR_CLOSURE:
                    fprintf(out,
                            "v%zu = closure unit=%s(",
                            instruction->as.closure.dest_vreg,
                            instruction->as.closure.unit_name);
                    for (size_t capture_index = 0;
                         capture_index < instruction->as.closure.capture_count;
                         capture_index++) {
                        if (capture_index > 0 && fputs(", ", out) == EOF) {
                            return false;
                        }
                        if (!dump_operand(out,
                                          unit,
                                          instruction->as.closure.captures[capture_index])) {
                            return false;
                        }
                    }
                    if (fputc(')', out) == EOF) {
                        return false;
                    }
                    break;
                case LIR_INSTR_CALL:
                    if (instruction->as.call.has_result) {
                        fprintf(out, "v%zu = ", instruction->as.call.dest_vreg);
                    }
                    fprintf(out, "call ");
                    if (!dump_operand(out, unit, instruction->as.call.callee) ||
                        fprintf(out, " argc=%zu", instruction->as.call.argument_count) < 0) {
                        return false;
                    }
                    break;
                case LIR_INSTR_CAST:
                    fprintf(out, "v%zu = cast ", instruction->as.cast.dest_vreg);
                    if (!dump_operand(out, unit, instruction->as.cast.operand) ||
                        fputs(" to ", out) == EOF ||
                        !write_checked_type(out, instruction->as.cast.target_type)) {
                        return false;
                    }
                    break;
                case LIR_INSTR_MEMBER:
                    fprintf(out, "v%zu = member ", instruction->as.member.dest_vreg);
                    if (!dump_operand(out, unit, instruction->as.member.target) ||
                        fprintf(out, ".%s", instruction->as.member.member) < 0) {
                        return false;
                    }
                    break;
                case LIR_INSTR_INDEX_LOAD:
                    fprintf(out, "v%zu = index ", instruction->as.index_load.dest_vreg);
                    if (!dump_operand(out, unit, instruction->as.index_load.target) ||
                        fputc('[', out) == EOF ||
                        !dump_operand(out, unit, instruction->as.index_load.index) ||
                        fputc(']', out) == EOF) {
                        return false;
                    }
                    break;
                case LIR_INSTR_ARRAY_LITERAL:
                    fprintf(out, "v%zu = array(", instruction->as.array_literal.dest_vreg);
                    for (size_t element_index = 0;
                         element_index < instruction->as.array_literal.element_count;
                         element_index++) {
                        if (element_index > 0 && fputs(", ", out) == EOF) {
                            return false;
                        }
                        if (!dump_operand(out,
                                          unit,
                                          instruction->as.array_literal.elements[element_index])) {
                            return false;
                        }
                    }
                    if (fputc(')', out) == EOF) {
                        return false;
                    }
                    break;
                case LIR_INSTR_TEMPLATE:
                    fprintf(out, "v%zu = template(", instruction->as.template_literal.dest_vreg);
                    for (size_t part_index = 0;
                         part_index < instruction->as.template_literal.part_count;
                         part_index++) {
                        if (part_index > 0 && fputs(", ", out) == EOF) {
                            return false;
                        }
                        if (!dump_template_part(out,
                                                unit,
                                                &instruction->as.template_literal.parts[part_index])) {
                            return false;
                        }
                    }
                    if (fputc(')', out) == EOF) {
                        return false;
                    }
                    break;
                case LIR_INSTR_STORE_SLOT:
                    fprintf(out,
                            "store slot(%zu:%s) <- ",
                            instruction->as.store_slot.slot_index,
                            unit->slots[instruction->as.store_slot.slot_index].name);
                    if (!dump_operand(out, unit, instruction->as.store_slot.value)) {
                        return false;
                    }
                    break;
                case LIR_INSTR_STORE_GLOBAL:
                    fprintf(out,
                            "store global(%s) <- ",
                            instruction->as.store_global.global_name);
                    if (!dump_operand(out, unit, instruction->as.store_global.value)) {
                        return false;
                    }
                    break;
                case LIR_INSTR_STORE_INDEX:
                    fprintf(out, "store index ");
                    if (!dump_operand(out, unit, instruction->as.store_index.target) ||
                        fputc('[', out) == EOF ||
                        !dump_operand(out, unit, instruction->as.store_index.index) ||
                        fputs("] <- ", out) == EOF ||
                        !dump_operand(out, unit, instruction->as.store_index.value)) {
                        return false;
                    }
                    break;
                case LIR_INSTR_STORE_MEMBER:
                    fprintf(out, "store member ");
                    if (!dump_operand(out, unit, instruction->as.store_member.target) ||
                        fprintf(out, ".%s <- ", instruction->as.store_member.member) < 0 ||
                        !dump_operand(out, unit, instruction->as.store_member.value)) {
                        return false;
                    }
                    break;
                }
                fputc('\n', out);
            }

            write_indent(out, 8);
            switch (block->terminator.kind) {
            case LIR_TERM_RETURN:
                fprintf(out, "return");
                if (block->terminator.as.return_term.has_value) {
                    fputc(' ', out);
                    if (!dump_operand(out, unit, block->terminator.as.return_term.value)) {
                        return false;
                    }
                }
                break;
            case LIR_TERM_JUMP:
                fprintf(out, "jump bb%zu", block->terminator.as.jump_term.target_block);
                break;
            case LIR_TERM_BRANCH:
                fprintf(out, "branch ");
                if (!dump_operand(out, unit, block->terminator.as.branch_term.condition)) {
                    return false;
                }
                fprintf(out,
                        " -> bb%zu, bb%zu",
                        block->terminator.as.branch_term.true_block,
                        block->terminator.as.branch_term.false_block);
                break;
            case LIR_TERM_THROW:
                fprintf(out, "throw ");
                if (!dump_operand(out, unit, block->terminator.as.throw_term.value)) {
                    return false;
                }
                break;
            case LIR_TERM_NONE:
                fprintf(out, "<no terminator>");
                break;
            }
            fputc('\n', out);
        }
    }

    return !ferror(out);
}

char *lir_dump_program_to_string(const LirProgram *program) {
    FILE *stream;
    char *buffer;
    long size;
    size_t read_size;

    if (!program) {
        return NULL;
    }

    stream = tmpfile();
    if (!stream) {
        return NULL;
    }

    if (!lir_dump_program(stream, program) || fflush(stream) != 0 ||
        fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return NULL;
    }

    size = ftell(stream);
    if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(stream);
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, stream);
    fclose(stream);
    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

static void write_indent(FILE *out, int indent) {
    int i;

    for (i = 0; i < indent; i++) {
        fputc(' ', out);
    }
}

static bool write_checked_type(FILE *out, CheckedType type) {
    char buffer[64];

    if (!checked_type_to_string(type, buffer, sizeof(buffer))) {
        return false;
    }

    fputs(buffer, out);
    return true;
}

static const char *binary_operator_name_text(AstBinaryOperator operator) {
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

static const char *unary_operator_name_text(AstUnaryOperator operator) {
    switch (operator) {
    case AST_UNARY_OP_LOGICAL_NOT:
        return "!";
    case AST_UNARY_OP_BITWISE_NOT:
        return "~";
    case AST_UNARY_OP_NEGATE:
        return "-";
    case AST_UNARY_OP_PLUS:
        return "+";
    }

    return "?";
}

static const char *slot_kind_name(LirSlotKind kind) {
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

static bool dump_template_part(FILE *out, const LirUnit *unit, const LirTemplatePart *part) {
    if (!out || !part) {
        return false;
    }

    if (part->kind == LIR_TEMPLATE_PART_TEXT) {
        return fprintf(out, "text(%s)", part->as.text) >= 0;
    }

    return fputs("value(", out) != EOF &&
           dump_operand(out, unit, part->as.value) &&
           fputc(')', out) != EOF;
}

static bool dump_operand(FILE *out, const LirUnit *unit, LirOperand operand) {
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