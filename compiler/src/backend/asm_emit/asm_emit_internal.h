#ifndef ASM_EMIT_INTERNAL_H
#define ASM_EMIT_INTERNAL_H

#include "asm_emit.h"
#include "machine.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    ASM_OPERAND_REGISTER = 0,
    ASM_OPERAND_IMMEDIATE,
    ASM_OPERAND_MEMORY,
    ASM_OPERAND_ADDRESS,
    ASM_OPERAND_BRANCH_LABEL,
    ASM_OPERAND_CALL_TARGET
} AsmOperandKind;

typedef struct {
    char          *text;
    AsmOperandKind kind;
} AsmOperand;

typedef struct {
    char *name;
    char *symbol;
} AsmUnitSymbol;

typedef struct {
    char *name;
    char *symbol;
    bool  has_store;
} AsmGlobalSymbol;

typedef struct {
    char *text;
    size_t length;
    char *label;
} AsmByteLiteral;

typedef struct {
    char *text;
    size_t length;
    char *bytes_label;
    char *object_label;
} AsmStringObjectLiteral;

typedef struct {
    bool   saves_r12;
    bool   saves_r13;
    bool   preserves_r10;
    bool   preserves_r11;
    bool   has_calls;
    size_t saved_reg_words;
    size_t frame_words;
    size_t spill_words;
    size_t helper_words;
    size_t call_preserve_words;
    size_t alignment_pad_words;
    size_t outgoing_words;
    size_t total_local_words;
} AsmUnitLayout;

typedef struct {
    const MachineProgram *program;
    AsmUnitSymbol        *unit_symbols;
    size_t                unit_symbol_count;
    size_t                unit_symbol_capacity;
    AsmGlobalSymbol      *global_symbols;
    size_t                global_symbol_count;
    size_t                global_symbol_capacity;
    AsmByteLiteral       *byte_literals;
    size_t                byte_literal_count;
    size_t                byte_literal_capacity;
    AsmStringObjectLiteral *string_literals;
    size_t                 string_literal_count;
    size_t                 string_literal_capacity;
    size_t                next_label_id;
} AsmEmitContext;

/* asm_emit_helpers.c */
bool ae_reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size);
char *ae_copy_text(const char *text);
char *ae_copy_text_n(const char *text, size_t length);
char *ae_copy_format(const char *format, ...);
char *ae_sanitize_symbol(const char *name);
bool ae_is_register_name(const char *text);
bool ae_starts_with(const char *text, const char *prefix);
bool ae_ends_with(const char *text, const char *suffix);
char *ae_trim_copy(const char *text);
bool ae_split_binary_operands(const char *text, char **left, char **right);
char *ae_between_parens(const char *text, const char *prefix);
bool ae_decode_quoted_text(const char *quoted, char **decoded, size_t *decoded_length);

/* asm_emit_symbols.c */
AsmUnitSymbol *ae_ensure_unit_symbol(AsmEmitContext *context, const char *name);
AsmGlobalSymbol *ae_ensure_global_symbol(AsmEmitContext *context, const char *name, bool has_store);
const MachineUnit *ae_find_program_unit(const AsmEmitContext *context, const char *name);
AsmByteLiteral *ae_ensure_byte_literal(AsmEmitContext *context, const char *text, size_t length, const char *prefix);
AsmStringObjectLiteral *ae_ensure_string_literal(AsmEmitContext *context, const char *text, size_t length);
char *ae_closure_wrapper_symbol_name(const char *unit_name);
const MachineFrameSlot *ae_find_frame_slot(const MachineUnit *unit, const char *name);
size_t ae_frame_slot_offset(const AsmUnitLayout *layout, size_t slot_index);
size_t ae_spill_slot_offset(const AsmUnitLayout *layout, const MachineUnit *unit, size_t spill_index);
size_t ae_helper_slot_offset(const AsmUnitLayout *layout, const MachineUnit *unit, size_t helper_index);
size_t ae_call_preserve_offset(const AsmUnitLayout *layout, const MachineUnit *unit, size_t preserve_index);

/* asm_emit_operand.c */
AsmUnitLayout ae_compute_unit_layout(const MachineUnit *unit);
bool ae_translate_operand(AsmEmitContext *context, const MachineUnit *unit, const AsmUnitLayout *layout, size_t block_index, size_t instruction_index, const char *operand_text, bool destination, AsmOperand *operand);

/* asm_emit_operand_ext.c */
bool ae_translate_operand_ext(AsmEmitContext *context, const MachineUnit *unit, const AsmUnitLayout *layout, const char *operand_text, bool destination, AsmOperand *operand);
void ae_free_operand(AsmOperand *operand);
bool ae_write_operand(FILE *out, const AsmOperand *operand);
long long ae_runtime_type_tag_value(const char *type_name);

/* asm_emit.c (emit utilities in main file) */
bool ae_emit_line(FILE *out, const char *format, ...);
bool ae_emit_two_operand(FILE *out, const char *mnemonic, AsmOperand *destination, AsmOperand *source);
bool ae_emit_setcc(FILE *out, const char *mnemonic, AsmOperand *destination);
bool ae_emit_shift(FILE *out, const char *mnemonic, AsmOperand *destination, AsmOperand *source);
bool ae_emit_div(FILE *out, const char *mnemonic, AsmOperand *source);

/* asm_emit_instr.c */
bool ae_emit_machine_instruction(AsmEmitContext *context, FILE *out, const MachineUnit *unit, const AsmUnitLayout *layout, size_t unit_index, size_t block_index, size_t instruction_index, const char *instruction_text);

/* asm_emit_instr_aarch64.c */
bool ae_emit_machine_instruction_aarch64(AsmEmitContext *context, FILE *out, const MachineUnit *unit, const AsmUnitLayout *layout, size_t unit_index, size_t block_index, size_t instruction_index, const char *instruction_text);
AsmUnitLayout ae_compute_unit_layout_aarch64(const MachineUnit *unit);
bool ae_translate_operand_aarch64(AsmEmitContext *context, const MachineUnit *unit, const AsmUnitLayout *layout, const char *operand_text, bool destination, AsmOperand *operand);

/* asm_emit_entry_aarch64.c */
bool ae_emit_program_entry_glue_aarch64(AsmEmitContext *context, FILE *out);

/* asm_emit_sections.c */
bool ae_emit_unit_text(AsmEmitContext *context, FILE *out, size_t unit_index, const MachineUnit *unit);
bool ae_emit_rodata(FILE *out, const AsmEmitContext *context);
bool ae_emit_data(FILE *out, const AsmEmitContext *context);

/* asm_emit_entry.c */
bool ae_emit_program_entry_glue(AsmEmitContext *context, FILE *out);

#endif
