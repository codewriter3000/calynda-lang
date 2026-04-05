#include "codegen_internal.h"

void codegen_program_init(CodegenProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
    program->target = CODEGEN_TARGET_X86_64_SYSV_ELF;
    program->target_desc = target_get_default();
}

void codegen_program_free(CodegenProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->unit_count; i++) {
        cg_unit_free(&program->units[i]);
    }
    free(program->units);
    memset(program, 0, sizeof(*program));
}

const CodegenBuildError *codegen_get_error(const CodegenProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool codegen_format_error(const CodegenBuildError *error,
                          char *buffer,
                          size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (cg_source_span_is_valid(error->primary_span)) {
        if (error->has_related_span && cg_source_span_is_valid(error->related_span)) {
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

bool codegen_build_program(CodegenProgram *program,
                           const LirProgram *lir_program,
                           const TargetDescriptor *target) {
    CodegenBuildContext context;
    size_t i;

    if (!program || !lir_program) {
        return false;
    }

    codegen_program_free(program);
    codegen_program_init(program);

    if (target) {
        program->target = target->kind;
        program->target_desc = target;
    }

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.lir_program = lir_program;
    context.target = program->target_desc;

    if (lir_get_error(lir_program) != NULL) {
        cg_set_error(&context,
                     (AstSourceSpan){0},
                     NULL,
                     "Cannot build codegen plan while the LIR reports errors.");
        return false;
    }

    program->unit_count = lir_program->unit_count;
    if (program->unit_count > 0) {
        program->units = calloc(program->unit_count, sizeof(*program->units));
        if (!program->units) {
            cg_set_error(&context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while allocating codegen units.");
            return false;
        }
    }

    for (i = 0; i < program->unit_count; i++) {
        if (!cg_build_unit(&context, &lir_program->units[i], &program->units[i])) {
            return false;
        }
    }

    return true;
}