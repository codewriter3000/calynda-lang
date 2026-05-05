#include "hir_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool hr_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

void hr_set_error(HirBuildContext *context,
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
    if (related_span && hr_source_span_is_valid(*related_span)) {
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

bool hr_reserve_items(void **items, size_t *capacity,
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

char *hr_qualified_name_to_string(const AstQualifiedName *name) {
    size_t i;
    size_t length = 0;
    char *joined;
    char *cursor;

    if (!name || name->count == 0) {
        return ast_copy_text("");
    }

    for (i = 0; i < name->count; i++) {
        length += strlen(name->segments[i]);
        if (i + 1 < name->count) {
            length += 1;
        }
    }

    joined = malloc(length + 1);
    if (!joined) {
        return NULL;
    }

    cursor = joined;
    for (i = 0; i < name->count; i++) {
        size_t segment_length = strlen(name->segments[i]);

        memcpy(cursor, name->segments[i], segment_length);
        cursor += segment_length;
        if (i + 1 < name->count) {
            *cursor++ = '.';
        }
    }
    *cursor = '\0';
    return joined;
}

bool hr_append_named_symbol(HirNamedSymbol **items,
                            size_t *count,
                            size_t *capacity,
                            HirNamedSymbol symbol) {
    if (!hr_reserve_items((void **)items, capacity, *count + 1, sizeof(**items))) {
        return false;
    }

    (*items)[(*count)++] = symbol;
    return true;
}

bool hr_append_top_level_decl(HirProgram *program, HirTopLevelDecl *decl) {
    if (!hr_reserve_items((void **)&program->top_level_decls,
                          &program->top_level_capacity,
                          program->top_level_count + 1,
                          sizeof(*program->top_level_decls))) {
        return false;
    }

    program->top_level_decls[program->top_level_count++] = decl;
    return true;
}

bool hr_append_parameter(HirParameterList *list, HirParameter parameter) {
    if (!hr_reserve_items((void **)&list->items,
                          &list->capacity,
                          list->count + 1,
                          sizeof(*list->items))) {
        return false;
    }

    list->items[list->count++] = parameter;
    return true;
}

bool hr_append_template_part(HirTemplatePartList *list, HirTemplatePart part) {
    if (!hr_reserve_items((void **)&list->items,
                          &list->capacity,
                          list->count + 1,
                          sizeof(*list->items))) {
        return false;
    }

    list->items[list->count++] = part;
    return true;
}

bool hr_append_argument(HirCallExpression *call, HirExpression *argument) {
    if (!hr_reserve_items((void **)&call->arguments,
                          &call->argument_capacity,
                          call->argument_count + 1,
                          sizeof(*call->arguments))) {
        return false;
    }

    call->arguments[call->argument_count++] = argument;
    return true;
}

bool hr_append_array_element(HirArrayLiteralExpression *array_literal,
                             HirExpression *element) {
    if (!hr_reserve_items((void **)&array_literal->elements,
                          &array_literal->element_capacity,
                          array_literal->element_count + 1,
                          sizeof(*array_literal->elements))) {
        return false;
    }

    array_literal->elements[array_literal->element_count++] = element;
    return true;
}

bool hr_append_statement(HirBlock *block, HirStatement *statement) {
    if (!hr_reserve_items((void **)&block->statements,
                          &block->statement_capacity,
                          block->statement_count + 1,
                          sizeof(*block->statements))) {
        return false;
    }

    block->statements[block->statement_count++] = statement;
    return true;
}

CheckedType hr_checked_type_from_resolved_type(ResolvedType type) {
    CheckedType checked;

    memset(&checked, 0, sizeof(checked));
    switch (type.kind) {
    case RESOLVED_TYPE_VOID:
        checked.kind = CHECKED_TYPE_VOID;
        break;
    case RESOLVED_TYPE_VALUE:
        checked.kind = CHECKED_TYPE_VALUE;
        checked.primitive = type.primitive;
        checked.array_depth = type.array_depth;
        break;
    case RESOLVED_TYPE_INVALID:
    default:
        checked.kind = CHECKED_TYPE_INVALID;
        break;
    }

    return checked;
}

size_t hr_primitive_byte_size(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:  case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_BYTE: case AST_PRIMITIVE_SBYTE:
    case AST_PRIMITIVE_BOOL: case AST_PRIMITIVE_CHAR:
        return 1;
    case AST_PRIMITIVE_INT16: case AST_PRIMITIVE_UINT16:
    case AST_PRIMITIVE_SHORT:
        return 2;
    case AST_PRIMITIVE_INT32:  case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_INT:    case AST_PRIMITIVE_UINT:
    case AST_PRIMITIVE_FLOAT32: case AST_PRIMITIVE_FLOAT:
        return 4;
    default:
        return 8;
    }
}

CheckedType hr_checked_type_void_value(void) {
    CheckedType type;

    memset(&type, 0, sizeof(type));
    type.kind = CHECKED_TYPE_VOID;
    return type;
}

size_t hr_layout_byte_size(const SymbolTable *symbols, const char *name) {
    const Scope *root;
    const Symbol *sym;
    const AstLayoutDecl *layout;
    size_t total = 0;
    size_t i;

#include "hir_helpers_p2.inc"
