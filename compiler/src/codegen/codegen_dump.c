#include "codegen.h"
#include "target.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void codegen_dump_write_indent(FILE *out, int indent);
bool codegen_dump_write_checked_type(FILE *out, CheckedType type);
const char *codegen_dump_unit_kind_name(LirUnitKind kind);
const char *codegen_dump_slot_kind_name(LirSlotKind kind);
const char *codegen_dump_instruction_kind_name(LirInstructionKind kind);
const char *codegen_dump_terminator_kind_name(LirTerminatorKind kind);

bool codegen_dump_program(FILE *out, const CodegenProgram *program) {
    const TargetDescriptor *target;
    size_t i;
    size_t j;

    if (!out || !program) {
        return false;
    }

    target = program->target_desc;
    if (!target) {
        target = target_get_default();
    }

    fprintf(out,
            "CodegenProgram target=%s return=%s env=%s args=[",
            codegen_target_name(program->target),
            target_register_name(target, target->return_register.id),
            target_register_name(target, target->closure_env_register.id));
    for (i = 0; i < target->arg_register_count; i++) {
        if (i > 0 && fputs(", ", out) == EOF) {
            return false;
        }
        if (fputs(target_register_name(target, target->arg_registers[i].id), out) == EOF) {
            return false;
        }
    }
    if (fputs("] alloc=[", out) == EOF) {
        return false;
    }
    for (i = 0; i < target->allocatable_register_count; i++) {
        if (i > 0 && fputs(", ", out) == EOF) {
            return false;
        }
        if (fputs(target_register_name(target, target->allocatable_registers[i].id), out) == EOF) {
            return false;
        }
    }
    if (fputs("]\n", out) == EOF) {
        return false;
    }

    for (i = 0; i < program->unit_count; i++) {
        const CodegenUnit *unit = &program->units[i];

        fprintf(out,
                "  Unit name=%s kind=%s return=",
                unit->name,
                codegen_dump_unit_kind_name(unit->kind));
        if (!codegen_dump_write_checked_type(out, unit->return_type)) {
            return false;
        }
        fprintf(out,
                " frame_slots=%zu spills=%zu vregs=%zu blocks=%zu\n",
                unit->frame_slot_count,
                unit->spill_slot_count,
                unit->vreg_count,
                unit->block_count);

        fprintf(out, "    FrameSlots:\n");
        for (j = 0; j < unit->frame_slot_count; j++) {
            fprintf(out,
                    "      FrameSlot index=%zu from=%s name=%s type=",
                    unit->frame_slots[j].frame_slot_index,
                    codegen_dump_slot_kind_name(unit->frame_slots[j].kind),
                    unit->frame_slots[j].name);
            if (!codegen_dump_write_checked_type(out, unit->frame_slots[j].type)) {
                return false;
            }
            fprintf(out, " final=%s\n", unit->frame_slots[j].is_final ? "true" : "false");
        }

        fprintf(out, "    VRegs:\n");
        for (j = 0; j < unit->vreg_count; j++) {
            fprintf(out, "      VReg index=%zu type=", unit->vreg_allocations[j].vreg_index);
            if (!codegen_dump_write_checked_type(out, unit->vreg_allocations[j].type)) {
                return false;
            }
            if (unit->vreg_allocations[j].location.kind == CODEGEN_VREG_REGISTER) {
                fprintf(out,
                        " -> reg(%s)\n",
                        codegen_register_name(unit->vreg_allocations[j].location.as.reg));
            } else {
                fprintf(out,
                        " -> spill(%zu)\n",
                        unit->vreg_allocations[j].location.as.spill_slot_index);
            }
        }

        fprintf(out, "    Blocks:\n");
        for (j = 0; j < unit->block_count; j++) {
            size_t k;

            fprintf(out, "      Block %s:\n", unit->blocks[j].label);
            for (k = 0; k < unit->blocks[j].instruction_count; k++) {
                const CodegenSelectedInstruction *selected = &unit->blocks[j].instructions[k];

                codegen_dump_write_indent(out, 8);
                fprintf(out, "%s -> ", codegen_dump_instruction_kind_name(selected->kind));
                if (selected->selection.kind == CODEGEN_SELECTION_DIRECT) {
                    fprintf(out,
                            "direct %s\n",
                            codegen_direct_pattern_name(selected->selection.as.direct_pattern));
                } else {
                    fprintf(out,
                            "runtime %s\n",
                            codegen_runtime_helper_name(selected->selection.as.runtime_helper));
                }
            }

            codegen_dump_write_indent(out, 8);
            fprintf(out, "terminator %s -> ", codegen_dump_terminator_kind_name(unit->blocks[j].terminator.kind));
            if (unit->blocks[j].terminator.selection.kind == CODEGEN_SELECTION_DIRECT) {
                fprintf(out,
                        "direct %s\n",
                        codegen_direct_pattern_name(unit->blocks[j].terminator.selection.as.direct_pattern));
            } else {
                fprintf(out,
                        "runtime %s\n",
                        codegen_runtime_helper_name(unit->blocks[j].terminator.selection.as.runtime_helper));
            }
        }
    }

    return !ferror(out);
}

char *codegen_dump_program_to_string(const CodegenProgram *program) {
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

    if (!codegen_dump_program(stream, program) || fflush(stream) != 0 ||
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