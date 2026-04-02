#include "asm_emit.h"

#include "runtime.h"

#include <ctype.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static bool reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size);
static char *copy_text(const char *text);
static char *copy_text_n(const char *text, size_t length);
static char *copy_format(const char *format, ...);
static char *sanitize_symbol(const char *name);
static bool is_register_name(const char *text);
static bool starts_with(const char *text, const char *prefix);
static bool ends_with(const char *text, const char *suffix);
static char *trim_copy(const char *text);
static bool split_binary_operands(const char *text, char **left, char **right);
static char *between_parens(const char *text, const char *prefix);
static bool decode_quoted_text(const char *quoted, char **decoded, size_t *decoded_length);

static AsmUnitSymbol *ensure_unit_symbol(AsmEmitContext *context, const char *name);
static AsmGlobalSymbol *ensure_global_symbol(AsmEmitContext *context,
                                             const char *name,
                                             bool has_store);
static const MachineUnit *find_program_unit(const AsmEmitContext *context, const char *name);
static AsmByteLiteral *ensure_byte_literal(AsmEmitContext *context,
                                           const char *text,
                                           size_t length,
                                           const char *prefix);
static AsmStringObjectLiteral *ensure_string_literal(AsmEmitContext *context,
                                                     const char *text,
                                                     size_t length);
static char *closure_wrapper_symbol_name(const char *unit_name);

static const MachineFrameSlot *find_frame_slot(const MachineUnit *unit, const char *name);
static AsmUnitLayout compute_unit_layout(const MachineUnit *unit);
static size_t frame_slot_offset(const AsmUnitLayout *layout, size_t slot_index);
static size_t spill_slot_offset(const AsmUnitLayout *layout,
                                const MachineUnit *unit,
                                size_t spill_index);
static size_t helper_slot_offset(const AsmUnitLayout *layout,
                                 const MachineUnit *unit,
                                 size_t helper_index);
static size_t call_preserve_offset(const AsmUnitLayout *layout,
                                   const MachineUnit *unit,
                                   size_t preserve_index);

static bool translate_operand(AsmEmitContext *context,
                              const MachineUnit *unit,
                              const AsmUnitLayout *layout,
                              size_t block_index,
                              size_t instruction_index,
                              const char *operand_text,
                              bool destination,
                              AsmOperand *operand);
static void free_operand(AsmOperand *operand);
static bool write_operand(FILE *out, const AsmOperand *operand);
static bool emit_line(FILE *out, const char *format, ...);
static bool emit_two_operand(FILE *out,
                             const char *mnemonic,
                             AsmOperand *destination,
                             AsmOperand *source);
static bool emit_setcc(FILE *out, const char *mnemonic, AsmOperand *destination);
static bool emit_shift(FILE *out,
                       const char *mnemonic,
                       AsmOperand *destination,
                       AsmOperand *source);
static bool emit_div(FILE *out, const char *mnemonic, AsmOperand *source);
static bool emit_machine_instruction(AsmEmitContext *context,
                                     FILE *out,
                                     const MachineUnit *unit,
                                     const AsmUnitLayout *layout,
                                     size_t unit_index,
                                     size_t block_index,
                                     size_t instruction_index,
                                     const char *instruction_text);
static bool emit_unit_text(AsmEmitContext *context,
                           FILE *out,
                           size_t unit_index,
                           const MachineUnit *unit);
static bool emit_program_entry_glue(AsmEmitContext *context, FILE *out);
static bool emit_rodata(FILE *out, const AsmEmitContext *context);
static bool emit_data(FILE *out, const AsmEmitContext *context);
static long long runtime_type_tag_value(const char *type_name);

bool asm_emit_program(FILE *out, const MachineProgram *program) {
    AsmEmitContext context;
    size_t unit_index;

    if (!out || !program) {
        return false;
    }

    memset(&context, 0, sizeof(context));
    context.program = program;

    if (fputs(".intel_syntax noprefix\n.text\n", out) == EOF) {
        return false;
    }
    for (unit_index = 0; unit_index < program->unit_count; unit_index++) {
        if (!emit_unit_text(&context, out, unit_index, &program->units[unit_index])) {
            return false;
        }
    }
    if (!emit_program_entry_glue(&context, out)) {
        return false;
    }
    if (!emit_rodata(out, &context) || !emit_data(out, &context) ||
        !emit_line(out, ".section .note.GNU-stack,\"\",@progbits\n")) {
        return false;
    }

    return !ferror(out);
}

char *asm_emit_program_to_string(const MachineProgram *program) {
    FILE *stream;
    char *buffer;
    long size;
    size_t read_size;

    if (!program) {
        return NULL;
    }

    stream = tmpfile();
    if (!stream) {
        return NULL;
    }

    if (!asm_emit_program(stream, program) || fflush(stream) != 0 ||
        fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return NULL;
    }

    size = ftell(stream);
    if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(stream);
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, stream);
    fclose(stream);
    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

static bool reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size) {
    void *resized;
    size_t new_capacity;

    if (*capacity >= needed) {
        return true;
    }
    new_capacity = (*capacity == 0) ? 8 : *capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }
    resized = realloc(*items, new_capacity * item_size);
    if (!resized) {
        return false;
    }
    *items = resized;
    *capacity = new_capacity;
    return true;
}

static char *copy_text(const char *text) {
    return copy_text_n(text ? text : "", text ? strlen(text) : 0);
}

static char *copy_text_n(const char *text, size_t length) {
    char *copy = malloc(length + 1);

    if (!copy) {
        return NULL;
    }
    if (length > 0) {
        memcpy(copy, text, length);
    }
    copy[length] = '\0';
    return copy;
}

static char *copy_format(const char *format, ...) {
    va_list args;
    va_list args_copy;
    int needed;
    char *buffer;

    va_start(args, format);
    va_copy(args_copy, args);
    needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    if (needed < 0) {
        va_end(args);
        return NULL;
    }
    buffer = malloc((size_t)needed + 1);
    if (!buffer) {
        va_end(args);
        return NULL;
    }
    vsnprintf(buffer, (size_t)needed + 1, format, args);
    va_end(args);
    return buffer;
}

static char *sanitize_symbol(const char *name) {
    size_t i;
    size_t length;
    char *sanitized;

    if (!name) {
        return NULL;
    }
    length = strlen(name);
    sanitized = malloc(length + 1);
    if (!sanitized) {
        return NULL;
    }
    for (i = 0; i < length; i++) {
        sanitized[i] = (char)(isalnum((unsigned char)name[i]) ? name[i] : '_');
    }
    sanitized[length] = '\0';
    return sanitized;
}

static bool is_register_name(const char *text) {
    static const char *const registers[] = {
        "rax", "rbp", "rsp", "rdi", "rsi", "rdx", "rcx",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };
    size_t i;

    for (i = 0; i < sizeof(registers) / sizeof(registers[0]); i++) {
        if (strcmp(text, registers[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool starts_with(const char *text, const char *prefix) {
    return text && prefix && strncmp(text, prefix, strlen(prefix)) == 0;
}

static bool ends_with(const char *text, const char *suffix) {
    size_t text_length;
    size_t suffix_length;

    if (!text || !suffix) {
        return false;
    }
    text_length = strlen(text);
    suffix_length = strlen(suffix);
    if (suffix_length > text_length) {
        return false;
    }
    return strcmp(text + (text_length - suffix_length), suffix) == 0;
}

static char *trim_copy(const char *text) {
    const char *start = text;
    const char *end;

    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) {
        end--;
    }
    return copy_text_n(start, (size_t)(end - start));
}

static bool split_binary_operands(const char *text, char **left, char **right) {
    const char *comma;
    char *raw_left;

    if (!text || !left || !right) {
        return false;
    }
    comma = strchr(text, ',');
    if (!comma) {
        return false;
    }
    raw_left = copy_text_n(text, (size_t)(comma - text));
    if (!raw_left) {
        return false;
    }
    *left = trim_copy(raw_left);
    free(raw_left);
    if (!*left) {
        return false;
    }
    *right = trim_copy(comma + 1);
    if (!*right) {
        free(*left);
        *left = NULL;
        return false;
    }
    return true;
}

static char *between_parens(const char *text, const char *prefix) {
    size_t prefix_length;

    if (!text || !prefix || !starts_with(text, prefix) || !ends_with(text, ")")) {
        return NULL;
    }
    prefix_length = strlen(prefix);
    return copy_text_n(text + prefix_length, strlen(text) - prefix_length - 1);
}

static bool decode_quoted_text(const char *quoted, char **decoded, size_t *decoded_length) {
    char *buffer;
    size_t i;
    size_t length = 0;

    if (!quoted || !decoded || !decoded_length) {
        return false;
    }
    if (quoted[0] != '"' || strlen(quoted) < 2 || quoted[strlen(quoted) - 1] != '"') {
        return false;
    }

    buffer = malloc(strlen(quoted));
    if (!buffer) {
        return false;
    }
    for (i = 1; i + 1 < strlen(quoted); i++) {
        if (quoted[i] == '\\' && i + 2 < strlen(quoted)) {
            i++;
            switch (quoted[i]) {
            case 'n':
                buffer[length++] = '\n';
                break;
            case 'r':
                buffer[length++] = '\r';
                break;
            case 't':
                buffer[length++] = '\t';
                break;
            case '\\':
                buffer[length++] = '\\';
                break;
            case '"':
                buffer[length++] = '"';
                break;
            case '0':
                buffer[length++] = '\0';
                break;
            default:
                buffer[length++] = quoted[i];
                break;
            }
        } else {
            buffer[length++] = quoted[i];
        }
    }
    buffer[length] = '\0';
    *decoded = buffer;
    *decoded_length = length;
    return true;
}

static AsmUnitSymbol *ensure_unit_symbol(AsmEmitContext *context, const char *name) {
    size_t i;
    char *sanitized;

    for (i = 0; i < context->unit_symbol_count; i++) {
        if (strcmp(context->unit_symbols[i].name, name) == 0) {
            return &context->unit_symbols[i];
        }
    }
    if (!reserve_items((void **)&context->unit_symbols,
                       &context->unit_symbol_capacity,
                       context->unit_symbol_count + 1,
                       sizeof(*context->unit_symbols))) {
        return NULL;
    }
    sanitized = sanitize_symbol(name);
    if (!sanitized) {
        return NULL;
    }
    context->unit_symbols[context->unit_symbol_count].name = copy_text(name);
    context->unit_symbols[context->unit_symbol_count].symbol = copy_format("calynda_unit_%s", sanitized);
    free(sanitized);
    if (!context->unit_symbols[context->unit_symbol_count].name ||
        !context->unit_symbols[context->unit_symbol_count].symbol) {
        return NULL;
    }
    context->unit_symbol_count++;
    return &context->unit_symbols[context->unit_symbol_count - 1];
}

static AsmGlobalSymbol *ensure_global_symbol(AsmEmitContext *context,
                                             const char *name,
                                             bool has_store) {
    size_t i;
    char *sanitized;

    for (i = 0; i < context->global_symbol_count; i++) {
        if (strcmp(context->global_symbols[i].name, name) == 0) {
            context->global_symbols[i].has_store = context->global_symbols[i].has_store || has_store;
            return &context->global_symbols[i];
        }
    }
    if (!reserve_items((void **)&context->global_symbols,
                       &context->global_symbol_capacity,
                       context->global_symbol_count + 1,
                       sizeof(*context->global_symbols))) {
        return NULL;
    }
    sanitized = sanitize_symbol(name);
    if (!sanitized) {
        return NULL;
    }
    context->global_symbols[context->global_symbol_count].name = copy_text(name);
    context->global_symbols[context->global_symbol_count].symbol = copy_format("calynda_global_%s", sanitized);
    context->global_symbols[context->global_symbol_count].has_store = has_store;
    free(sanitized);
    if (!context->global_symbols[context->global_symbol_count].name ||
        !context->global_symbols[context->global_symbol_count].symbol) {
        return NULL;
    }
    context->global_symbol_count++;
    return &context->global_symbols[context->global_symbol_count - 1];
}

static const MachineUnit *find_program_unit(const AsmEmitContext *context, const char *name) {
    size_t i;

    if (!context || !context->program || !name) {
        return NULL;
    }

    for (i = 0; i < context->program->unit_count; i++) {
        if (strcmp(context->program->units[i].name, name) == 0) {
            return &context->program->units[i];
        }
    }

    return NULL;
}

static AsmByteLiteral *ensure_byte_literal(AsmEmitContext *context,
                                           const char *text,
                                           size_t length,
                                           const char *prefix) {
    size_t i;

    for (i = 0; i < context->byte_literal_count; i++) {
        if (context->byte_literals[i].length == length &&
            memcmp(context->byte_literals[i].text, text, length) == 0) {
            return &context->byte_literals[i];
        }
    }
    if (!reserve_items((void **)&context->byte_literals,
                       &context->byte_literal_capacity,
                       context->byte_literal_count + 1,
                       sizeof(*context->byte_literals))) {
        return NULL;
    }
    context->byte_literals[context->byte_literal_count].text = copy_text_n(text, length);
    context->byte_literals[context->byte_literal_count].length = length;
    context->byte_literals[context->byte_literal_count].label = copy_format(".L%s_%zu", prefix, context->next_label_id++);
    if (!context->byte_literals[context->byte_literal_count].text ||
        !context->byte_literals[context->byte_literal_count].label) {
        return NULL;
    }
    context->byte_literal_count++;
    return &context->byte_literals[context->byte_literal_count - 1];
}

static AsmStringObjectLiteral *ensure_string_literal(AsmEmitContext *context,
                                                     const char *text,
                                                     size_t length) {
    size_t i;

    for (i = 0; i < context->string_literal_count; i++) {
        if (context->string_literals[i].length == length &&
            memcmp(context->string_literals[i].text, text, length) == 0) {
            return &context->string_literals[i];
        }
    }
    if (!reserve_items((void **)&context->string_literals,
                       &context->string_literal_capacity,
                       context->string_literal_count + 1,
                       sizeof(*context->string_literals))) {
        return NULL;
    }
    context->string_literals[context->string_literal_count].text = copy_text_n(text, length);
    context->string_literals[context->string_literal_count].length = length;
    context->string_literals[context->string_literal_count].bytes_label = copy_format(".Lstr_bytes_%zu", context->next_label_id);
    context->string_literals[context->string_literal_count].object_label = copy_format(".Lstr_obj_%zu", context->next_label_id++);
    if (!context->string_literals[context->string_literal_count].text ||
        !context->string_literals[context->string_literal_count].bytes_label ||
        !context->string_literals[context->string_literal_count].object_label) {
        return NULL;
    }
    context->string_literal_count++;
    return &context->string_literals[context->string_literal_count - 1];
}

static char *closure_wrapper_symbol_name(const char *unit_name) {
    char *sanitized;
    char *symbol;

    if (!unit_name) {
        return NULL;
    }

    sanitized = sanitize_symbol(unit_name);
    if (!sanitized) {
        return NULL;
    }

    symbol = copy_format("calynda_closure_%s", sanitized);
    free(sanitized);
    return symbol;
}

static const MachineFrameSlot *find_frame_slot(const MachineUnit *unit, const char *name) {
    size_t i;

    if (!unit || !name) {
        return NULL;
    }
    for (i = 0; i < unit->frame_slot_count; i++) {
        if (strcmp(unit->frame_slots[i].name, name) == 0) {
            return &unit->frame_slots[i];
        }
    }
    return NULL;
}

static AsmUnitLayout compute_unit_layout(const MachineUnit *unit) {
    AsmUnitLayout layout;
    size_t block_index;
    size_t instruction_index;
    size_t named_local_words;

    memset(&layout, 0, sizeof(layout));
    layout.frame_words = unit->frame_slot_count;
    layout.spill_words = unit->spill_slot_count;
    layout.helper_words = unit->helper_slot_count;
    layout.outgoing_words = unit->outgoing_stack_slot_count;

    if (unit->block_count > 0) {
        const MachineBlock *entry_block = &unit->blocks[0];

        for (instruction_index = 0; instruction_index < entry_block->instruction_count; instruction_index++) {
            if (strcmp(entry_block->instructions[instruction_index].text, "push r12") == 0) {
                layout.saves_r12 = true;
            } else if (strcmp(entry_block->instructions[instruction_index].text, "push r13") == 0) {
                layout.saves_r13 = true;
            }
        }
    }

    for (block_index = 0; block_index < unit->block_count; block_index++) {
        for (instruction_index = 0;
             instruction_index < unit->blocks[block_index].instruction_count;
             instruction_index++) {
            const char *text = unit->blocks[block_index].instructions[instruction_index].text;

            if (starts_with(text, "call ")) {
                layout.has_calls = true;
            }
            if (strcmp(text, "push r10") == 0 || strcmp(text, "pop r10") == 0) {
                layout.preserves_r10 = true;
            }
            if (strcmp(text, "push r11") == 0 || strcmp(text, "pop r11") == 0) {
                layout.preserves_r11 = true;
            }
        }
    }

    layout.saved_reg_words = 1 + (layout.saves_r12 ? 1 : 0) + (layout.saves_r13 ? 1 : 0);
    layout.call_preserve_words = (layout.preserves_r10 ? 1 : 0) + (layout.preserves_r11 ? 1 : 0);
    named_local_words = layout.frame_words + layout.spill_words + layout.helper_words + layout.call_preserve_words + layout.outgoing_words;
    if (layout.has_calls && ((layout.saved_reg_words + named_local_words) % 2) != 0) {
        layout.alignment_pad_words = 1;
    }
    layout.total_local_words = named_local_words + layout.alignment_pad_words;
    return layout;
}

static size_t frame_slot_offset(const AsmUnitLayout *layout, size_t slot_index) {
    return (layout->saved_reg_words + slot_index + 1) * 8;
}

static size_t spill_slot_offset(const AsmUnitLayout *layout,
                                const MachineUnit *unit,
                                size_t spill_index) {
    return (layout->saved_reg_words + unit->frame_slot_count + spill_index + 1) * 8;
}

static size_t helper_slot_offset(const AsmUnitLayout *layout,
                                 const MachineUnit *unit,
                                 size_t helper_index) {
    return (layout->saved_reg_words + unit->frame_slot_count + unit->spill_slot_count +
            (unit->helper_slot_count - helper_index)) * 8;
}

static size_t call_preserve_offset(const AsmUnitLayout *layout,
                                   const MachineUnit *unit,
                                   size_t preserve_index) {
    return (layout->saved_reg_words + unit->frame_slot_count + unit->spill_slot_count +
            unit->helper_slot_count + preserve_index + 1) * 8;
}

static bool translate_operand(AsmEmitContext *context,
                              const MachineUnit *unit,
                              const AsmUnitLayout *layout,
                              size_t block_index,
                              size_t instruction_index,
                              const char *operand_text,
                              bool destination,
                              AsmOperand *operand) {
    char *inner = NULL;
    char *decoded = NULL;
    size_t decoded_length = 0;

    (void)block_index;
    (void)instruction_index;
    (void)destination;

    if (!operand || !operand_text) {
        return false;
    }
    memset(operand, 0, sizeof(*operand));

    if (is_register_name(operand_text)) {
        operand->kind = ASM_OPERAND_REGISTER;
        operand->text = copy_text(operand_text);
        return operand->text != NULL;
    }
    if ((operand_text[0] == '-' && isdigit((unsigned char)operand_text[1])) ||
        isdigit((unsigned char)operand_text[0])) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = copy_text(operand_text);
        return operand->text != NULL;
    }
    if (strcmp(operand_text, "null") == 0) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = copy_text("0");
        return operand->text != NULL;
    }
    if (strcmp(operand_text, "bool(true)") == 0 || strcmp(operand_text, "bool(false)") == 0) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = copy_text(strcmp(operand_text, "bool(true)") == 0 ? "1" : "0");
        return operand->text != NULL;
    }

    inner = between_parens(operand_text, "frame(");
    if (inner) {
        const MachineFrameSlot *slot = find_frame_slot(unit, inner);
        free(inner);
        if (!slot) {
            return false;
        }
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = copy_format("QWORD PTR [rbp - %zu]", frame_slot_offset(layout, slot->index));
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "spill(");
    if (inner) {
        size_t spill_index = (size_t)strtoull(inner, NULL, 10);
        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = copy_format("QWORD PTR [rbp - %zu]", spill_slot_offset(layout, unit, spill_index));
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "helper(");
    if (inner) {
        size_t helper_index = (size_t)strtoull(inner, NULL, 10);
        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = copy_format("QWORD PTR [rbp - %zu]", helper_slot_offset(layout, unit, helper_index));
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "argin(");
    if (inner) {
        size_t argument_index = (size_t)strtoull(inner, NULL, 10);
        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = copy_format("QWORD PTR [rbp + %zu]", 16 + (argument_index * 8));
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "argout(");
    if (inner) {
        size_t argument_index = (size_t)strtoull(inner, NULL, 10);
        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = copy_format("QWORD PTR [rsp + %zu]", argument_index * 8);
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "env(");
    if (inner) {
        size_t capture_index = (size_t)strtoull(inner, NULL, 10);
        free(inner);
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = copy_format("QWORD PTR [r15 + %zu]", capture_index * 8);
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "global(");
    if (inner) {
        AsmGlobalSymbol *symbol = ensure_global_symbol(context, inner, destination);
        free(inner);
        if (!symbol) {
            return false;
        }
        operand->kind = ASM_OPERAND_MEMORY;
        operand->text = copy_format("QWORD PTR [rip + %s]", symbol->symbol);
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "code(");
    if (inner) {
        const MachineUnit *referenced_unit = find_program_unit(context, inner);

        if (referenced_unit && referenced_unit->kind == LIR_UNIT_LAMBDA) {
            operand->kind = ASM_OPERAND_ADDRESS;
            operand->text = closure_wrapper_symbol_name(inner);
            free(inner);
            return operand->text != NULL;
        }

        {
            AsmUnitSymbol *symbol = ensure_unit_symbol(context, inner);

            free(inner);
            if (!symbol) {
                return false;
            }
            operand->kind = ASM_OPERAND_ADDRESS;
            operand->text = copy_text(symbol->symbol);
            return operand->text != NULL;
        }
    }
    inner = between_parens(operand_text, "symbol(");
    if (inner) {
        AsmByteLiteral *literal = ensure_byte_literal(context, inner, strlen(inner), "sym");
        free(inner);
        if (!literal) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = copy_text(literal->label);
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "text(");
    if (inner) {
        AsmByteLiteral *literal;

        if (!decode_quoted_text(inner, &decoded, &decoded_length)) {
            free(inner);
            return false;
        }
        literal = ensure_byte_literal(context, decoded, decoded_length, "txt");
        free(decoded);
        free(inner);
        if (!literal) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = copy_text(literal->label);
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "string(");
    if (inner) {
        AsmStringObjectLiteral *literal;

        if (!decode_quoted_text(inner, &decoded, &decoded_length)) {
            free(inner);
            return false;
        }
        literal = ensure_string_literal(context, decoded, decoded_length);
        free(decoded);
        free(inner);
        if (!literal) {
            return false;
        }
        operand->kind = ASM_OPERAND_ADDRESS;
        operand->text = copy_text(literal->object_label);
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "tag(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = copy_text(strcmp(inner, "text") == 0 ? "0" : "1");
        free(inner);
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "type(");
    if (inner) {
        long long type_tag = runtime_type_tag_value(inner);

        free(inner);
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = copy_format("%lld", type_tag);
        return operand->text != NULL;
    }
    inner = between_parens(operand_text, "int32(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = inner;
        return true;
    }
    inner = between_parens(operand_text, "int64(");
    if (inner) {
        operand->kind = ASM_OPERAND_IMMEDIATE;
        operand->text = inner;
        return true;
    }

    if (starts_with(operand_text, "bb")) {
        AsmUnitSymbol *symbol = ensure_unit_symbol(context, unit->name);

        if (!symbol) {
            return false;
        }
        operand->kind = ASM_OPERAND_BRANCH_LABEL;
        operand->text = copy_format(".L%s_%s", symbol->symbol, operand_text);
        return operand->text != NULL;
    }

    operand->kind = ASM_OPERAND_CALL_TARGET;
    if (starts_with(operand_text, "__calynda_rt_")) {
        operand->text = copy_text(operand_text);
    } else {
        AsmUnitSymbol *symbol = ensure_unit_symbol(context, operand_text);

        if (!symbol) {
            return false;
        }
        operand->text = copy_text(symbol->symbol);
    }
    return operand->text != NULL;
}

static void free_operand(AsmOperand *operand) {
    if (!operand) {
        return;
    }
    free(operand->text);
    memset(operand, 0, sizeof(*operand));
}

static bool write_operand(FILE *out, const AsmOperand *operand) {
    if (!out || !operand || !operand->text) {
        return false;
    }
    switch (operand->kind) {
    case ASM_OPERAND_IMMEDIATE:
        return fprintf(out, "%s", operand->text) >= 0;
    case ASM_OPERAND_ADDRESS:
        return fprintf(out, "OFFSET FLAT:%s", operand->text) >= 0;
    default:
        return fprintf(out, "%s", operand->text) >= 0;
    }
}

static bool emit_line(FILE *out, const char *format, ...) {
    va_list args;
    int written;

    va_start(args, format);
    written = vfprintf(out, format, args);
    va_end(args);
    return written >= 0;
}

static bool emit_two_operand(FILE *out,
                             const char *mnemonic,
                             AsmOperand *destination,
                             AsmOperand *source) {
    if (!out || !mnemonic || !destination || !source) {
        return false;
    }

    if (strcmp(mnemonic, "mov") == 0) {
        if (destination->kind == ASM_OPERAND_REGISTER && source->kind == ASM_OPERAND_ADDRESS) {
            return emit_line(out, "    lea %s, [rip + %s]\n", destination->text, source->text);
        }
        if (destination->kind == ASM_OPERAND_MEMORY &&
            (source->kind == ASM_OPERAND_MEMORY || source->kind == ASM_OPERAND_ADDRESS)) {
            if (source->kind == ASM_OPERAND_ADDRESS) {
                return emit_line(out, "    lea rcx, [rip + %s]\n", source->text) &&
                       emit_line(out, "    mov %s, rcx\n", destination->text);
            }
            return emit_line(out, "    mov rcx, %s\n", source->text) &&
                   emit_line(out, "    mov %s, rcx\n", destination->text);
        }
    }
    if (strcmp(mnemonic, "cmp") == 0 && destination->kind == ASM_OPERAND_MEMORY &&
        source->kind == ASM_OPERAND_MEMORY) {
        return emit_line(out, "    mov rcx, %s\n", source->text) &&
               emit_line(out, "    cmp %s, rcx\n", destination->text);
    }

    if (!emit_line(out, "    %s ", mnemonic) ||
        !write_operand(out, destination) ||
        !emit_line(out, ", ") ||
        !write_operand(out, source) ||
        !emit_line(out, "\n")) {
        return false;
    }

    return true;
}

static bool emit_setcc(FILE *out, const char *mnemonic, AsmOperand *destination) {
    const char *byte_reg = NULL;

    if (!out || !mnemonic || !destination || destination->kind != ASM_OPERAND_REGISTER) {
        return false;
    }

    if (strcmp(destination->text, "rax") == 0) {
        byte_reg = "al";
    } else if (strcmp(destination->text, "rcx") == 0) {
        byte_reg = "cl";
    } else if (strcmp(destination->text, "rdx") == 0) {
        byte_reg = "dl";
    } else if (strcmp(destination->text, "rdi") == 0) {
        byte_reg = "dil";
    } else if (strcmp(destination->text, "rsi") == 0) {
        byte_reg = "sil";
    } else if (strcmp(destination->text, "r8") == 0) {
        byte_reg = "r8b";
    } else if (strcmp(destination->text, "r9") == 0) {
        byte_reg = "r9b";
    } else if (strcmp(destination->text, "r10") == 0) {
        byte_reg = "r10b";
    } else if (strcmp(destination->text, "r11") == 0) {
        byte_reg = "r11b";
    } else if (strcmp(destination->text, "r12") == 0) {
        byte_reg = "r12b";
    } else if (strcmp(destination->text, "r13") == 0) {
        byte_reg = "r13b";
    } else if (strcmp(destination->text, "r14") == 0) {
        byte_reg = "r14b";
    } else if (strcmp(destination->text, "r15") == 0) {
        byte_reg = "r15b";
    }

    if (!byte_reg) {
        return false;
    }

    return emit_line(out, "    %s %s\n", mnemonic, byte_reg) &&
           emit_line(out, "    movzx %s, %s\n", destination->text, byte_reg);
}

static bool emit_shift(FILE *out,
                       const char *mnemonic,
                       AsmOperand *destination,
                       AsmOperand *source) {
    if (!out || !mnemonic || !destination || !source) {
        return false;
    }
    if (source->kind == ASM_OPERAND_IMMEDIATE) {
        return emit_two_operand(out, mnemonic, destination, source);
    }
    return emit_line(out, "    mov rcx, ") &&
           write_operand(out, source) &&
           emit_line(out, "\n    %s %s, cl\n", mnemonic, destination->text);
}

static bool emit_div(FILE *out, const char *mnemonic, AsmOperand *source) {
    if (!out || !mnemonic || !source) {
        return false;
    }
    if (source->kind == ASM_OPERAND_IMMEDIATE || source->kind == ASM_OPERAND_ADDRESS) {
        return emit_line(out, "    mov rcx, ") &&
               write_operand(out, source) &&
               emit_line(out, "\n    %s rcx\n", mnemonic);
    }
    return emit_line(out, "    %s ", mnemonic) && write_operand(out, source) && emit_line(out, "\n");
}

static bool emit_machine_instruction(AsmEmitContext *context,
                                     FILE *out,
                                     const MachineUnit *unit,
                                     const AsmUnitLayout *layout,
                                     size_t unit_index,
                                     size_t block_index,
                                     size_t instruction_index,
                                     const char *instruction_text) {
    char *space;
    char *mnemonic = NULL;
    char *operand_text = NULL;
    char *left_text = NULL;
    char *right_text = NULL;
    AsmOperand left;
    AsmOperand right;
    bool ok = false;
    const char *lea_source;

    (void)unit_index;

    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));

    if (strcmp(instruction_text, "push rbp") == 0 ||
        strcmp(instruction_text, "mov rbp, rsp") == 0 ||
        strcmp(instruction_text, "push r14") == 0 ||
        strcmp(instruction_text, "push r12") == 0 ||
        strcmp(instruction_text, "push r13") == 0 ||
        starts_with(instruction_text, "sub rsp, ") ||
        starts_with(instruction_text, "add rsp, ") ||
        strcmp(instruction_text, "pop r14") == 0 ||
        strcmp(instruction_text, "pop r12") == 0 ||
        strcmp(instruction_text, "pop r13") == 0 ||
        strcmp(instruction_text, "pop rbp") == 0) {
        return true;
    }
    if (strcmp(instruction_text, "push r10") == 0) {
        if (!layout->preserves_r10) {
            return true;
        }
        return emit_line(out,
                         "    mov QWORD PTR [rbp - %zu], r10\n",
                         call_preserve_offset(layout, unit, 0));
    }
    if (strcmp(instruction_text, "pop r10") == 0) {
        if (!layout->preserves_r10) {
            return true;
        }
        return emit_line(out,
                         "    mov r10, QWORD PTR [rbp - %zu]\n",
                         call_preserve_offset(layout, unit, 0));
    }
    if (strcmp(instruction_text, "push r11") == 0) {
        size_t preserve_index = layout->preserves_r10 ? 1 : 0;

        if (!layout->preserves_r11) {
            return true;
        }
        return emit_line(out,
                         "    mov QWORD PTR [rbp - %zu], r11\n",
                         call_preserve_offset(layout, unit, preserve_index));
    }
    if (strcmp(instruction_text, "pop r11") == 0) {
        size_t preserve_index = layout->preserves_r10 ? 1 : 0;

        if (!layout->preserves_r11) {
            return true;
        }
        return emit_line(out,
                         "    mov r11, QWORD PTR [rbp - %zu]\n",
                         call_preserve_offset(layout, unit, preserve_index));
    }
    if (strcmp(instruction_text, "ret") == 0) {
        if (layout->total_local_words > 0 &&
            !emit_line(out, "    add rsp, %zu\n", layout->total_local_words * 8)) {
            return false;
        }
        if (layout->saves_r13 && !emit_line(out, "    pop r13\n")) {
            return false;
        }
        if (layout->saves_r12 && !emit_line(out, "    pop r12\n")) {
            return false;
        }
        return emit_line(out, "    pop r14\n    pop rbp\n    ret\n");
    }

    if (strcmp(instruction_text, "cqo") == 0) {
        return emit_line(out, "    cqo\n");
    }

    space = strchr(instruction_text, ' ');
    if (!space) {
        return false;
    }
    mnemonic = copy_text_n(instruction_text, (size_t)(space - instruction_text));
    operand_text = trim_copy(space + 1);
    if (!mnemonic || !operand_text) {
        free(mnemonic);
        free(operand_text);
        return false;
    }

    if (strcmp(mnemonic, "call") == 0 || strcmp(mnemonic, "jmp") == 0 || strcmp(mnemonic, "jne") == 0) {
        if (!translate_operand(context,
                               unit,
                               layout,
                               block_index,
                               instruction_index,
                               operand_text,
                               false,
                               &left)) {
            free(mnemonic);
            free(operand_text);
            return false;
        }
        if (strcmp(mnemonic, "call") == 0) {
            bool ok = emit_line(out, "    call %s\n", left.text);

            free_operand(&left);
            free(mnemonic);
            free(operand_text);
            return ok;
        }
        {
            bool ok = emit_line(out, "    %s %s\n", mnemonic, left.text);

            free_operand(&left);
            free(mnemonic);
            free(operand_text);
            return ok;
        }
    }

    if (!strchr(operand_text, ',')) {
        if (!translate_operand(context,
                               unit,
                               layout,
                               block_index,
                               instruction_index,
                               operand_text,
                               true,
                               &left)) {
            free(mnemonic);
            free(operand_text);
            return false;
        }
        if (starts_with(mnemonic, "set")) {
            bool ok = emit_setcc(out, mnemonic, &left);
            free_operand(&left);
            free(mnemonic);
            free(operand_text);
            return ok;
        }
        if (strcmp(mnemonic, "push") == 0 || strcmp(mnemonic, "pop") == 0 ||
            strcmp(mnemonic, "neg") == 0 || strcmp(mnemonic, "not") == 0) {
            bool ok = emit_line(out, "    %s %s\n", mnemonic, left.text);
            free_operand(&left);
            free(mnemonic);
            free(operand_text);
            return ok;
        }
        if (strcmp(mnemonic, "div") == 0 || strcmp(mnemonic, "idiv") == 0) {
            bool ok = emit_div(out, mnemonic, &left);
            free_operand(&left);
            free(mnemonic);
            free(operand_text);
            return ok;
        }
        free_operand(&left);
        free(mnemonic);
        free(operand_text);
        return false;
    }

    if (!split_binary_operands(operand_text, &left_text, &right_text) ||
        !translate_operand(context,
                           unit,
                           layout,
                           block_index,
                           instruction_index,
                           left_text,
                           true,
                           &left) ||
        !translate_operand(context,
                           unit,
                           layout,
                           block_index,
                           instruction_index,
                           right_text,
                           false,
                           &right)) {
        free(mnemonic);
        free(operand_text);
        free(left_text);
        free(right_text);
        free_operand(&left);
        free_operand(&right);
        return false;
    }
    free(operand_text);
    free(left_text);
    free(right_text);

    if (strcmp(mnemonic, "lea") == 0) {
        lea_source = starts_with(right.text, "QWORD PTR ") ? right.text + strlen("QWORD PTR ") : right.text;
        ok = emit_line(out, "    lea %s, %s\n", left.text, lea_source);
        free_operand(&left);
        free_operand(&right);
        free(mnemonic);
        return ok;
    }
    if (strcmp(mnemonic, "shl") == 0 || strcmp(mnemonic, "shr") == 0 || strcmp(mnemonic, "sar") == 0) {
        bool ok = emit_shift(out, mnemonic, &left, &right);
        free_operand(&left);
        free_operand(&right);
        free(mnemonic);
        return ok;
    }

    {
        bool ok = emit_two_operand(out, mnemonic, &left, &right);

        free_operand(&left);
        free_operand(&right);
        free(mnemonic);
        return ok;
    }
}

static bool emit_unit_text(AsmEmitContext *context,
                           FILE *out,
                           size_t unit_index,
                           const MachineUnit *unit) {
    AsmUnitSymbol *symbol;
    AsmUnitLayout layout;
    size_t block_index;

    symbol = ensure_unit_symbol(context, unit->name);
    if (!symbol) {
        return false;
    }
    layout = compute_unit_layout(unit);

    if (!emit_line(out, ".globl %s\n%s:\n", symbol->symbol, symbol->symbol) ||
        !emit_line(out, "    push rbp\n    mov rbp, rsp\n    push r14\n")) {
        return false;
    }
    if (layout.saves_r12 && !emit_line(out, "    push r12\n")) {
        return false;
    }
    if (layout.saves_r13 && !emit_line(out, "    push r13\n")) {
        return false;
    }
    if (layout.total_local_words > 0 &&
        !emit_line(out, "    sub rsp, %zu\n", layout.total_local_words * 8)) {
        return false;
    }

    for (block_index = 0; block_index < unit->block_count; block_index++) {
        size_t instruction_index;

        if (block_index > 0 &&
            !emit_line(out, ".L%s_%s:\n", symbol->symbol, unit->blocks[block_index].label)) {
            return false;
        }
        for (instruction_index = 0;
             instruction_index < unit->blocks[block_index].instruction_count;
             instruction_index++) {
            if (!emit_machine_instruction(context,
                                          out,
                                          unit,
                                          &layout,
                                          unit_index,
                                          block_index,
                                          instruction_index,
                                          unit->blocks[block_index].instructions[instruction_index].text)) {
                return false;
            }
        }
    }

    return true;
}

static bool emit_program_entry_glue(AsmEmitContext *context, FILE *out) {
    static const char *const argument_registers[] = {
        "rdi", "rsi", "rdx", "rcx", "r8", "r9"
    };
    const MachineUnit *start_unit = NULL;
    AsmUnitSymbol *start_symbol = NULL;
    size_t string_index;
    size_t unit_index;

    if (!context || !context->program || !out) {
        return false;
    }

    for (unit_index = 0; unit_index < context->program->unit_count; unit_index++) {
        const MachineUnit *unit = &context->program->units[unit_index];

        if (unit->kind == LIR_UNIT_START) {
            start_unit = unit;
            break;
        }
    }

    if (!start_unit) {
        return true;
    }

    start_symbol = ensure_unit_symbol(context, start_unit->name);
    if (!start_symbol) {
        return false;
    }

    if (!emit_line(out,
                   ".globl calynda_program_start\n"
                   "calynda_program_start:\n"
                   "    push rbp\n"
                   "    mov rbp, rsp\n"
                   "    push r15\n"
                   "    sub rsp, 8\n"
                   "    xor r15, r15\n")) {
        return false;
    }

    for (string_index = 0; string_index < context->string_literal_count; string_index++) {
        if (!emit_line(out,
                       "    mov rdi, OFFSET FLAT:%s\n"
                       "    call calynda_rt_register_static_object\n",
                       context->string_literals[string_index].object_label)) {
            return false;
        }
    }

    if (!emit_line(out,
                   "    call %s\n"
                   "    add rsp, 8\n"
                   "    pop r15\n"
                   "    pop rbp\n"
                   "    ret\n",
                   start_symbol->symbol) ||
        !emit_line(out,
                   ".globl main\n"
                   "main:\n"
                   "    push rbp\n"
                   "    mov rbp, rsp\n"
                   "    mov rdx, rsi\n"
                   "    mov esi, edi\n"
                   "    mov rdi, OFFSET FLAT:calynda_program_start\n"
                   "    call calynda_rt_start_process\n"
                   "    pop rbp\n"
                   "    ret\n")) {
        return false;
    }

    for (unit_index = 0; unit_index < context->program->unit_count; unit_index++) {
        const MachineUnit *unit = &context->program->units[unit_index];
        AsmUnitSymbol *unit_symbol;
        char *wrapper_symbol;
        size_t extra_argument_count;
        size_t stack_pad;
        size_t register_argument_count;
        size_t argument_index;
        size_t cleanup_bytes;

        if (unit->kind != LIR_UNIT_LAMBDA) {
            continue;
        }

        unit_symbol = ensure_unit_symbol(context, unit->name);
        wrapper_symbol = closure_wrapper_symbol_name(unit->name);
        if (!unit_symbol || !wrapper_symbol) {
            free(wrapper_symbol);
            return false;
        }

        extra_argument_count = unit->parameter_count > 6 ? unit->parameter_count - 6 : 0;
        stack_pad = (extra_argument_count % 2 == 0) ? 8 : 0;
        register_argument_count = unit->parameter_count < 6 ? unit->parameter_count : 6;

        if (!emit_line(out,
                       ".globl %s\n"
                       "%s:\n"
                       "    push rbp\n"
                       "    mov rbp, rsp\n"
                       "    push r15\n"
                       "    mov r15, rdi\n"
                       "    mov r11, rdx\n",
                       wrapper_symbol,
                       wrapper_symbol)) {
            free(wrapper_symbol);
            return false;
        }

        if (stack_pad > 0 && !emit_line(out, "    sub rsp, %zu\n", stack_pad)) {
            free(wrapper_symbol);
            return false;
        }

        for (argument_index = unit->parameter_count; argument_index > 6; argument_index--) {
            if (!emit_line(out,
                           "    push QWORD PTR [r11 + %zu]\n",
                           (argument_index - 1) * 8)) {
                free(wrapper_symbol);
                return false;
            }
        }

        for (argument_index = 0; argument_index < register_argument_count; argument_index++) {
            if (!emit_line(out,
                           "    mov %s, QWORD PTR [r11 + %zu]\n",
                           argument_registers[argument_index],
                           argument_index * 8)) {
                free(wrapper_symbol);
                return false;
            }
        }

        if (!emit_line(out, "    call %s\n", unit_symbol->symbol)) {
            free(wrapper_symbol);
            return false;
        }

        cleanup_bytes = stack_pad + (extra_argument_count * 8);
        if (cleanup_bytes > 0 && !emit_line(out, "    add rsp, %zu\n", cleanup_bytes)) {
            free(wrapper_symbol);
            return false;
        }

        if (!emit_line(out,
                       "    pop r15\n"
                       "    pop rbp\n"
                       "    ret\n")) {
            free(wrapper_symbol);
            return false;
        }

        free(wrapper_symbol);
    }

    return true;
}

static bool emit_rodata(FILE *out, const AsmEmitContext *context) {
    size_t i;
    size_t j;

    if (!context || (context->byte_literal_count == 0 && context->string_literal_count == 0)) {
        return true;
    }
    if (!emit_line(out, ".section .rodata\n")) {
        return false;
    }
    for (i = 0; i < context->byte_literal_count; i++) {
        if (!emit_line(out, "%s:\n", context->byte_literals[i].label)) {
            return false;
        }
        if (context->byte_literals[i].length == 0) {
            if (!emit_line(out, "    .byte 0\n")) {
                return false;
            }
        } else {
            if (!emit_line(out, "    .byte ")) {
                return false;
            }
            for (j = 0; j < context->byte_literals[i].length; j++) {
                if (j > 0 && !emit_line(out, ", ")) {
                    return false;
                }
                if (!emit_line(out, "%u", (unsigned int)(unsigned char)context->byte_literals[i].text[j])) {
                    return false;
                }
            }
            if (!emit_line(out, ", 0\n")) {
                return false;
            }
        }
    }
    return true;
}

static bool emit_data(FILE *out, const AsmEmitContext *context) {
    size_t i;

    if (!context) {
        return false;
    }
    if (context->global_symbol_count == 0 && context->string_literal_count == 0) {
        return true;
    }
    if (!emit_line(out, ".data\n")) {
        return false;
    }
    for (i = 0; i < context->string_literal_count; i++) {
        if (!emit_line(out,
                       "%s:\n"
                       "    .long %u\n"
                       "    .long %u\n"
                       "    .quad %zu\n"
                       "    .quad %s\n",
                       context->string_literals[i].object_label,
                       CALYNDA_RT_OBJECT_MAGIC,
                       CALYNDA_RT_OBJECT_STRING,
                       context->string_literals[i].length,
                       context->string_literals[i].bytes_label)) {
            return false;
        }
        if (!emit_line(out, "%s:\n", context->string_literals[i].bytes_label)) {
            return false;
        }
        if (context->string_literals[i].length == 0) {
            if (!emit_line(out, "    .byte 0\n")) {
                return false;
            }
        } else {
            size_t j;

            if (!emit_line(out, "    .byte ")) {
                return false;
            }
            for (j = 0; j < context->string_literals[i].length; j++) {
                if (j > 0 && !emit_line(out, ", ")) {
                    return false;
                }
                if (!emit_line(out,
                               "%u",
                               (unsigned int)(unsigned char)context->string_literals[i].text[j])) {
                    return false;
                }
            }
            if (!emit_line(out, ", 0\n")) {
                return false;
            }
        }
    }
    for (i = 0; i < context->global_symbol_count; i++) {
        char *sanitized = sanitize_symbol(context->global_symbols[i].name);

        if (!sanitized) {
            return false;
        }
        if (!emit_line(out, ".globl %s\n%s:\n", context->global_symbols[i].symbol, context->global_symbols[i].symbol)) {
            free(sanitized);
            return false;
        }
        if (context->global_symbols[i].has_store) {
            if (!emit_line(out, "    .quad 0\n")) {
                free(sanitized);
                return false;
            }
        } else {
            if (!emit_line(out, "    .quad __calynda_pkg_%s\n", sanitized)) {
                free(sanitized);
                return false;
            }
        }
        free(sanitized);
    }
    return true;
}

static long long runtime_type_tag_value(const char *type_name) {
    if (strcmp(type_name, "bool") == 0) {
        return CALYNDA_RT_TYPE_BOOL;
    }
    if (strcmp(type_name, "int32") == 0) {
        return CALYNDA_RT_TYPE_INT32;
    }
    if (strcmp(type_name, "int64") == 0) {
        return CALYNDA_RT_TYPE_INT64;
    }
    if (strcmp(type_name, "string") == 0) {
        return CALYNDA_RT_TYPE_STRING;
    }
    if (strcmp(type_name, "string[]") == 0 || strstr(type_name, "[]") != NULL) {
        return CALYNDA_RT_TYPE_ARRAY;
    }
    return CALYNDA_RT_TYPE_RAW_WORD;
}