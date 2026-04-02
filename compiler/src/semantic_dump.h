#ifndef CALYNDA_SEMANTIC_DUMP_H
#define CALYNDA_SEMANTIC_DUMP_H

#include "symbol_table.h"
#include "type_checker.h"

#include <stdbool.h>
#include <stdio.h>

bool semantic_dump_program(FILE *out,
                           const SymbolTable *symbols,
                           const TypeChecker *checker);
char *semantic_dump_program_to_string(const SymbolTable *symbols,
                                      const TypeChecker *checker);

#endif /* CALYNDA_SEMANTIC_DUMP_H */