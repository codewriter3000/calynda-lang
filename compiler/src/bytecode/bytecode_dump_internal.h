#ifndef BYTECODE_DUMP_INTERNAL_H
#define BYTECODE_DUMP_INTERNAL_H

#include "bytecode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool bytecode_dump_write_checked_type(FILE *out, CheckedType type);
const char *bytecode_dump_local_kind_name(BytecodeLocalKind kind);
const char *bytecode_dump_unit_kind_name(BytecodeUnitKind kind);
const char *bytecode_dump_binary_operator_name_text(AstBinaryOperator operator);
const char *bytecode_dump_unary_operator_name_text(AstUnaryOperator operator);
const char *bytecode_dump_literal_kind_name(AstLiteralKind kind);
const char *bytecode_dump_type_tag_name(CalyndaRtTypeTag tag);
bool bytecode_dump_write_quoted_text(FILE *out, const char *text);
bool bytecode_dump_constant_ref(FILE *out, const BytecodeProgram *program, size_t constant_index);
bool bytecode_dump_value(FILE *out,
                         const BytecodeProgram *program,
                         const BytecodeUnit *unit,
                         BytecodeValue value);
bool bytecode_dump_template_part(FILE *out,
                                 const BytecodeProgram *program,
                                 const BytecodeUnit *unit,
                                 const BytecodeTemplatePart *part);

bool bytecode_dump_instruction(FILE *out,
                               const BytecodeProgram *program,
                               const BytecodeUnit *unit,
                               const BytecodeInstruction *instruction);

#endif
