#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_indent(FILE *out, int indent);
static bool write_checked_type(FILE *out, CheckedType type);
static const char *unit_kind_name(LirUnitKind kind);
static const char *slot_kind_name(LirSlotKind kind);
static const char *instruction_kind_name(LirInstructionKind kind);
static const char *terminator_kind_name(LirTerminatorKind kind);

bool codegen_dump_program(FILE *out, const CodegenProgram *program) {
    static const CodegenRegister arg_regs[] = {
        CODEGEN_REG_RDI,
        CODEGEN_REG_RSI,
        CODEGEN_REG_RDX,
        CODEGEN_REG_RCX,
        CODEGEN_REG_R8,
        CODEGEN_REG_R9
    };
    static const CodegenRegister alloc_regs[] = {
        CODEGEN_REG_R10,
        CODEGEN_REG_R11,
        CODEGEN_REG_R12,
        CODEGEN_REG_R13
    };
    size_t i;
    size_t j;

    if (!out || !program) {
        return false;
    }

    fprintf(out,
            "CodegenProgram target=%s return=%s env=%s args=[",
            codegen_target_name(program->target),
            codegen_register_name(CODEGEN_REG_RAX),
            codegen_register_name(CODEGEN_REG_R15));
    for (i = 0; i < sizeof(arg_regs) / sizeof(arg_regs[0]); i++) {
        if (i > 0 && fputs(", ", out) == EOF) {
            return false;
        }
        if (fputs(codegen_register_name(arg_regs[i]), out) == EOF) {
            return false;
        }
    }
    if (fputs("] alloc=[", out) == EOF) {
        return false;
    }
    for (i = 0; i < sizeof(alloc_regs) / sizeof(alloc_regs[0]); i++) {
        if (i > 0 && fputs(", ", out) == EOF) {
            return false;
        }
        if (fputs(codegen_register_name(alloc_regs[i]), out) == EOF) {
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
                unit_kind_name(unit->kind));
        if (!write_checked_type(out, unit->return_type)) {
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
                    slot_kind_name(unit->frame_slots[j].kind),
                    unit->frame_slots[j].name);
            if (!write_checked_type(out, unit->frame_slots[j].type)) {
                return false;
            }
            fprintf(out, " final=%s\n", unit->frame_slots[j].is_final ? "true" : "false");
        }

        fprintf(out, "    VRegs:\n");
        for (j = 0; j < unit->vreg_count; j++) {
            fprintf(out, "      VReg index=%zu type=", unit->vreg_allocations[j].vreg_index);
            if (!write_checked_type(out, unit->vreg_allocations[j].type)) {
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

                write_indent(out, 8);
                fprintf(out, "%s -> ", instruction_kind_name(selected->kind));
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

            write_indent(out, 8);
            fprintf(out, "terminator %s -> ", terminator_kind_name(unit->blocks[j].terminator.kind));
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

static const char *unit_kind_name(LirUnitKind kind) {
    switch (kind) {
    case LIR_UNIT_START:
        return "start";
    case LIR_UNIT_BINDING:
        return "binding";
    case LIR_UNIT_INIT:
        return "init";
    case LIR_UNIT_LAMBDA:
        return "lambda";
    }

    return "unknown";
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

static const char *instruction_kind_name(LirInstructionKind kind) {
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
    }

    return "unknown";
}

static const char *terminator_kind_name(LirTerminatorKind kind) {
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