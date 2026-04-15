#include "asm_emit_internal.h"
#include <stdlib.h>
#include <string.h>

AsmUnitSymbol *ae_ensure_unit_symbol(AsmEmitContext *context, const char *name) {
    size_t i;
    char *sanitized;

    for (i = 0; i < context->unit_symbol_count; i++) {
        if (strcmp(context->unit_symbols[i].name, name) == 0) {
            return &context->unit_symbols[i];
        }
    }
    if (!ae_reserve_items((void **)&context->unit_symbols,
                       &context->unit_symbol_capacity,
                       context->unit_symbol_count + 1,
                       sizeof(*context->unit_symbols))) {
        return NULL;
    }
    sanitized = ae_sanitize_symbol(name);
    if (!sanitized) {
        return NULL;
    }
    context->unit_symbols[context->unit_symbol_count].name = ae_copy_text(name);
    context->unit_symbols[context->unit_symbol_count].symbol = ae_copy_format("calynda_unit_%s", sanitized);
    free(sanitized);
    if (!context->unit_symbols[context->unit_symbol_count].name ||
        !context->unit_symbols[context->unit_symbol_count].symbol) {
        return NULL;
    }
    context->unit_symbol_count++;
    return &context->unit_symbols[context->unit_symbol_count - 1];
}

AsmGlobalSymbol *ae_ensure_global_symbol(AsmEmitContext *context,
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
    if (!ae_reserve_items((void **)&context->global_symbols,
                       &context->global_symbol_capacity,
                       context->global_symbol_count + 1,
                       sizeof(*context->global_symbols))) {
        return NULL;
    }
    sanitized = ae_sanitize_symbol(name);
    if (!sanitized) {
        return NULL;
    }
    context->global_symbols[context->global_symbol_count].name = ae_copy_text(name);
    context->global_symbols[context->global_symbol_count].symbol = ae_copy_format("calynda_global_%s", sanitized);
    context->global_symbols[context->global_symbol_count].has_store = has_store;
    free(sanitized);
    if (!context->global_symbols[context->global_symbol_count].name ||
        !context->global_symbols[context->global_symbol_count].symbol) {
        return NULL;
    }
    context->global_symbol_count++;
    return &context->global_symbols[context->global_symbol_count - 1];
}

const MachineUnit *ae_find_program_unit(const AsmEmitContext *context, const char *name) {
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

AsmByteLiteral *ae_ensure_byte_literal(AsmEmitContext *context,
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
    if (!ae_reserve_items((void **)&context->byte_literals,
                       &context->byte_literal_capacity,
                       context->byte_literal_count + 1,
                       sizeof(*context->byte_literals))) {
        return NULL;
    }
    context->byte_literals[context->byte_literal_count].text = ae_copy_text_n(text, length);
    context->byte_literals[context->byte_literal_count].length = length;
    context->byte_literals[context->byte_literal_count].label = ae_copy_format(".L%s_%zu", prefix, context->next_label_id++);
    if (!context->byte_literals[context->byte_literal_count].text ||
        !context->byte_literals[context->byte_literal_count].label) {
        return NULL;
    }
    context->byte_literal_count++;
    return &context->byte_literals[context->byte_literal_count - 1];
}

AsmStringObjectLiteral *ae_ensure_string_literal(AsmEmitContext *context,
                                                     const char *text,
                                                     size_t length) {
    size_t i;

    for (i = 0; i < context->string_literal_count; i++) {
        if (context->string_literals[i].length == length &&
            memcmp(context->string_literals[i].text, text, length) == 0) {
            return &context->string_literals[i];
        }
    }
    if (!ae_reserve_items((void **)&context->string_literals,
                       &context->string_literal_capacity,
                       context->string_literal_count + 1,
                       sizeof(*context->string_literals))) {
        return NULL;
    }
    context->string_literals[context->string_literal_count].text = ae_copy_text_n(text, length);
    context->string_literals[context->string_literal_count].length = length;
    context->string_literals[context->string_literal_count].bytes_label = ae_copy_format(".Lstr_bytes_%zu", context->next_label_id);
    context->string_literals[context->string_literal_count].object_label = ae_copy_format(".Lstr_obj_%zu", context->next_label_id++);
    if (!context->string_literals[context->string_literal_count].text ||
        !context->string_literals[context->string_literal_count].bytes_label ||
        !context->string_literals[context->string_literal_count].object_label) {
        return NULL;
    }
    context->string_literal_count++;
    return &context->string_literals[context->string_literal_count - 1];
}

char *ae_closure_wrapper_symbol_name(const char *unit_name) {
    char *sanitized;
    char *symbol;

    if (!unit_name) {
        return NULL;
    }

    sanitized = ae_sanitize_symbol(unit_name);
    if (!sanitized) {
        return NULL;
    }

    symbol = ae_copy_format("calynda_closure_%s", sanitized);
    free(sanitized);
    return symbol;
}

const MachineFrameSlot *ae_find_frame_slot(const MachineUnit *unit, const char *name) {
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

size_t ae_frame_slot_offset(const AsmUnitLayout *layout, size_t slot_index) {
    return (layout->saved_reg_words + slot_index + 1) * 8;
}

size_t ae_spill_slot_offset(const AsmUnitLayout *layout,
                                const MachineUnit *unit,
                                size_t spill_index) {
    return (layout->saved_reg_words + unit->frame_slot_count + spill_index + 1) * 8;
}

size_t ae_helper_slot_offset(const AsmUnitLayout *layout,
                                 const MachineUnit *unit,
                                 size_t helper_index) {
    return (layout->saved_reg_words + unit->frame_slot_count + unit->spill_slot_count +
            (unit->helper_slot_count - helper_index)) * 8;
}

size_t ae_call_preserve_offset(const AsmUnitLayout *layout,
                                   const MachineUnit *unit,
                                   size_t preserve_index) {
    return (layout->saved_reg_words + unit->frame_slot_count + unit->spill_slot_count +
            unit->helper_slot_count + preserve_index + 1) * 8;
}
