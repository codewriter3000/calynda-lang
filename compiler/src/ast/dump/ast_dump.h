#ifndef CALYNDA_AST_DUMP_H
#define CALYNDA_AST_DUMP_H

#include "ast.h"

#include <stdbool.h>
#include <stdio.h>

bool ast_dump_program(FILE *out, const AstProgram *program);
bool ast_dump_expression(FILE *out, const AstExpression *expression);

char *ast_dump_program_to_string(const AstProgram *program);
char *ast_dump_expression_to_string(const AstExpression *expression);

#endif /* CALYNDA_AST_DUMP_H */