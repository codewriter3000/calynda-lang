#include "machine_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool mc_reserve_items(void **items, size_t *capacity,
                      size_t needed, size_t item_size) {
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

bool mc_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

void mc_set_error(MachineBuildContext *context,
                  AstSourceSpan primary_span,
                  const AstSourceSpan *related_span,
                  const char *format,
                  ...) {
    va_list args;

    if (!context || !context->program || context->program->has_error) {
        return;
    }

    context->program->has_error = true;
    context->program->error.primary_span = primary_span;
    if (related_span && mc_source_span_is_valid(*related_span)) {
        context->program->error.related_span = *related_span;
        context->program->error.has_related_span = true;
    }

    va_start(args, format);
    vsnprintf(context->program->error.message,
              sizeof(context->program->error.message),
              format,
              args);
    va_end(args);
}

void mc_block_free(MachineBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    free(block->label);
    for (i = 0; i < block->instruction_count; i++) {
        free(block->instructions[i].text);
    }
    free(block->instructions);
    memset(block, 0, sizeof(*block));
}

void mc_unit_free(MachineUnit *unit) {
    size_t i;

    if (!unit) {
        return;
    }

    free(unit->name);
    for (i = 0; i < unit->frame_slot_count; i++) {
        free(unit->frame_slots[i].name);
    }
    free(unit->frame_slots);
    for (i = 0; i < unit->block_count; i++) {
        mc_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    free(unit->asm_body);
    memset(unit, 0, sizeof(*unit));
}

char *mc_copy_format(const char *format, ...) {
    va_list args;
    va_list args_copy;
    int needed;
    char *buffer;

    if (!format) {
        return NULL;
    }

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

bool mc_append_line(MachineBuildContext *context,
                    MachineBlock *block,
                    const char *format,
                    ...) {
    va_list args;
    va_list args_copy;
    int needed;
    char *buffer;

    if (!block || !format) {
        return false;
    }

    if (!mc_reserve_items((void **)&block->instructions,
                          &block->instruction_capacity,
                          block->instruction_count + 1,
                          sizeof(*block->instructions))) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while appending machine instructions.");
        return false;
    }

    va_start(args, format);
    va_copy(args_copy, args);
    needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    if (needed < 0) {
        va_end(args);
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Failed to format a machine instruction.");
        return false;
    }

    buffer = malloc((size_t)needed + 1);
    if (!buffer) {
        va_end(args);
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while formatting a machine instruction.");
        return false;
    }
    vsnprintf(buffer, (size_t)needed + 1, format, args);
    va_end(args);

    block->instructions[block->instruction_count].text = buffer;
    block->instruction_count++;
    return true;
}

bool mc_checked_type_is_unsigned(CheckedType type) {
    if (type.kind != CHECKED_TYPE_VALUE || type.array_depth != 0) {
        return false;
    }

    switch (type.primitive) {
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_UINT16:
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_UINT64:
    case AST_PRIMITIVE_BOOL:
    case AST_PRIMITIVE_CHAR:
        return true;
    default:
        return false;
    }
}
