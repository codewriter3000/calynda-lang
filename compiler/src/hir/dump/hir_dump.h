#ifndef CALYNDA_HIR_DUMP_H
#define CALYNDA_HIR_DUMP_H

#include "hir.h"

#include <stdbool.h>
#include <stdio.h>

bool hir_dump_program(FILE *out, const HirProgram *program);
char *hir_dump_program_to_string(const HirProgram *program);

#endif /* CALYNDA_HIR_DUMP_H */