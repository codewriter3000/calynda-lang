#ifndef BYTECODE_INTERNAL_H
#define BYTECODE_INTERNAL_H

#include "bytecode.h"

typedef struct {
    BytecodeProgram *program;
    const MirProgram *mir_program;
} BytecodeBuildContext;

/* bytecode_memory.c */
bool bc_reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size);
void bc_constant_free(BytecodeConstant *constant);
void bc_template_part_free(BytecodeTemplatePart *part);
void bc_instruction_free(BytecodeInstruction *instruction);
void bc_block_free(BytecodeBasicBlock *block);
void bc_unit_free(BytecodeUnit *unit);

/* bytecode_helpers.c */
bool bc_source_span_is_valid(AstSourceSpan span);
void bc_set_error(BytecodeBuildContext *context,
                  AstSourceSpan primary_span,
                  const AstSourceSpan *related_span,
                  const char *format,
                  ...);
BytecodeValue bc_invalid_value(void);
BytecodeLocalKind bc_local_kind_from_mir(MirLocalKind kind);
BytecodeUnitKind bc_unit_kind_from_mir(MirUnitKind kind);
size_t bc_find_unit_index(const MirProgram *program, const char *name);
CalyndaRtTypeTag bc_checked_type_to_runtime_tag(CheckedType type);
size_t bc_intern_union_type_descriptor(BytecodeBuildContext *context,
                                       const char *name,
                                       size_t generic_param_count,
                                       const CalyndaRtTypeTag *generic_param_tags,
                                       const char *const *variant_names,
                                       const CalyndaRtTypeTag *variant_payload_tags,
                                       size_t variant_count);
bool bc_lower_hetero_array_instruction(BytecodeBuildContext *context,
                                       const MirInstruction *instruction,
                                       BytecodeInstruction *lowered);
bool bc_lower_union_new_instruction(BytecodeBuildContext *context,
                                    const MirInstruction *instruction,
                                    BytecodeInstruction *lowered);

/* bytecode_constants.c */
size_t bc_intern_text_constant(BytecodeBuildContext *context,
                               BytecodeConstantKind kind,
                               const char *text);
size_t bc_intern_literal_constant(BytecodeBuildContext *context,
                                  AstLiteralKind kind,
                                  const char *text,
                                  bool bool_value);
bool bc_value_from_mir_value(BytecodeBuildContext *context,
                             MirValue value,
                             BytecodeValue *lowered);
bool bc_lower_template_part(BytecodeBuildContext *context,
                            const MirTemplatePart *part,
                            BytecodeTemplatePart *lowered);

/* bytecode_lower_instr.c */
bool bc_lower_instruction(BytecodeBuildContext *context,
                          const MirInstruction *instruction,
                          BytecodeInstruction *lowered);

/* bytecode_lower.c */
bool bc_lower_unit(BytecodeBuildContext *context,
                   const MirUnit *mir_unit,
                   BytecodeUnit *unit);

#endif
