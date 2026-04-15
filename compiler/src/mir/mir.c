#include "mir_internal.h"

const char *MIR_MODULE_INIT_NAME = "__mir$module_init";

void mir_program_init(MirProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
}

void mir_program_free(MirProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->unit_count; i++) {
        mr_unit_free(&program->units[i]);
    }
    free(program->units);
    memset(program, 0, sizeof(*program));
}

const MirBuildError *mir_get_error(const MirProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool mir_format_error(const MirBuildError *error,
                      char *buffer,
                      size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (mr_source_span_is_valid(error->primary_span)) {
        if (error->has_related_span && mr_source_span_is_valid(error->related_span)) {
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

bool mr_top_level_binding_uses_lambda_unit(const HirTopLevelDecl *decl) {
    return decl &&
           decl->kind == HIR_TOP_LEVEL_BINDING &&
           decl->as.binding.is_callable &&
           decl->as.binding.initializer &&
           decl->as.binding.initializer->kind == HIR_EXPR_LAMBDA;
}

