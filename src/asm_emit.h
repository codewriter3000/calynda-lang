#ifndef CALYNDA_ASM_EMIT_H
#define CALYNDA_ASM_EMIT_H

#include "machine.h"

#include <stdbool.h>
#include <stdio.h>

bool asm_emit_program(FILE *out, const MachineProgram *program);
char *asm_emit_program_to_string(const MachineProgram *program);

#endif /* CALYNDA_ASM_EMIT_H */