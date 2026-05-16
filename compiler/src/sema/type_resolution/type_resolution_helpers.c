#include "type_resolution_internal.h"

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size) {
    size_t new_capacity;
    void *resized;

    if (needed <= *capacity) {
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

static bool parse_array_size_literal(const char *text, unsigned long long *value_out) {
    char *end = NULL;
    unsigned long long value;

    if (!text || text[0] == '\0') {
        return false;
    }

    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno != 0 || !end || *end != '\0') {
        return false;
    }

    if (value_out) {
        *value_out = value;
    }
    return true;
}

static bool tr_register_owned_array_extent_block(TypeResolver *resolver,
                                                 ArrayExtent *array_extents) {
    if (!reserve_items((void **)&resolver->owned_array_extent_blocks,
                       &resolver->owned_array_extent_block_capacity,
                       resolver->owned_array_extent_block_count + 1,
                       sizeof(*resolver->owned_array_extent_blocks))) {
        tr_set_error(resolver,
                     "Out of memory while storing resolved array extent metadata.");
        return false;
    }

    resolver->owned_array_extent_blocks[resolver->owned_array_extent_block_count++] =
        array_extents;
    return true;
}

static bool tr_allocate_array_extents(TypeResolver *resolver,
                                      size_t count,
                                      ArrayExtent **array_extents_out) {
    ArrayExtent *array_extents;

    if (!resolver || !array_extents_out) {
        return false;
    }

    *array_extents_out = NULL;
    if (count == 0) {
        return true;
    }

    array_extents = calloc(count, sizeof(*array_extents));
    if (!array_extents) {
        tr_set_error(resolver,
                     "Out of memory while allocating resolved array extent metadata.");
        return false;
    }

    if (!tr_register_owned_array_extent_block(resolver, array_extents)) {
        free(array_extents);
        return false;
    }

    *array_extents_out = array_extents;
    return true;
}

const char *tr_primitive_type_name(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
        return "int8";
    case AST_PRIMITIVE_INT16:
        return "int16";
    case AST_PRIMITIVE_INT32:
        return "int32";
    case AST_PRIMITIVE_INT64:
        return "int64";
    case AST_PRIMITIVE_UINT8:
        return "uint8";
    case AST_PRIMITIVE_UINT16:
        return "uint16";
    case AST_PRIMITIVE_UINT32:
        return "uint32";
    case AST_PRIMITIVE_UINT64:
        return "uint64";
    case AST_PRIMITIVE_FLOAT32:
        return "float32";
    case AST_PRIMITIVE_FLOAT64:
        return "float64";
    case AST_PRIMITIVE_BOOL:
        return "bool";
    case AST_PRIMITIVE_CHAR:
        return "char";
    case AST_PRIMITIVE_STRING:
        return "string";
    case AST_PRIMITIVE_BYTE:
        return "byte";
    case AST_PRIMITIVE_SBYTE:
        return "sbyte";
    case AST_PRIMITIVE_SHORT:
        return "short";
    case AST_PRIMITIVE_INT:
        return "int";
    case AST_PRIMITIVE_LONG:
        return "long";
    case AST_PRIMITIVE_ULONG:
        return "ulong";
    case AST_PRIMITIVE_UINT:
        return "uint";
    case AST_PRIMITIVE_FLOAT:
        return "float";
    case AST_PRIMITIVE_DOUBLE:
        return "double";
    }

    return "unknown";
}

ResolvedType tr_resolved_type_invalid(void) {
    ResolvedType type;

    memset(&type, 0, sizeof(type));
    type.kind = RESOLVED_TYPE_INVALID;
    return type;
}

ResolvedType tr_resolved_type_void(void) {
    ResolvedType type = tr_resolved_type_invalid();
    type.kind = RESOLVED_TYPE_VOID;
    return type;
}

ResolvedType tr_resolved_type_value(AstPrimitiveType primitive, size_t array_depth) {
    ResolvedType type = tr_resolved_type_invalid();
    type.kind = RESOLVED_TYPE_VALUE;
    type.primitive = primitive;
    type.array_depth = array_depth;
    return type;
}

ResolvedType tr_resolved_type_named(const char *name, size_t generic_arg_count,
                                    size_t array_depth) {
    ResolvedType type = tr_resolved_type_invalid();
    type.kind = RESOLVED_TYPE_NAMED;
    type.name = name;
    type.generic_arg_count = generic_arg_count;
    type.array_depth = array_depth;
    return type;
}

ResolvedType tr_resolved_type_with_extra_arrays(ResolvedType type,
                                                size_t extra_array_depth) {
    type.array_depth += extra_array_depth;
    return type;
}

bool tr_build_array_extents(TypeResolver *resolver,
                            const ArrayExtent *base_extents,
                            size_t base_extent_count,
                            const AstArrayDimension *extra_dimensions,
                            size_t extra_dimension_count,
                            ArrayExtent **array_extents_out) {
    ArrayExtent *array_extents;
    size_t total_count = base_extent_count + extra_dimension_count;
    size_t i;

    if (!resolver || !array_extents_out) {
        return false;
    }

    *array_extents_out = NULL;
    if (total_count == 0) {
        return true;
    }

    if (!tr_allocate_array_extents(resolver, total_count, &array_extents)) {
        return false;
    }

    for (i = 0; i < base_extent_count; i++) {
        if (base_extents) {
            array_extents[i] = base_extents[i];
        }
    }

    for (i = 0; i < extra_dimension_count; i++) {
        if (extra_dimensions[i].has_size) {
            unsigned long long size_value = 0;

            if (!parse_array_size_literal(extra_dimensions[i].size_literal, &size_value) ||
                size_value == 0) {
                tr_set_error(resolver,
                             "Internal error: invalid resolved array extent '%s'.",
                             extra_dimensions[i].size_literal
                                 ? extra_dimensions[i].size_literal
                                 : "<missing>");
                return false;
            }

            array_extents[base_extent_count + i].has_size = true;
            array_extents[base_extent_count + i].size = size_value;
        }
    }

    *array_extents_out = array_extents;
    return true;
}

bool tr_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

void tr_set_error(TypeResolver *resolver, const char *format, ...) {
    va_list args;

    if (!resolver || resolver->has_error) {
        return;
    }

    resolver->has_error = true;
    va_start(args, format);
    vsnprintf(resolver->error.message, sizeof(resolver->error.message), format, args);
    va_end(args);
}

void tr_set_error_at(TypeResolver *resolver,
                     AstSourceSpan primary_span,
                     const AstSourceSpan *related_span,
                     const char *format,
                     ...) {
    va_list args;

    if (!resolver || resolver->has_error) {
        return;
    }

    resolver->has_error = true;
    resolver->error.primary_span = primary_span;
    if (related_span && tr_source_span_is_valid(*related_span)) {
        resolver->error.related_span = *related_span;
        resolver->error.has_related_span = true;
    }

    va_start(args, format);
    vsnprintf(resolver->error.message, sizeof(resolver->error.message), format, args);
    va_end(args);
}

bool tr_append_type_entry(TypeResolver *resolver,
                          const AstType *type_ref,
                          ResolvedType type) {
    if (!reserve_items((void **)&resolver->type_entries,
                       &resolver->type_capacity,
                       resolver->type_count + 1,
                       sizeof(*resolver->type_entries))) {
        tr_set_error(resolver,
                     "Out of memory while storing resolved type information.");
        return false;
    }

    resolver->type_entries[resolver->type_count].type_ref = type_ref;
    resolver->type_entries[resolver->type_count].type = type;
    resolver->type_count++;
    return true;
}

bool tr_append_cast_entry(TypeResolver *resolver,
                          const AstExpression *cast_expression,
                          ResolvedType target_type) {
    if (!reserve_items((void **)&resolver->cast_entries,
                       &resolver->cast_capacity,
                       resolver->cast_count + 1,
                       sizeof(*resolver->cast_entries))) {
        tr_set_error(resolver,
                     "Out of memory while storing resolved cast types.");
        return false;
    }

    resolver->cast_entries[resolver->cast_count].cast_expression = cast_expression;
    resolver->cast_entries[resolver->cast_count].target_type = target_type;
    resolver->cast_count++;
    return true;
}

bool tr_append_alias_entry(TypeResolver *resolver,
                           const char *name,
                           const AstTypeAliasDecl *decl) {
    if (!reserve_items((void **)&resolver->alias_entries,
                       &resolver->alias_capacity,
                       resolver->alias_count + 1,
                       sizeof(*resolver->alias_entries))) {
        tr_set_error(resolver,
                     "Out of memory while storing type alias information.");
        return false;
    }

    resolver->alias_entries[resolver->alias_count].name = name;
    resolver->alias_entries[resolver->alias_count].decl = decl;
    resolver->alias_count++;
    return true;
}

const AstTypeAliasDecl *tr_find_alias_decl(const TypeResolver *resolver,
                                           const char *name) {
    size_t i;

    if (!resolver || !name) {
        return NULL;
    }

    for (i = 0; i < resolver->alias_count; i++) {
        if (resolver->alias_entries[i].name &&
            strcmp(resolver->alias_entries[i].name, name) == 0) {
            return resolver->alias_entries[i].decl;
        }
    }

    return NULL;
}
