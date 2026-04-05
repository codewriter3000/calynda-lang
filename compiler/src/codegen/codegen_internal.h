#ifndef CODEGEN_INTERNAL_H
#define CODEGEN_INTERNAL_H

#include "codegen.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    CodegenProgram         *program;
    const LirProgram       *lir_program;
    const TargetDescriptor *target;
} CodegenBuildContext;

/* codegen_helpers.c */
bool cg_source_span_is_valid(AstSourceSpan span);
void cg_set_error(CodegenBuildContext *context,
                  AstSourceSpan primary_span,
                  const AstSourceSpan *related_span,
                  const char *format,
                  ...);
void cg_block_free(CodegenBlock *block);
void cg_unit_free(CodegenUnit *unit);
CheckedType cg_checked_type_invalid_value(void);
bool cg_is_direct_scalar_type(CheckedType type);

/* codegen_infer.c */
void cg_infer_vreg_types(const LirUnit *lir_unit,
                         CheckedType *types,
                         size_t type_count);

/* codegen_select.c */
bool cg_select_instruction(CodegenBuildContext *context,
                           const LirInstruction *instruction,
                           CodegenSelectedInstruction *selected);
bool cg_select_terminator(CodegenBuildContext *context,
                          const LirTerminator *terminator,
                          CodegenSelectedTerminator *selected);

/* codegen_build.c */
bool cg_build_unit(CodegenBuildContext *context,
                   const LirUnit *lir_unit,
                   CodegenUnit *unit);

#endif
