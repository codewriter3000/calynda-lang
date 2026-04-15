#ifndef HIR_DUMP_INTERNAL_H
#define HIR_DUMP_INTERNAL_H

#include "hir_dump.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hir_dump_write_indent(FILE *out, int indent);
bool hir_dump_write_checked_type(FILE *out, CheckedType type);
bool hir_dump_write_callable_signature(FILE *out, const HirCallableSignature *signature);
void hir_dump_write_span(FILE *out, AstSourceSpan span);
const char *hir_dump_binary_operator_name_text(AstBinaryOperator operator);

bool hir_dump_block(FILE *out, const HirBlock *block, int indent);
bool hir_dump_statement(FILE *out, const HirStatement *statement, int indent);
bool hir_dump_expression(FILE *out, const HirExpression *expression, int indent);
bool hir_dump_expression_ext(FILE *out, const HirExpression *expression, int indent);

#endif
