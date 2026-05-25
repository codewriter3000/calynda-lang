#include "asm_emit_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool ae_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

static size_t ae_find_program_unit_index(const AsmEmitContext *context,
                                         const char *name) {
    size_t i;

    if (!context || !context->program || !name) {
        return (size_t)-1;
    }

    for (i = 0; i < context->program->unit_count; i++) {
        if (context->program->units[i].name &&
            strcmp(context->program->units[i].name, name) == 0) {
            return i;
        }
    }

    return (size_t)-1;
}

static void ae_mark_unit_reachable(AsmEmitContext *context, size_t unit_index);

static void ae_mark_named_unit_reachable(AsmEmitContext *context, const char *name) {
    size_t unit_index = ae_find_program_unit_index(context, name);

    if (unit_index == (size_t)-1) {
        return;
    }

    ae_mark_unit_reachable(context, unit_index);
}

static void ae_mark_paren_references(AsmEmitContext *context,
                                     const char *instruction_text,
                                     const char *prefix) {
    const char *cursor;
    size_t prefix_length;

    if (!context || !instruction_text || !prefix) {
        return;
    }

    prefix_length = strlen(prefix);
    cursor = instruction_text;
    while ((cursor = strstr(cursor, prefix)) != NULL) {
        const char *name_start = cursor + prefix_length;
        const char *name_end = strchr(name_start, ')');

        if (!name_end) {
            return;
        }

        if (name_end > name_start) {
            char *name = ae_copy_text_n(name_start, (size_t)(name_end - name_start));

            if (name) {
                ae_mark_named_unit_reachable(context, name);
                free(name);
            }
        }
        cursor = name_end + 1;
    }
}

static void ae_mark_direct_call_reference(AsmEmitContext *context,
                                          const char *instruction_text) {
    const char *cursor;
    const char *name_end;
    char *name;

    if (!context || !instruction_text) {
        return;
    }

    cursor = instruction_text;
    while (*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }

    if (ae_starts_with(cursor, "call ")) {
        cursor += 5;
    } else if (ae_starts_with(cursor, "bl ")) {
        cursor += 3;
    } else {
        return;
    }

    while (*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }
    name_end = cursor;
    while (*name_end != '\0' && *name_end != ' ' && *name_end != '\t' && *name_end != ',') {
        name_end++;
    }

    if (name_end == cursor) {
        return;
    }

    name = ae_copy_text_n(cursor, (size_t)(name_end - cursor));
    if (!name) {
        return;
    }

    ae_mark_named_unit_reachable(context, name);
    free(name);
}

static void ae_mark_unit_dependencies(AsmEmitContext *context,
                                      const MachineUnit *unit) {
    size_t block_index;

    if (!context || !unit) {
        return;
    }

    for (block_index = 0; block_index < unit->block_count; block_index++) {
        size_t instruction_index;

        for (instruction_index = 0;
             instruction_index < unit->blocks[block_index].instruction_count;
             instruction_index++) {
            const char *instruction_text =
                unit->blocks[block_index].instructions[instruction_index].text;

            ae_mark_paren_references(context, instruction_text, "code(");
            ae_mark_paren_references(context, instruction_text, "global(");
            ae_mark_direct_call_reference(context, instruction_text);
        }
    }
}

static void ae_mark_unit_reachable(AsmEmitContext *context, size_t unit_index) {
    if (!context || !context->program || !context->reachable_units ||
        unit_index >= context->program->unit_count || context->reachable_units[unit_index]) {
        return;
    }

    context->reachable_units[unit_index] = true;
    ae_mark_unit_dependencies(context, &context->program->units[unit_index]);
}

static bool ae_compute_reachable_units(AsmEmitContext *context) {
    size_t unit_index;
    bool found_start = false;

    if (!context || !context->program) {
        return false;
    }

    if (context->program->unit_count == 0) {
        return true;
    }

    context->reachable_units = calloc(context->program->unit_count,
                                      sizeof(*context->reachable_units));
    if (!context->reachable_units) {
        return false;
    }

    for (unit_index = 0; unit_index < context->program->unit_count; unit_index++) {
        if (context->program->units[unit_index].kind == LIR_UNIT_START) {
            found_start = true;
            ae_mark_unit_reachable(context, unit_index);
        }
    }

    if (!found_start) {
        memset(context->reachable_units,
               1,
               context->program->unit_count * sizeof(*context->reachable_units));
    }

    return true;
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
    bool ok = false;

    if (!out || !program) {
        return false;
    }
    if (machine_get_error(program) != NULL) {
        return false;
    }

    memset(&context, 0, sizeof(context));
    context.program = program;
    if (!ae_compute_reachable_units(&context)) {
        goto cleanup;
    }
    is_arm64 = program->target_desc &&
               program->target_desc->kind == TARGET_KIND_AARCH64_AAPCS_ELF;
    {
        bool is_riscv64 = program->target_desc &&
                          program->target_desc->kind == TARGET_KIND_RISCV64_LP64D_ELF;

        if (is_arm64 || is_riscv64) {
            if (fputs(".text\n", out) == EOF) {
                goto cleanup;
            }
        } else {
            if (fputs(".intel_syntax noprefix\n.text\n", out) == EOF) {
                goto cleanup;
            }
        }
    }
    for (unit_index = 0; unit_index < program->unit_count; unit_index++) {
        if (!ae_is_unit_reachable(&context, unit_index)) {
            continue;
        }
        if (!ae_emit_unit_text(&context, out, unit_index, &program->units[unit_index])) {
            goto cleanup;
        }
    }
    {
        bool is_riscv64 = program->target_desc &&
                          program->target_desc->kind == TARGET_KIND_RISCV64_LP64D_ELF;

        if (is_arm64) {
            if (!ae_emit_program_entry_glue_aarch64(&context, out)) {
                goto cleanup;
            }
        } else if (is_riscv64) {
            if (!ae_emit_program_entry_glue_riscv64(&context, out)) {
                goto cleanup;
            }
        } else {
            if (!ae_emit_program_entry_glue(&context, out)) {
                goto cleanup;
            }
        }
    }
    if (!ae_emit_rodata(out, &context) || !ae_emit_data(out, &context) ||
        !ae_emit_line(out, ".section .note.GNU-stack,\"\",@progbits\n")) {
        goto cleanup;
    }

    ok = !ferror(out);

cleanup:
    free(context.reachable_units);
    return ok;
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

#include "asm_emit_p2.inc"
