#ifndef LIR_DUMP_INTERNAL_H
#define LIR_DUMP_INTERNAL_H

#include "lir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lir_dump_write_indent(FILE *out, int indent);
bool lir_dump_write_checked_type(FILE *out, CheckedType type);
const char *lir_dump_binary_operator_name_text(AstBinaryOperator operator);
const char *lir_dump_unary_operator_name_text(AstUnaryOperator operator);
const char *lir_dump_slot_kind_name(LirSlotKind kind);
const char *lir_dump_type_tag_name(CalyndaRtTypeTag tag);
bool lir_dump_type_descriptor(FILE *out, const CalyndaRtTypeDescriptor *type_desc);
bool lir_dump_template_part(FILE *out, const LirUnit *unit, const LirTemplatePart *part);
bool lir_dump_operand(FILE *out, const LirUnit *unit, LirOperand operand);

bool lir_dump_instruction(FILE *out, const LirUnit *unit, const LirInstruction *instruction);
bool lir_dump_terminator(FILE *out, const LirUnit *unit, const LirBasicBlock *block);

#endif
