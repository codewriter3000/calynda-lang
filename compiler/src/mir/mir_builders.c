#include "mir_internal.h"

const HirUnionDecl *mr_find_hir_union_decl(const MirBuildContext *build,
                                           const char *union_name) {
    size_t d;

    if (!build || !build->hir_program || !union_name) {
        return NULL;
    }
    for (d = 0; d < build->hir_program->top_level_count; d++) {
        const HirTopLevelDecl *decl = build->hir_program->top_level_decls[d];

        if (decl->kind == HIR_TOP_LEVEL_UNION && decl->as.union_decl.name &&
            strcmp(decl->as.union_decl.name, union_name) == 0) {
            return &decl->as.union_decl;
        }
    }
    return NULL;
}

CalyndaRtTypeTag mr_checked_type_to_runtime_tag(CheckedType type) {
    if (type.kind == CHECKED_TYPE_VALUE) {
        if (type.array_depth > 0) {
            return CALYNDA_RT_TYPE_ARRAY;
        }
        switch (type.primitive) {
        case AST_PRIMITIVE_BOOL:   return CALYNDA_RT_TYPE_BOOL;
        case AST_PRIMITIVE_STRING: return CALYNDA_RT_TYPE_STRING;
        case AST_PRIMITIVE_INT64:
        case AST_PRIMITIVE_UINT64: return CALYNDA_RT_TYPE_INT64;
        default:                   return CALYNDA_RT_TYPE_INT32;
        }
    }
    if (type.kind == CHECKED_TYPE_NAMED) {
        return type.name && strcmp(type.name, "arr") == 0
            ? CALYNDA_RT_TYPE_HETERO_ARRAY : CALYNDA_RT_TYPE_UNION;
    }
    if (type.kind == CHECKED_TYPE_EXTERNAL) {
        return CALYNDA_RT_TYPE_EXTERNAL;
    }
    return CALYNDA_RT_TYPE_RAW_WORD;
}

bool mr_find_hir_union_variant(const MirBuildContext *build,
                               const char *union_name,
                               const char *variant_name,
                               size_t *out_variant_index,
                               size_t *out_variant_count) {
    const HirUnionDecl *decl = mr_find_hir_union_decl(build, union_name);
    size_t v;

    if (!decl) {
        return false;
    }
    if (out_variant_count) {
        *out_variant_count = decl->variant_count;
    }
    for (v = 0; v < decl->variant_count; v++) {
        if (decl->variants[v].name && strcmp(decl->variants[v].name, variant_name) == 0) {
            if (out_variant_index) {
                *out_variant_index = v;
            }
            return true;
        }
    }
    return false;
}

bool mr_reserve_items(void **items, size_t *capacity,
                      size_t needed, size_t item_size) {
    void *resized;
    size_t new_capacity;

    if (*capacity >= needed) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 4 : *capacity;
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

bool mr_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

CheckedType mr_checked_type_void_value(void) {
    CheckedType type;

    memset(&type, 0, sizeof(type));
    type.kind = CHECKED_TYPE_VOID;
    return type;
}

void mr_set_error(MirBuildContext *context,
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
    if (related_span && mr_source_span_is_valid(*related_span)) {
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

bool mr_append_unit(MirProgram *program, MirUnit unit) {
    if (!mr_reserve_items((void **)&program->units,
                       &program->unit_capacity,
                       program->unit_count + 1,
                       sizeof(*program->units))) {
        return false;
    }

    program->units[program->unit_count++] = unit;
    return true;
}

bool mr_append_local(MirUnit *unit, MirLocal local) {
    if (!mr_reserve_items((void **)&unit->locals,
                       &unit->local_capacity,
                       unit->local_count + 1,
                       sizeof(*unit->locals))) {
        return false;
    }

    unit->locals[unit->local_count++] = local;
    return true;
}

bool mr_append_block(MirUnit *unit, MirBasicBlock block) {
    if (!mr_reserve_items((void **)&unit->blocks,
                       &unit->block_capacity,
                       unit->block_count + 1,
                       sizeof(*unit->blocks))) {
        return false;
    }

    unit->blocks[unit->block_count++] = block;
    return true;
}

bool mr_append_instruction(MirBasicBlock *block, MirInstruction instruction) {
    if (!mr_reserve_items((void **)&block->instructions,
                       &block->instruction_capacity,
                       block->instruction_count + 1,
                       sizeof(*block->instructions))) {
        return false;
    }

    block->instructions[block->instruction_count++] = instruction;
    return true;
}

MirBasicBlock *mr_current_block(MirUnitBuildContext *context) {
    if (!context || !context->unit || context->current_block_index >= context->unit->block_count) {
        return NULL;
    }

    return &context->unit->blocks[context->current_block_index];
}

bool mr_create_block(MirUnitBuildContext *context, size_t *block_index) {
    MirBasicBlock block;
    char label[32];
    int written;
    size_t index;

    if (!context || !context->unit || !block_index) {
        return false;
    }

    index = context->unit->block_count;
    written = snprintf(label, sizeof(label), "bb%zu", index);
    if (written < 0 || (size_t)written >= sizeof(label)) {
        mr_set_error(context->build,
                      (AstSourceSpan){0},
                      NULL,
                      "Failed to label MIR block.");
        return false;
    }

    memset(&block, 0, sizeof(block));
    block.label = ast_copy_text(label);
    if (!block.label) {
        mr_set_error(context->build,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR blocks.");
        return NULL;
    }

    if (!mr_append_block(context->unit, block)) {
        free(block.label);
        mr_set_error(context->build,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR blocks.");
        return NULL;
    }

    *block_index = index;
    return true;
}

size_t mr_find_local_index(const MirUnit *unit, const Symbol *symbol) {
    size_t i;

    if (!unit || !symbol) {
        return (size_t)-1;
    }

    for (i = 0; i < unit->local_count; i++) {
        if (unit->locals[i].symbol == symbol) {
            return i;
        }
    }

    return (size_t)-1;
}
