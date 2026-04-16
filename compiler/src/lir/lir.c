#include "lir.h"
#include "lir_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lir_program_init(LirProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
}

void lir_program_free(LirProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->unit_count; i++) {
        lr_unit_free(&program->units[i]);
    }
    free(program->units);
    memset(program, 0, sizeof(*program));
}

const LirBuildError *lir_get_error(const LirProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool lir_format_error(const LirBuildError *error,
                      char *buffer,
                      size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (lr_source_span_is_valid(error->primary_span)) {
        if (error->has_related_span && lr_source_span_is_valid(error->related_span)) {
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

bool lir_build_program(LirProgram *program, const MirProgram *mir_program) {
    LirBuildContext context;
    const MirBuildError *mir_error;
    size_t i;

    if (!program || !mir_program) {
        return false;
    }

    lir_program_free(program);
    lir_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.mir_program = mir_program;

    mir_error = mir_get_error(mir_program);
    if (mir_error != NULL) {
        lr_set_error(&context,
                     mir_error->primary_span,
                     mir_error->has_related_span
                         ? &mir_error->related_span
                         : NULL,
                     "%s",
                     mir_error->message);
        return false;
    }

    for (i = 0; i < mir_program->unit_count; i++) {
        LirUnit unit;

        if (!lr_lower_mir_unit(&context, &mir_program->units[i], &unit)) {
            return false;
        }
        if (!lr_append_unit(program, unit)) {
            lr_unit_free(&unit);
            lr_set_error(&context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while assembling LIR units.");
            return false;
        }
    }

    return true;
}
