#ifndef CALYNDA_ASM_EMIT_H
#define CALYNDA_ASM_EMIT_H

#include "machine.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} AsmEmitError;

bool asm_emit_program(FILE *out, const MachineProgram *program);
char *asm_emit_program_to_string(const MachineProgram *program);
bool asm_emit_get_error(const MachineProgram *program, AsmEmitError *out);
bool asm_emit_format_error(const AsmEmitError *error,
                           char *buffer,
                           size_t buffer_size);

#endif /* CALYNDA_ASM_EMIT_H */
