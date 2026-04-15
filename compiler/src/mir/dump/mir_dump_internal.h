#ifndef CALYNDA_MIR_DUMP_INTERNAL_H
#define CALYNDA_MIR_DUMP_INTERNAL_H

#include "mir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper utilities */
void mir_dump_write_indent(FILE *out, int indent);
bool mir_dump_write_checked_type(FILE *out, CheckedType type);
const char *mir_dump_binary_operator_name_text(AstBinaryOperator op);
const char *mir_dump_unary_operator_name_text(AstUnaryOperator op);
const char *mir_dump_local_kind_name(MirLocalKind kind);
bool mir_dump_template_part(FILE *out, const MirUnit *unit, const MirTemplatePart *part);
bool mir_dump_value(FILE *out, const MirUnit *unit, MirValue value);

/* Extracted instruction/terminator dumpers */
bool mir_dump_instruction(FILE *out, const MirUnit *unit, const MirInstruction *instruction);
bool mir_dump_terminator(FILE *out, const MirUnit *unit, const MirBasicBlock *block);

#endif
