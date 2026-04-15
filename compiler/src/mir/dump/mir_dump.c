#include "mir_dump_internal.h"

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
        case MIR_UNIT_ASM:
            unit_kind = "asm";
            break;
        }

        fprintf(out, "  Unit name=%s kind=%s return=",
                unit->name,
            unit_kind);
        if (!mir_dump_write_checked_type(out, unit->return_type)) {
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
                    mir_dump_local_kind_name(unit->locals[j].kind),
                    unit->locals[j].name);
            if (!mir_dump_write_checked_type(out, unit->locals[j].type)) {
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

                mir_dump_write_indent(out, 8);
                if (!mir_dump_instruction(out, unit, instruction)) {
                    return false;
                }
                fputc('\n', out);
            }

            mir_dump_write_indent(out, 8);
            if (!mir_dump_terminator(out, unit, block)) {
                return false;
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