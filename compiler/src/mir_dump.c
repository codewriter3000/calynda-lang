#include "mir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_indent(FILE *out, int indent);
static bool write_checked_type(FILE *out, CheckedType type);
static const char *binary_operator_name_text(AstBinaryOperator operator);
static const char *unary_operator_name_text(AstUnaryOperator operator);
static const char *local_kind_name(MirLocalKind kind);
static bool dump_template_part(FILE *out, const MirUnit *unit, const MirTemplatePart *part);
static bool dump_value(FILE *out, const MirUnit *unit, MirValue value);

bool mir_dump_program(FILE *out, const MirProgram *program) {
    size_t i;
    size_t j;

    if (!out || !program) {
        return false;
    }

    fprintf(out, "MirProgram\n");
    for (i = 0; i < program->unit_count; i++) {
        const MirUnit *unit = &program->units[i];
        const char *unit_kind = "lambda";

        switch (unit->kind) {
        case MIR_UNIT_START:
            unit_kind = "start";
            break;
        case MIR_UNIT_BINDING:
            unit_kind = "binding";
            break;
        case MIR_UNIT_INIT:
            unit_kind = "init";
            break;
        case MIR_UNIT_LAMBDA:
            unit_kind = "lambda";
            break;
        }

        fprintf(out, "  Unit name=%s kind=%s return=",
                unit->name,
            unit_kind);
        if (!write_checked_type(out, unit->return_type)) {
            return false;
        }
        fprintf(out, " params=%zu locals=%zu blocks=%zu\n",
                unit->parameter_count,
                unit->local_count,
                unit->block_count);

        fprintf(out, "    Locals:\n");
        for (j = 0; j < unit->local_count; j++) {
            fprintf(out, "      Local index=%zu kind=%s name=%s type=",
                    unit->locals[j].index,
                    local_kind_name(unit->locals[j].kind),
                    unit->locals[j].name);
            if (!write_checked_type(out, unit->locals[j].type)) {
                return false;
            }
            fprintf(out, " final=%s\n", unit->locals[j].is_final ? "true" : "false");
        }

        fprintf(out, "    Blocks:\n");
        for (j = 0; j < unit->block_count; j++) {
            size_t k;
            const MirBasicBlock *block = &unit->blocks[j];

            fprintf(out, "      Block %s:\n", block->label);
            for (k = 0; k < block->instruction_count; k++) {
                const MirInstruction *instruction = &block->instructions[k];

                write_indent(out, 8);
                switch (instruction->kind) {
                case MIR_INSTR_BINARY:
                    fprintf(out, "t%zu = binary %s ",
                            instruction->as.binary.dest_temp,
                            binary_operator_name_text(instruction->as.binary.operator));
                    if (!dump_value(out, unit, instruction->as.binary.left) ||
                        fputs(" ", out) == EOF ||
                        !dump_value(out, unit, instruction->as.binary.right)) {
                        return false;
                    }
                    break;
                case MIR_INSTR_UNARY:
                    fprintf(out, "t%zu = unary %s ",
                            instruction->as.unary.dest_temp,
                            unary_operator_name_text(instruction->as.unary.operator));
                    if (!dump_value(out, unit, instruction->as.unary.operand)) {
                        return false;
                    }
                    break;
                case MIR_INSTR_CLOSURE:
                    fprintf(out, "t%zu = closure unit=%s(",
                            instruction->as.closure.dest_temp,
                            instruction->as.closure.unit_name);
                    for (size_t capture_index = 0;
                         capture_index < instruction->as.closure.capture_count;
                         capture_index++) {
                        if (capture_index > 0 && fputs(", ", out) == EOF) {
                            return false;
                        }
                        if (!dump_value(out,
                                        unit,
                                        instruction->as.closure.captures[capture_index])) {
                            return false;
                        }
                    }
                    if (fputc(')', out) == EOF) {
                        return false;
                    }
                    break;
                case MIR_INSTR_CALL:
                    if (instruction->as.call.has_result) {
                        fprintf(out, "t%zu = ", instruction->as.call.dest_temp);
                    }
                    fprintf(out, "call ");
                    if (!dump_value(out, unit, instruction->as.call.callee)) {
                        return false;
                    }
                    fputc('(', out);
                    for (size_t arg_index = 0; arg_index < instruction->as.call.argument_count; arg_index++) {
                        if (arg_index > 0) {
                            fputs(", ", out);
                        }
                        if (!dump_value(out, unit, instruction->as.call.arguments[arg_index])) {
                            return false;
                        }
                    }
                    fputc(')', out);
                    break;
                case MIR_INSTR_CAST:
                    fprintf(out, "t%zu = cast ", instruction->as.cast.dest_temp);
                    if (!dump_value(out, unit, instruction->as.cast.operand) ||
                        fputs(" to ", out) == EOF ||
                        !write_checked_type(out, instruction->as.cast.target_type)) {
                        return false;
                    }
                    break;
                case MIR_INSTR_MEMBER:
                    fprintf(out, "t%zu = member ", instruction->as.member.dest_temp);
                    if (!dump_value(out, unit, instruction->as.member.target) ||
                        fprintf(out, ".%s", instruction->as.member.member) < 0) {
                        return false;
                    }
                    break;
                case MIR_INSTR_INDEX_LOAD:
                    fprintf(out, "t%zu = index ", instruction->as.index_load.dest_temp);
                    if (!dump_value(out, unit, instruction->as.index_load.target) ||
                        fputc('[', out) == EOF ||
                        !dump_value(out, unit, instruction->as.index_load.index) ||
                        fputc(']', out) == EOF) {
                        return false;
                    }
                    break;
                case MIR_INSTR_ARRAY_LITERAL:
                    fprintf(out, "t%zu = array(", instruction->as.array_literal.dest_temp);
                    for (size_t element_index = 0;
                         element_index < instruction->as.array_literal.element_count;
                         element_index++) {
                        if (element_index > 0 && fputs(", ", out) == EOF) {
                            return false;
                        }
                        if (!dump_value(out,
                                        unit,
                                        instruction->as.array_literal.elements[element_index])) {
                            return false;
                        }
                    }
                    if (fputc(')', out) == EOF) {
                        return false;
                    }
                    break;
                case MIR_INSTR_TEMPLATE:
                    fprintf(out, "t%zu = template(", instruction->as.template_literal.dest_temp);
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
                case MIR_INSTR_STORE_LOCAL:
                    fprintf(out, "store local(%zu:%s) <- ",
                            instruction->as.store_local.local_index,
                            unit->locals[instruction->as.store_local.local_index].name);
                    if (!dump_value(out, unit, instruction->as.store_local.value)) {
                        return false;
                    }
                    break;
                case MIR_INSTR_STORE_GLOBAL:
                    fprintf(out, "store global(%s) <- ", instruction->as.store_global.global_name);
                    if (!dump_value(out, unit, instruction->as.store_global.value)) {
                        return false;
                    }
                    break;
                case MIR_INSTR_STORE_INDEX:
                    fprintf(out, "store index ");
                    if (!dump_value(out, unit, instruction->as.store_index.target) ||
                        fputc('[', out) == EOF ||
                        !dump_value(out, unit, instruction->as.store_index.index) ||
                        fputs("] <- ", out) == EOF ||
                        !dump_value(out, unit, instruction->as.store_index.value)) {
                        return false;
                    }
                    break;
                case MIR_INSTR_STORE_MEMBER:
                    fprintf(out, "store member ");
                    if (!dump_value(out, unit, instruction->as.store_member.target) ||
                        fprintf(out, ".%s <- ", instruction->as.store_member.member) < 0 ||
                        !dump_value(out, unit, instruction->as.store_member.value)) {
                        return false;
                    }
                    break;
                }
                fputc('\n', out);
            }

            write_indent(out, 8);
            switch (block->terminator.kind) {
            case MIR_TERM_RETURN:
                fprintf(out, "return");
                if (block->terminator.as.return_term.has_value) {
                    fputc(' ', out);
                    if (!dump_value(out, unit, block->terminator.as.return_term.value)) {
                        return false;
                    }
                }
                break;
            case MIR_TERM_GOTO:
                fprintf(out, "goto bb%zu", block->terminator.as.goto_term.target_block);
                break;
            case MIR_TERM_BRANCH:
                fprintf(out, "branch ");
                if (!dump_value(out, unit, block->terminator.as.branch_term.condition)) {
                    return false;
                }
                fprintf(out, " -> bb%zu, bb%zu",
                        block->terminator.as.branch_term.true_block,
                        block->terminator.as.branch_term.false_block);
                break;
            case MIR_TERM_THROW:
                fprintf(out, "throw ");
                if (!dump_value(out, unit, block->terminator.as.throw_term.value)) {
                    return false;
                }
                break;
            case MIR_TERM_NONE:
                fprintf(out, "<no terminator>");
                break;
            }
            fputc('\n', out);
        }
    }

    return !ferror(out);
}

char *mir_dump_program_to_string(const MirProgram *program) {
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

    if (!mir_dump_program(stream, program) || fflush(stream) != 0 ||
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

static const char *local_kind_name(MirLocalKind kind) {
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

static bool dump_template_part(FILE *out, const MirUnit *unit, const MirTemplatePart *part) {
    if (!out || !part) {
        return false;
    }

    if (part->kind == MIR_TEMPLATE_PART_TEXT) {
        return fprintf(out, "text(%s)", part->as.text) >= 0;
    }

    return fputs("value(", out) != EOF &&
           dump_value(out, unit, part->as.value) &&
           fputc(')', out) != EOF;
}

static bool dump_value(FILE *out, const MirUnit *unit, MirValue value) {
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