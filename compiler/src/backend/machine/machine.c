#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

void machine_program_init(MachineProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
    program->target = CODEGEN_TARGET_X86_64_SYSV_ELF;
    program->target_desc = target_get_default();
}

void machine_program_free(MachineProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->unit_count; i++) {
        mc_unit_free(&program->units[i]);
    }
    free(program->units);
    memset(program, 0, sizeof(*program));
}

bool machine_program_has_boot(const MachineProgram *program) {
    size_t i;

    if (!program) {
        return false;
    }

    for (i = 0; i < program->unit_count; i++) {
        if (program->units[i].kind == LIR_UNIT_START &&
            program->units[i].is_boot) {
            return true;
        }
    }
    return false;
}

const MachineBuildError *machine_get_error(const MachineProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool machine_format_error(const MachineBuildError *error,
                          char *buffer,
                          size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (mc_source_span_is_valid(error->primary_span)) {
        if (error->has_related_span && mc_source_span_is_valid(error->related_span)) {
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

bool machine_build_program(MachineProgram *program,
                           const LirProgram *lir_program,
                           const CodegenProgram *codegen_program) {
    MachineBuildContext context;
    const LirBuildError *lir_error;
    const CodegenBuildError *codegen_error;
    size_t i;

    if (!program || !lir_program || !codegen_program) {
        return false;
    }

    machine_program_free(program);
    machine_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.lir_program = lir_program;
    context.codegen_program = codegen_program;

    lir_error = lir_get_error(lir_program);
    if (lir_error != NULL) {
        mc_set_error(&context,
                     lir_error->primary_span,
                     lir_error->has_related_span
                         ? &lir_error->related_span
                         : NULL,
                     "%s",
                     lir_error->message);
        return false;
    }
    codegen_error = codegen_get_error(codegen_program);
    if (codegen_error != NULL) {
        mc_set_error(&context,
                     codegen_error->primary_span,
                     codegen_error->has_related_span
                         ? &codegen_error->related_span
                         : NULL,
                     "%s",
                     codegen_error->message);
        return false;
    }
    if (codegen_program->target != CODEGEN_TARGET_X86_64_SYSV_ELF &&
        codegen_program->target != TARGET_KIND_AARCH64_AAPCS_ELF &&
        codegen_program->target != TARGET_KIND_RISCV64_LP64D_ELF) {
        mc_set_error(&context,
                     (AstSourceSpan){0},
                     NULL,
                     "Unsupported machine emission target %d.",
                     codegen_program->target);
        return false;
    }
    if (lir_program->unit_count != codegen_program->unit_count) {
        mc_set_error(&context,
                     (AstSourceSpan){0},
                     NULL,
                     "Machine emission requires matching LIR/codegen unit counts.");
        return false;
    }

    program->target = codegen_program->target;
    program->target_desc = codegen_program->target_desc;
    program->unit_count = lir_program->unit_count;
    if (program->unit_count > 0) {
        program->units = calloc(program->unit_count, sizeof(*program->units));
        if (!program->units) {
            mc_set_error(&context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while allocating machine units.");
            return false;
        }
    }

    for (i = 0; i < program->unit_count; i++) {
        if (!mc_build_unit(&context,
                           &lir_program->units[i],
                           &codegen_program->units[i],
                           &program->units[i])) {
            return false;
        }
    }

    return true;
}
