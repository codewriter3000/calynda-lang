#include "machine_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *mc_unit_kind_name(LirUnitKind kind) {
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

bool machine_dump_program(FILE *out, const MachineProgram *program) {
    size_t i;
    size_t j;

    if (!out || !program) {
        return false;
    }

    fprintf(out,
            "MachineProgram target=%s scratch=%s\n",
            codegen_target_name(program->target),
            codegen_register_name(CODEGEN_REG_R14));
    if (!runtime_abi_dump_surface(out, program->target)) {
        return false;
    }
    for (i = 0; i < program->unit_count; i++) {
        const MachineUnit *unit = &program->units[i];

        fprintf(out,
                "  Unit name=%s kind=%s return=",
                unit->name,
                mc_unit_kind_name(unit->kind));
        {
            char type_buffer[64];
            if (!checked_type_to_string(unit->return_type, type_buffer, sizeof(type_buffer))) {
                return false;
            }
            if (fputs(type_buffer, out) == EOF) {
                return false;
            }
        }
        fprintf(out,
                " frame_slots=%zu spills=%zu helper_slots=%zu outgoing_stack=%zu blocks=%zu\n",
                unit->frame_slot_count,
                unit->spill_slot_count,
                unit->helper_slot_count,
                unit->outgoing_stack_slot_count,
                unit->block_count);

        fprintf(out, "    Blocks:\n");
        for (j = 0; j < unit->block_count; j++) {
            size_t k;

            fprintf(out, "      Block %s:\n", unit->blocks[j].label);
            for (k = 0; k < unit->blocks[j].instruction_count; k++) {
                fprintf(out, "        %s\n", unit->blocks[j].instructions[k].text);
            }
        }
    }

    return !ferror(out);
}

char *machine_dump_program_to_string(const MachineProgram *program) {
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

    if (!machine_dump_program(stream, program) || fflush(stream) != 0 ||
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
