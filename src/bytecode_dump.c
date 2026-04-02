#include "bytecode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool write_checked_type(FILE *out, CheckedType type);
static const char *local_kind_name(BytecodeLocalKind kind);
static const char *unit_kind_name(BytecodeUnitKind kind);
static const char *binary_operator_name_text(AstBinaryOperator operator);
static const char *unary_operator_name_text(AstUnaryOperator operator);
static const char *literal_kind_name(AstLiteralKind kind);
static bool write_quoted_text(FILE *out, const char *text);
static bool dump_constant_ref(FILE *out, const BytecodeProgram *program, size_t constant_index);
static bool dump_value(FILE *out,
                       const BytecodeProgram *program,
                       const BytecodeUnit *unit,
                       BytecodeValue value);
static bool dump_template_part(FILE *out,
                               const BytecodeProgram *program,
                               const BytecodeUnit *unit,
                               const BytecodeTemplatePart *part);

bool bytecode_dump_program(FILE *out, const BytecodeProgram *program) {
    size_t i;
    size_t j;

    if (!out || !program) {
        return false;
    }

    fprintf(out,
            "BytecodeProgram target=%s constants=%zu units=%zu\n",
            bytecode_target_name(program->target),
            program->constant_count,
            program->unit_count);
    fprintf(out, "  Constants:\n");
    for (i = 0; i < program->constant_count; i++) {
        fprintf(out, "    c%zu ", i);
        switch (program->constants[i].kind) {
        case BYTECODE_CONSTANT_LITERAL:
            fprintf(out,
                    "literal kind=%s",
                    literal_kind_name(program->constants[i].as.literal.kind));
            if (program->constants[i].as.literal.kind == AST_LITERAL_BOOL) {
                fprintf(out,
                        " value=%s",
                        program->constants[i].as.literal.bool_value ? "true" : "false");
            } else if (program->constants[i].as.literal.text) {
                fprintf(out, " text=");
                if (!write_quoted_text(out, program->constants[i].as.literal.text)) {
                    return false;
                }
            }
            break;
        case BYTECODE_CONSTANT_SYMBOL:
            fprintf(out, "symbol ");
            if (!write_quoted_text(out, program->constants[i].as.text)) {
                return false;
            }
            break;
        case BYTECODE_CONSTANT_TEXT:
            fprintf(out, "text ");
            if (!write_quoted_text(out, program->constants[i].as.text)) {
                return false;
            }
            break;
        }
        fputc('\n', out);
    }

    for (i = 0; i < program->unit_count; i++) {
        const BytecodeUnit *unit = &program->units[i];

        fprintf(out,
                "  Unit name=%s kind=%s return=",
                unit->name,
                unit_kind_name(unit->kind));
        if (!write_checked_type(out, unit->return_type)) {
            return false;
        }
        fprintf(out,
                " params=%zu locals=%zu temps=%zu blocks=%zu\n",
                unit->parameter_count,
                unit->local_count,
                unit->temp_count,
                unit->block_count);

        fprintf(out, "    Locals:\n");
        for (j = 0; j < unit->local_count; j++) {
            fprintf(out,
                    "      Local index=%zu kind=%s name=%s type=",
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
            const BytecodeBasicBlock *block = &unit->blocks[j];
            size_t instruction_index;

            fprintf(out, "      Block %s:\n", block->label);
            for (instruction_index = 0;
                 instruction_index < block->instruction_count;
                 instruction_index++) {
                const BytecodeInstruction *instruction = &block->instructions[instruction_index];
                size_t item_index;

                fputs("        ", out);
                switch (instruction->kind) {
                case BYTECODE_INSTR_BINARY:
                    fprintf(out,
                            "BC_BINARY t%zu <- ",
                            instruction->as.binary.dest_temp);
                    if (!dump_value(out, program, unit, instruction->as.binary.left) ||
                        fprintf(out,
                                " %s ",
                                binary_operator_name_text(instruction->as.binary.operator)) < 0 ||
                        !dump_value(out, program, unit, instruction->as.binary.right)) {
                        return false;
                    }
                    break;
                case BYTECODE_INSTR_UNARY:
                    fprintf(out,
                            "BC_UNARY t%zu <- %s ",
                            instruction->as.unary.dest_temp,
                            unary_operator_name_text(instruction->as.unary.operator));
                    if (!dump_value(out, program, unit, instruction->as.unary.operand)) {
                        return false;
                    }
                    break;
                case BYTECODE_INSTR_CLOSURE:
                    fprintf(out,
                            "BC_CLOSURE t%zu <- unit#%zu(%s)",
                            instruction->as.closure.dest_temp,
                            instruction->as.closure.unit_index,
                            program->units[instruction->as.closure.unit_index].name);
                    fputc('(', out);
                    for (item_index = 0;
                         item_index < instruction->as.closure.capture_count;
                         item_index++) {
                        if (item_index > 0 && fputs(", ", out) == EOF) {
                            return false;
                        }
                        if (!dump_value(out,
                                        program,
                                        unit,
                                        instruction->as.closure.captures[item_index])) {
                            return false;
                        }
                    }
                    fputc(')', out);
                    break;
                case BYTECODE_INSTR_CALL:
                    if (instruction->as.call.has_result) {
                        fprintf(out, "BC_CALL t%zu <- ", instruction->as.call.dest_temp);
                    } else {
                        fprintf(out, "BC_CALL void <- ");
                    }
                    if (!dump_value(out, program, unit, instruction->as.call.callee)) {
                        return false;
                    }
                    fputc('(', out);
                    for (item_index = 0;
                         item_index < instruction->as.call.argument_count;
                         item_index++) {
                        if (item_index > 0 && fputs(", ", out) == EOF) {
                            return false;
                        }
                        if (!dump_value(out,
                                        program,
                                        unit,
                                        instruction->as.call.arguments[item_index])) {
                            return false;
                        }
                    }
                    fputc(')', out);
                    break;
                case BYTECODE_INSTR_CAST:
                    fprintf(out, "BC_CAST t%zu <- ", instruction->as.cast.dest_temp);
                    if (!dump_value(out, program, unit, instruction->as.cast.operand) ||
                        fputs(" to ", out) == EOF ||
                        !write_checked_type(out, instruction->as.cast.target_type)) {
                        return false;
                    }
                    break;
                case BYTECODE_INSTR_MEMBER:
                    fprintf(out, "BC_MEMBER t%zu <- ", instruction->as.member.dest_temp);
                    if (!dump_value(out, program, unit, instruction->as.member.target) ||
                        fputs(" . ", out) == EOF ||
                        !dump_constant_ref(out, program, instruction->as.member.member_index)) {
                        return false;
                    }
                    break;
                case BYTECODE_INSTR_INDEX_LOAD:
                    fprintf(out, "BC_INDEX_LOAD t%zu <- ", instruction->as.index_load.dest_temp);
                    if (!dump_value(out, program, unit, instruction->as.index_load.target) ||
                        fputc('[', out) == EOF ||
                        !dump_value(out, program, unit, instruction->as.index_load.index) ||
                        fputc(']', out) == EOF) {
                        return false;
                    }
                    break;
                case BYTECODE_INSTR_ARRAY_LITERAL:
                    fprintf(out,
                            "BC_ARRAY_LITERAL t%zu <- [",
                            instruction->as.array_literal.dest_temp);
                    for (item_index = 0;
                         item_index < instruction->as.array_literal.element_count;
                         item_index++) {
                        if (item_index > 0 && fputs(", ", out) == EOF) {
                            return false;
                        }
                        if (!dump_value(out,
                                        program,
                                        unit,
                                        instruction->as.array_literal.elements[item_index])) {
                            return false;
                        }
                    }
                    fputc(']', out);
                    break;
                case BYTECODE_INSTR_TEMPLATE:
                    fprintf(out,
                            "BC_TEMPLATE t%zu <- [",
                            instruction->as.template_literal.dest_temp);
                    for (item_index = 0;
                         item_index < instruction->as.template_literal.part_count;
                         item_index++) {
                        if (item_index > 0 && fputs(", ", out) == EOF) {
                            return false;
                        }
                        if (!dump_template_part(out,
                                                program,
                                                unit,
                                                &instruction->as.template_literal.parts[item_index])) {
                            return false;
                        }
                    }
                    fputc(']', out);
                    break;
                case BYTECODE_INSTR_STORE_LOCAL:
                    fprintf(out,
                            "BC_STORE_LOCAL local(%zu:%s) <- ",
                            instruction->as.store_local.local_index,
                            unit->locals[instruction->as.store_local.local_index].name);
                    if (!dump_value(out, program, unit, instruction->as.store_local.value)) {
                        return false;
                    }
                    break;
                case BYTECODE_INSTR_STORE_GLOBAL:
                    fprintf(out, "BC_STORE_GLOBAL ");
                    if (!dump_constant_ref(out, program, instruction->as.store_global.global_index) ||
                        fputs(" <- ", out) == EOF ||
                        !dump_value(out, program, unit, instruction->as.store_global.value)) {
                        return false;
                    }
                    break;
                case BYTECODE_INSTR_STORE_INDEX:
                    fprintf(out, "BC_STORE_INDEX ");
                    if (!dump_value(out, program, unit, instruction->as.store_index.target) ||
                        fputc('[', out) == EOF ||
                        !dump_value(out, program, unit, instruction->as.store_index.index) ||
                        fputs("] <- ", out) == EOF ||
                        !dump_value(out, program, unit, instruction->as.store_index.value)) {
                        return false;
                    }
                    break;
                case BYTECODE_INSTR_STORE_MEMBER:
                    fprintf(out, "BC_STORE_MEMBER ");
                    if (!dump_value(out, program, unit, instruction->as.store_member.target) ||
                        fputs(" . ", out) == EOF ||
                        !dump_constant_ref(out, program, instruction->as.store_member.member_index) ||
                        fputs(" <- ", out) == EOF ||
                        !dump_value(out, program, unit, instruction->as.store_member.value)) {
                        return false;
                    }
                    break;
                }
                fputc('\n', out);
            }

            fputs("        ", out);
            switch (block->terminator.kind) {
            case BYTECODE_TERM_NONE:
                fputs("BC_TERM_NONE", out);
                break;
            case BYTECODE_TERM_RETURN:
                fputs("BC_RETURN", out);
                if (block->terminator.as.return_term.has_value) {
                    fputc(' ', out);
                    if (!dump_value(out, program, unit, block->terminator.as.return_term.value)) {
                        return false;
                    }
                }
                break;
            case BYTECODE_TERM_JUMP:
                fprintf(out, "BC_JUMP bb%zu", block->terminator.as.jump_term.target_block);
                break;
            case BYTECODE_TERM_BRANCH:
                fputs("BC_BRANCH ", out);
                if (!dump_value(out, program, unit, block->terminator.as.branch_term.condition) ||
                    fprintf(out,
                            " -> bb%zu, bb%zu",
                            block->terminator.as.branch_term.true_block,
                            block->terminator.as.branch_term.false_block) < 0) {
                    return false;
                }
                break;
            case BYTECODE_TERM_THROW:
                fputs("BC_THROW ", out);
                if (!dump_value(out, program, unit, block->terminator.as.throw_term.value)) {
                    return false;
                }
                break;
            }
            fputc('\n', out);
        }
    }

    return !ferror(out);
}

char *bytecode_dump_program_to_string(const BytecodeProgram *program) {
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

    if (!bytecode_dump_program(stream, program) || fflush(stream) != 0 ||
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

static bool write_checked_type(FILE *out, CheckedType type) {
    char buffer[64];

    if (!checked_type_to_string(type, buffer, sizeof(buffer))) {
        return false;
    }

    return fputs(buffer, out) != EOF;
}

static const char *local_kind_name(BytecodeLocalKind kind) {
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

static const char *unit_kind_name(BytecodeUnitKind kind) {
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

static const char *binary_operator_name_text(AstBinaryOperator operator) {
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

static const char *literal_kind_name(AstLiteralKind kind) {
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

static bool write_quoted_text(FILE *out, const char *text) {
    const unsigned char *cursor = (const unsigned char *)(text ? text : "");

    if (fputc('"', out) == EOF) {
        return false;
    }
    while (*cursor) {
        switch (*cursor) {
        case '\\':
            if (fputs("\\\\", out) == EOF) {
                return false;
            }
            break;
        case '"':
            if (fputs("\\\"", out) == EOF) {
                return false;
            }
            break;
        case '\n':
            if (fputs("\\n", out) == EOF) {
                return false;
            }
            break;
        case '\r':
            if (fputs("\\r", out) == EOF) {
                return false;
            }
            break;
        case '\t':
            if (fputs("\\t", out) == EOF) {
                return false;
            }
            break;
        default:
            if (fputc((int)*cursor, out) == EOF) {
                return false;
            }
            break;
        }
        cursor++;
    }
    return fputc('"', out) != EOF;
}

static bool dump_constant_ref(FILE *out, const BytecodeProgram *program, size_t constant_index) {
    if (!out || !program || constant_index >= program->constant_count) {
        return false;
    }

    fprintf(out, "c%zu:", constant_index);
    switch (program->constants[constant_index].kind) {
    case BYTECODE_CONSTANT_LITERAL:
        fprintf(out, "%s", literal_kind_name(program->constants[constant_index].as.literal.kind));
        if (program->constants[constant_index].as.literal.kind == AST_LITERAL_BOOL) {
            fprintf(out,
                    "(%s)",
                    program->constants[constant_index].as.literal.bool_value ? "true" : "false");
        } else if (program->constants[constant_index].as.literal.text) {
            fputc('(', out);
            if (!write_quoted_text(out, program->constants[constant_index].as.literal.text)) {
                return false;
            }
            fputc(')', out);
        }
        return true;
    case BYTECODE_CONSTANT_SYMBOL:
        return write_quoted_text(out, program->constants[constant_index].as.text);
    case BYTECODE_CONSTANT_TEXT:
        return write_quoted_text(out, program->constants[constant_index].as.text);
    }

    return false;
}

static bool dump_value(FILE *out,
                       const BytecodeProgram *program,
                       const BytecodeUnit *unit,
                       BytecodeValue value) {
    if (!out || !program || !unit) {
        return false;
    }

    switch (value.kind) {
    case BYTECODE_VALUE_INVALID:
        return fputs("<invalid>", out) != EOF;
    case BYTECODE_VALUE_TEMP:
        return fprintf(out, "t%zu", value.as.temp_index) >= 0;
    case BYTECODE_VALUE_LOCAL:
        return fprintf(out,
                       "local(%zu:%s)",
                       value.as.local_index,
                       unit->locals[value.as.local_index].name) >= 0;
    case BYTECODE_VALUE_GLOBAL:
        return fprintf(out, "global(") >= 0 &&
               dump_constant_ref(out, program, value.as.global_index) &&
               fprintf(out, ")") >= 0;
    case BYTECODE_VALUE_CONSTANT:
        return fprintf(out, "const(") >= 0 &&
               dump_constant_ref(out, program, value.as.constant_index) &&
               fprintf(out, ")") >= 0;
    }

    return false;
}

static bool dump_template_part(FILE *out,
                               const BytecodeProgram *program,
                               const BytecodeUnit *unit,
                               const BytecodeTemplatePart *part) {
    if (!out || !program || !unit || !part) {
        return false;
    }

    if (part->kind == BYTECODE_TEMPLATE_PART_TEXT) {
        return fprintf(out, "text(") >= 0 &&
               dump_constant_ref(out, program, part->as.text_index) &&
               fprintf(out, ")") >= 0;
    }

    return fprintf(out, "value(") >= 0 &&
           dump_value(out, program, unit, part->as.value) &&
           fprintf(out, ")") >= 0;
}