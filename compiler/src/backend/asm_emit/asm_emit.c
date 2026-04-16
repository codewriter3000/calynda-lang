#include "asm_emit_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool ae_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

bool asm_emit_get_error(const MachineProgram *program, AsmEmitError *out) {
    const MachineBuildError *machine_error;

    if (!program || !out) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    machine_error = machine_get_error(program);
    if (!machine_error) {
        return false;
    }

    out->primary_span = machine_error->primary_span;
    out->related_span = machine_error->related_span;
    out->has_related_span = machine_error->has_related_span;
    memcpy(out->message, machine_error->message, sizeof(out->message));
    return true;
}

bool asm_emit_format_error(const AsmEmitError *error,
                           char *buffer,
                           size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (ae_source_span_is_valid(error->primary_span)) {
        if (error->has_related_span && ae_source_span_is_valid(error->related_span)) {
            written = snprintf(buffer,
                               buffer_size,
                               "%d:%d: %s Related location at %d:%d.",
                               error->primary_span.start_line,
                               error->primary_span.start_column,
                               error->message,
                               error->related_span.start_line,
                               error->related_span.start_column);
        } else {
            written = snprintf(buffer,
                               buffer_size,
                               "%d:%d: %s",
                               error->primary_span.start_line,
                               error->primary_span.start_column,
                               error->message);
        }
    } else {
        written = snprintf(buffer, buffer_size, "%s", error->message);
    }

    return written >= 0 && (size_t)written < buffer_size;
}

bool asm_emit_program(FILE *out, const MachineProgram *program) {
    AsmEmitContext context;
    size_t unit_index;
    bool is_arm64;

    if (!out || !program) {
        return false;
    }
    if (machine_get_error(program) != NULL) {
        return false;
    }

    memset(&context, 0, sizeof(context));
    context.program = program;
    is_arm64 = program->target_desc &&
               program->target_desc->kind == TARGET_KIND_AARCH64_AAPCS_ELF;
    {
        bool is_riscv64 = program->target_desc &&
                          program->target_desc->kind == TARGET_KIND_RISCV64_LP64D_ELF;

        if (is_arm64 || is_riscv64) {
            if (fputs(".text\n", out) == EOF) {
                return false;
            }
        } else {
            if (fputs(".intel_syntax noprefix\n.text\n", out) == EOF) {
                return false;
            }
        }
    }
    for (unit_index = 0; unit_index < program->unit_count; unit_index++) {
        if (!ae_emit_unit_text(&context, out, unit_index, &program->units[unit_index])) {
            return false;
        }
    }
    {
        bool is_riscv64 = program->target_desc &&
                          program->target_desc->kind == TARGET_KIND_RISCV64_LP64D_ELF;

        if (is_arm64) {
            if (!ae_emit_program_entry_glue_aarch64(&context, out)) {
                return false;
            }
        } else if (is_riscv64) {
            if (!ae_emit_program_entry_glue_riscv64(&context, out)) {
                return false;
            }
        } else {
            if (!ae_emit_program_entry_glue(&context, out)) {
                return false;
            }
        }
    }
    if (!ae_emit_rodata(out, &context) || !ae_emit_data(out, &context) ||
        !ae_emit_line(out, ".section .note.GNU-stack,\"\",@progbits\n")) {
        return false;
    }

    return !ferror(out);
}

char *asm_emit_program_to_string(const MachineProgram *program) {
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

    if (!asm_emit_program(stream, program) || fflush(stream) != 0 ||
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

bool ae_emit_line(FILE *out, const char *format, ...) {
    va_list args;
    int written;

    va_start(args, format);
    written = vfprintf(out, format, args);
    va_end(args);
    return written >= 0;
}

bool ae_emit_two_operand(FILE *out,
                         const char *mnemonic,
                         AsmOperand *destination,
                         AsmOperand *source) {
    if (!out || !mnemonic || !destination || !source) {
        return false;
    }

    if (strcmp(mnemonic, "mov") == 0) {
        if (destination->kind == ASM_OPERAND_REGISTER && source->kind == ASM_OPERAND_ADDRESS) {
            return ae_emit_line(out, "    lea %s, [rip + %s]\n", destination->text, source->text);
        }
        if (destination->kind == ASM_OPERAND_MEMORY &&
            (source->kind == ASM_OPERAND_MEMORY || source->kind == ASM_OPERAND_ADDRESS)) {
            if (source->kind == ASM_OPERAND_ADDRESS) {
                return ae_emit_line(out, "    lea rcx, [rip + %s]\n", source->text) &&
                       ae_emit_line(out, "    mov %s, rcx\n", destination->text);
            }
            return ae_emit_line(out, "    mov rcx, %s\n", source->text) &&
                   ae_emit_line(out, "    mov %s, rcx\n", destination->text);
        }
    }
    if (strcmp(mnemonic, "cmp") == 0 && destination->kind == ASM_OPERAND_MEMORY &&
        source->kind == ASM_OPERAND_MEMORY) {
        return ae_emit_line(out, "    mov rcx, %s\n", source->text) &&
               ae_emit_line(out, "    cmp %s, rcx\n", destination->text);
    }

    if (!ae_emit_line(out, "    %s ", mnemonic) ||
        !ae_write_operand(out, destination) ||
        !ae_emit_line(out, ", ") ||
        !ae_write_operand(out, source) ||
        !ae_emit_line(out, "\n")) {
        return false;
    }

    return true;
}

bool ae_emit_setcc(FILE *out, const char *mnemonic, AsmOperand *destination) {
    const char *byte_reg = NULL;

    if (!out || !mnemonic || !destination || destination->kind != ASM_OPERAND_REGISTER) {
        return false;
    }

    if (strcmp(destination->text, "rax") == 0) {
        byte_reg = "al";
    } else if (strcmp(destination->text, "rcx") == 0) {
        byte_reg = "cl";
    } else if (strcmp(destination->text, "rdx") == 0) {
        byte_reg = "dl";
    } else if (strcmp(destination->text, "rdi") == 0) {
        byte_reg = "dil";
    } else if (strcmp(destination->text, "rsi") == 0) {
        byte_reg = "sil";
    } else if (strcmp(destination->text, "r8") == 0) {
        byte_reg = "r8b";
    } else if (strcmp(destination->text, "r9") == 0) {
        byte_reg = "r9b";
    } else if (strcmp(destination->text, "r10") == 0) {
        byte_reg = "r10b";
    } else if (strcmp(destination->text, "r11") == 0) {
        byte_reg = "r11b";
    } else if (strcmp(destination->text, "r12") == 0) {
        byte_reg = "r12b";
    } else if (strcmp(destination->text, "r13") == 0) {
        byte_reg = "r13b";
    } else if (strcmp(destination->text, "r14") == 0) {
        byte_reg = "r14b";
    } else if (strcmp(destination->text, "r15") == 0) {
        byte_reg = "r15b";
    }

    if (!byte_reg) {
        return false;
    }

    return ae_emit_line(out, "    %s %s\n", mnemonic, byte_reg) &&
           ae_emit_line(out, "    movzx %s, %s\n", destination->text, byte_reg);
}

bool ae_emit_shift(FILE *out,
                   const char *mnemonic,
                   AsmOperand *destination,
                   AsmOperand *source) {
    if (!out || !mnemonic || !destination || !source) {
        return false;
    }
    if (source->kind == ASM_OPERAND_IMMEDIATE) {
        return ae_emit_two_operand(out, mnemonic, destination, source);
    }
    return ae_emit_line(out, "    mov rcx, ") &&
           ae_write_operand(out, source) &&
           ae_emit_line(out, "\n    %s %s, cl\n", mnemonic, destination->text);
}

bool ae_emit_div(FILE *out, const char *mnemonic, AsmOperand *source) {
    if (!out || !mnemonic || !source) {
        return false;
    }
    if (source->kind == ASM_OPERAND_IMMEDIATE || source->kind == ASM_OPERAND_ADDRESS) {
        return ae_emit_line(out, "    mov rcx, ") &&
               ae_write_operand(out, source) &&
               ae_emit_line(out, "\n    %s rcx\n", mnemonic);
    }
    return ae_emit_line(out, "    %s ", mnemonic) && ae_write_operand(out, source) && ae_emit_line(out, "\n");
}
