#include "bytecode_dump_internal.h"

static bool bytecode_dump_terminator(FILE *out,
                                     const BytecodeProgram *program,
                                     const BytecodeUnit *unit,
                                     const BytecodeBasicBlock *block);

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
                    bytecode_dump_literal_kind_name(program->constants[i].as.literal.kind));
            if (program->constants[i].as.literal.kind == AST_LITERAL_BOOL) {
                fprintf(out,
                        " value=%s",
                        program->constants[i].as.literal.bool_value ? "true" : "false");
            } else if (program->constants[i].as.literal.text) {
                fprintf(out, " text=");
                if (!bytecode_dump_write_quoted_text(out,
                        program->constants[i].as.literal.text)) {
                    return false;
                }
            }
            break;
        case BYTECODE_CONSTANT_SYMBOL:
            fprintf(out, "symbol ");
            if (!bytecode_dump_write_quoted_text(out, program->constants[i].as.text)) {
                return false;
            }
            break;
        case BYTECODE_CONSTANT_TEXT:
            fprintf(out, "text ");
            if (!bytecode_dump_write_quoted_text(out, program->constants[i].as.text)) {
                return false;
            }
            break;
        case BYTECODE_CONSTANT_TYPE_DESCRIPTOR:
            fprintf(out,
                    "type_desc name=%s generic_params=[",
                    program->constants[i].as.type_descriptor.name);
            for (j = 0; j < program->constants[i].as.type_descriptor.generic_param_count; j++) {
                CalyndaRtTypeTag tag = program->constants[i].as.type_descriptor.generic_param_tags
                    ? program->constants[i].as.type_descriptor.generic_param_tags[j]
                    : CALYNDA_RT_TYPE_RAW_WORD;

                if (j > 0) {
                    fputs(", ", out);
                }
                fputs(bytecode_dump_type_tag_name(tag), out);
            }
            fputs("] variants=[", out);
            for (j = 0; j < program->constants[i].as.type_descriptor.variant_count; j++) {
                const char *variant_name =
                    program->constants[i].as.type_descriptor.variant_names
                        ? program->constants[i].as.type_descriptor.variant_names[j]
                        : NULL;

                if (j > 0) {
                    fputs(", ", out);
                }
                if (variant_name) {
                    fprintf(out,
                            "%s:%s",
                            variant_name,
                            bytecode_dump_type_tag_name(
                                program->constants[i].as.type_descriptor.variant_payload_tags[j]));
                } else {
                    fprintf(out,
                            "%s",
                            bytecode_dump_type_tag_name(
                                program->constants[i].as.type_descriptor.variant_payload_tags[j]));
                }
            }
            fputc(']', out);
            break;
        }
        fputc('\n', out);
    }

    for (i = 0; i < program->unit_count; i++) {
        const BytecodeUnit *unit = &program->units[i];

        fprintf(out,
                "  Unit name=%s kind=%s return=",
                unit->name,
                bytecode_dump_unit_kind_name(unit->kind));
        if (!bytecode_dump_write_checked_type(out, unit->return_type)) {
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
                    bytecode_dump_local_kind_name(unit->locals[j].kind),
                    unit->locals[j].name);
            if (!bytecode_dump_write_checked_type(out, unit->locals[j].type)) {
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
                const BytecodeInstruction *instruction =
                    &block->instructions[instruction_index];

                fputs("        ", out);
                if (!bytecode_dump_instruction(out, program, unit, instruction)) {
                    return false;
                }
                fputc('\n', out);
            }

            fputs("        ", out);
            if (!bytecode_dump_terminator(out, program, unit, block)) {
                return false;
            }
            fputc('\n', out);
        }
    }

    return !ferror(out);
}

static bool bytecode_dump_terminator(FILE *out,
                              const BytecodeProgram *program,
                              const BytecodeUnit *unit,
                              const BytecodeBasicBlock *block) {
    switch (block->terminator.kind) {
    case BYTECODE_TERM_NONE:
        fputs("BC_TERM_NONE", out);
        break;
    case BYTECODE_TERM_RETURN:
        fputs("BC_RETURN", out);
        if (block->terminator.as.return_term.has_value) {
            fputc(' ', out);
            if (!bytecode_dump_value(out, program, unit,
                    block->terminator.as.return_term.value)) {
                return false;
            }
        }
        break;
    case BYTECODE_TERM_JUMP:
        fprintf(out, "BC_JUMP bb%zu", block->terminator.as.jump_term.target_block);
        break;
    case BYTECODE_TERM_BRANCH:
        fputs("BC_BRANCH ", out);
        if (!bytecode_dump_value(out, program, unit,
                block->terminator.as.branch_term.condition) ||
            fprintf(out,
                    " -> bb%zu, bb%zu",
                    block->terminator.as.branch_term.true_block,
                    block->terminator.as.branch_term.false_block) < 0) {
            return false;
        }
        break;
    case BYTECODE_TERM_THROW:
        fputs("BC_THROW ", out);
        if (!bytecode_dump_value(out, program, unit,
                block->terminator.as.throw_term.value)) {
            return false;
        }
        break;
    }

    return true;
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

