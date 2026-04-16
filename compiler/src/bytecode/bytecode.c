#include "bytecode.h"
#include "bytecode_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bytecode_program_init(BytecodeProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
    program->target = BYTECODE_TARGET_PORTABLE_V1;
}

void bytecode_program_free(BytecodeProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->constant_count; i++) {
        bc_constant_free(&program->constants[i]);
    }
    free(program->constants);

    for (i = 0; i < program->unit_count; i++) {
        bc_unit_free(&program->units[i]);
    }
    free(program->units);

    memset(program, 0, sizeof(*program));
}

const BytecodeBuildError *bytecode_get_error(const BytecodeProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool bytecode_format_error(const BytecodeBuildError *error,
                           char *buffer,
                           size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (bc_source_span_is_valid(error->primary_span)) {
        if (error->has_related_span && bc_source_span_is_valid(error->related_span)) {
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

const char *bytecode_target_name(BytecodeTargetKind target) {
    switch (target) {
    case BYTECODE_TARGET_PORTABLE_V1:
        return "portable-v1";
    }

    return "unknown";
}

bool bytecode_build_program(BytecodeProgram *program, const MirProgram *mir_program) {
    BytecodeBuildContext context;
    size_t unit_index;

    if (!program || !mir_program) {
        return false;
    }

    bytecode_program_free(program);
    bytecode_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.mir_program = mir_program;

    if (mir_get_error(mir_program) != NULL) {
        const MirBuildError *mir_error = mir_get_error(mir_program);

        bc_set_error(&context,
                     mir_error->primary_span,
                     mir_error->has_related_span
                         ? &mir_error->related_span
                         : NULL,
                     "%s",
                     mir_error->message);
        return false;
    }

    program->unit_count = mir_program->unit_count;
    if (program->unit_count > 0) {
        program->units = calloc(program->unit_count, sizeof(*program->units));
        if (!program->units) {
            bc_set_error(&context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while allocating bytecode units.");
            return false;
        }
    }

    for (unit_index = 0; unit_index < mir_program->unit_count; unit_index++) {
        if (!bc_lower_unit(&context, &mir_program->units[unit_index], &program->units[unit_index])) {
            return false;
        }
    }

    return true;
}
