#include "lir_dump_internal.h"

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
        case LIR_UNIT_ASM:
            unit_kind = "asm";
            break;
        }

        fprintf(out, "  Unit name=%s kind=%s return=", unit->name, unit_kind);
        if (!lir_dump_write_checked_type(out, unit->return_type)) {
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
                    lir_dump_slot_kind_name(unit->slots[j].kind),
                    unit->slots[j].name);
            if (!lir_dump_write_checked_type(out, unit->slots[j].type)) {
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

                lir_dump_write_indent(out, 8);
                if (!lir_dump_instruction(out, unit, instruction)) {
                    return false;
                }
                fputc('\n', out);
            }

            lir_dump_write_indent(out, 8);
            if (!lir_dump_terminator(out, unit, block)) {
                return false;
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