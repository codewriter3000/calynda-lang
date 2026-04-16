#ifndef MACHINE_INTERNAL_H
#define MACHINE_INTERNAL_H

#include "machine.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    MachineProgram      *program;
    const LirProgram    *lir_program;
    const CodegenProgram *codegen_program;
} MachineBuildContext;

/* Convenience accessors for the target descriptor from a build context */
#define mc_target(ctx)       ((ctx)->program->target_desc)
#define mc_target_kind(ctx)  ((ctx)->program->target_desc->kind)
#define mc_is_aarch64(ctx)   ((ctx)->program->target_desc->kind == TARGET_KIND_AARCH64_AAPCS_ELF)
#define mc_is_riscv64(ctx)   ((ctx)->program->target_desc->kind == TARGET_KIND_RISCV64_LP64D_ELF)

bool mc_reserve_items(void **items, size_t *capacity,
                      size_t needed, size_t item_size);
bool mc_source_span_is_valid(AstSourceSpan span);
void mc_set_error(MachineBuildContext *context,
                  AstSourceSpan primary_span,
                  const AstSourceSpan *related_span,
                  const char *format,
                  ...);
void mc_block_free(MachineBlock *block);
void mc_unit_free(MachineUnit *unit);
char *mc_copy_format(const char *format, ...);
bool mc_append_line(MachineBuildContext *context,
                    MachineBlock *block,
                    const char *format,
                    ...);
const char *mc_unit_kind_name(LirUnitKind kind);
bool mc_checked_type_is_unsigned(CheckedType type);
bool mc_unit_uses_register(const CodegenUnit *unit, CodegenRegister reg);
bool mc_emit_compare_3op(MachineBuildContext *context,
                         MachineBlock *block,
                         const char *work_reg,
                         const char *left,
                         const char *right,
                         AstBinaryOperator op,
                         bool is_unsigned,
                         bool is_riscv64);
size_t mc_compute_helper_slot_count(const LirUnit *lir_unit,
                                    const CodegenUnit *codegen_unit);
size_t mc_compute_outgoing_stack_slot_count(const LirUnit *lir_unit,
                                            const CodegenUnit *codegen_unit,
                                            const TargetDescriptor *target);
size_t mc_unit_stack_word_count(const MachineUnit *unit);
bool mc_format_slot_operand(const CodegenUnit *codegen_unit,
                            size_t slot_index, char **text);
bool mc_format_vreg_operand(const TargetDescriptor *target,
                            const CodegenUnit *codegen_unit,
                            size_t vreg_index, char **text);
bool mc_format_literal_operand(LirOperand operand, char **text);
bool mc_format_operand(const TargetDescriptor *target,
                       const LirUnit *lir_unit,
                       const CodegenUnit *codegen_unit,
                       LirOperand operand, char **text);
bool mc_format_capture_operand(size_t capture_index, char **text);
bool mc_format_helper_slot_operand(size_t helper_slot_index, char **text);
bool mc_format_incoming_arg_stack_operand(size_t argument_index, char **text);
bool mc_format_outgoing_arg_stack_operand(size_t argument_index, char **text);
bool mc_format_member_symbol_operand(const char *member, char **text);
bool mc_format_code_label_operand(const char *unit_name, char **text);
bool mc_format_template_text_operand(const char *text, char **formatted);
CalyndaRtTypeTag mc_checked_type_to_runtime_tag(CheckedType type);
bool mc_format_hetero_array_type_descriptor_operand(const LirInstruction *instruction,
                                                    char **text);
bool mc_format_union_type_descriptor_operand(const LirInstruction *instruction,
                                             char **text);
bool mc_emit_move_to_destination(MachineBuildContext *context,
                                 MachineBlock *block,
                                 const char *destination,
                                 const char *source);
bool mc_emit_store_vreg(MachineBuildContext *context,
                        const CodegenUnit *codegen_unit,
                        size_t vreg_index,
                        MachineBlock *block,
                        const char *source);
bool mc_emit_preserve_before_call(MachineBuildContext *context,
                                  const CodegenUnit *codegen_unit,
                                  MachineBlock *block);
bool mc_emit_preserve_after_call(MachineBuildContext *context,
                                 const CodegenUnit *codegen_unit,
                                 MachineBlock *block);
bool mc_emit_entry_prologue(MachineBuildContext *context,
                            const CodegenUnit *codegen_unit,
                            MachineUnit *machine_unit,
                            MachineBlock *block);
bool mc_emit_return_epilogue(MachineBuildContext *context,
                             const CodegenUnit *codegen_unit,
                             const MachineUnit *machine_unit,
                             MachineBlock *block);
const LirInstruction *mc_find_pending_call_instruction(const LirBasicBlock *block,
                                                       size_t instruction_index,
                                                       size_t *call_index_out);
const CodegenSelectedInstruction *mc_find_pending_call_selection(const CodegenBlock *block,
                                                                 size_t instruction_index,
                                                                 size_t *call_index_out);
bool mc_emit_binary(MachineBuildContext *context,
                    const LirUnit *lir_unit,
                    const CodegenUnit *codegen_unit,
                    const LirInstruction *instruction,
                    MachineBlock *block);
bool mc_emit_unary(MachineBuildContext *context,
                   const LirUnit *lir_unit,
                   const CodegenUnit *codegen_unit,
                   const LirInstruction *instruction,
                   MachineBlock *block);
bool mc_emit_cast(MachineBuildContext *context,
                  const LirUnit *lir_unit,
                  const CodegenUnit *codegen_unit,
                  const LirInstruction *instruction,
                  MachineBlock *block);
bool mc_emit_call_global(MachineBuildContext *context,
                         const LirUnit *lir_unit,
                         const CodegenUnit *codegen_unit,
                         const LirInstruction *instruction,
                         MachineBlock *block);
bool mc_emit_runtime_helper_call(MachineBuildContext *context,
                                 const CodegenUnit *codegen_unit,
                                 const RuntimeAbiHelperSignature *signature,
                                 MachineBlock *block,
                                 bool is_noreturn);
bool mc_emit_instruction(MachineBuildContext *context,
                         const LirUnit *lir_unit,
                         const CodegenUnit *codegen_unit,
                         const LirBasicBlock *lir_block,
                         const CodegenBlock *codegen_block,
                         size_t instruction_index,
                         MachineBlock *block);
bool mc_emit_instruction_runtime(MachineBuildContext *context,
                                 const LirUnit *lir_unit,
                                 const CodegenUnit *codegen_unit,
                                 const LirInstruction *instruction,
                                 const CodegenSelectedInstruction *selected,
                                 MachineBlock *block);
bool mc_emit_instruction_runtime_ext(MachineBuildContext *context,
                                     const LirUnit *lir_unit,
                                     const CodegenUnit *codegen_unit,
                                     const LirInstruction *instruction,
                                     const CodegenSelectedInstruction *selected,
                                     MachineBlock *block);
bool mc_emit_terminator(MachineBuildContext *context,
                        const LirUnit *lir_unit,
                        const CodegenUnit *codegen_unit,
                        const LirBasicBlock *lir_block,
                        const MachineUnit *machine_unit,
                        MachineBlock *block);
bool mc_build_unit(MachineBuildContext *context,
                   const LirUnit *lir_unit,
                   const CodegenUnit *codegen_unit,
                   MachineUnit *machine_unit);

#endif /* MACHINE_INTERNAL_H */
