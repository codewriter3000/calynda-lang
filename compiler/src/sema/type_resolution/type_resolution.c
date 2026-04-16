#include "type_resolution_internal.h"

static bool tr_validate_layout_decl(TypeResolver *resolver,
                                    const AstLayoutDecl *layout_decl) {
    size_t field_index;

    if (!resolver || !layout_decl) {
        return false;
    }

    for (field_index = 0; field_index < layout_decl->field_count; field_index++) {
        const AstLayoutField *field = &layout_decl->fields[field_index];

        if (field->field_type.kind != AST_TYPE_PRIMITIVE ||
            field->field_type.dimension_count != 0) {
            tr_set_error_at(resolver,
                            layout_decl->name_span,
                            NULL,
                            "Layout '%s' field '%s' must use a scalar primitive type in 0.4.0.",
                            layout_decl->name ? layout_decl->name : "?",
                            field->name ? field->name : "?");
            return false;
        }
    }

    return true;
}

void type_resolver_init(TypeResolver *resolver) {
    if (!resolver) {
        return;
    }

    memset(resolver, 0, sizeof(*resolver));
}

void type_resolver_free(TypeResolver *resolver) {
    if (!resolver) {
        return;
    }

    free(resolver->type_entries);
    free(resolver->cast_entries);
    memset(resolver, 0, sizeof(*resolver));
}

bool type_resolver_resolve_program(TypeResolver *resolver, const AstProgram *program) {
    size_t i;

    if (!resolver || !program) {
        return false;
    }

    type_resolver_free(resolver);
    type_resolver_init(resolver);
    resolver->program = program;

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (!decl) {
            continue;
        }

        if (decl->kind == AST_TOP_LEVEL_BINDING) {
            if (!tr_resolve_binding_decl(resolver, &decl->as.binding_decl) ||
                !tr_resolve_expression(resolver, decl->as.binding_decl.initializer)) {
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_UNION) {
            const AstUnionDecl *union_decl = &decl->as.union_decl;
            size_t v;
            for (v = 0; v < union_decl->variant_count; v++) {
                if (union_decl->variants[v].payload_type) {
                    if (!tr_resolve_declared_type(resolver,
                                                  union_decl->variants[v].payload_type,
                                                  union_decl->name_span,
                                                  "Union variant",
                                                  union_decl->variants[v].name,
                                                  false)) {
                        return false;
                    }
                }
            }
        } else if (decl->kind == AST_TOP_LEVEL_ASM) {
            const AstAsmDecl *asm_decl = &decl->as.asm_decl;
            size_t p;
            if (!tr_resolve_declared_type(resolver,
                                          &asm_decl->return_type,
                                          asm_decl->name_span,
                                          "Asm binding",
                                          asm_decl->name,
                                          true)) {
                return false;
            }
            for (p = 0; p < asm_decl->parameters.count; p++) {
                if (!tr_resolve_parameter(resolver, &asm_decl->parameters.items[p])) {
                    return false;
                }
            }
        } else if (decl->kind == AST_TOP_LEVEL_LAYOUT) {
            if (!tr_validate_layout_decl(resolver, &decl->as.layout_decl)) {
                return false;
            }
        } else if (!tr_resolve_start_decl(resolver, &decl->as.start_decl)) {
            return false;
        }
    }

    return !resolver->has_error;
}

const TypeResolutionError *type_resolver_get_error(const TypeResolver *resolver) {
    if (!resolver || !resolver->has_error) {
        return NULL;
    }

    return &resolver->error;
}

bool type_resolver_format_error(const TypeResolutionError *error,
                                char *buffer,
                                size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (tr_source_span_is_valid(error->primary_span)) {
        written = snprintf(buffer, buffer_size, "%d:%d: %s",
                           error->primary_span.start_line,
                           error->primary_span.start_column,
                           error->message);
    } else {
        written = snprintf(buffer, buffer_size, "%s", error->message);
    }

    if (written < 0 || (size_t)written >= buffer_size) {
        return false;
    }

    if (error->has_related_span && tr_source_span_is_valid(error->related_span)) {
        written += snprintf(buffer + written, buffer_size - (size_t)written,
                            " Related location at %d:%d.",
                            error->related_span.start_line,
                            error->related_span.start_column);
        if (written < 0 || (size_t)written >= buffer_size) {
            return false;
        }
    }

    return true;
}

const ResolvedType *type_resolver_get_type(const TypeResolver *resolver,
                                           const AstType *type_ref) {
    size_t i;

    if (!resolver || !type_ref) {
        return NULL;
    }

    for (i = 0; i < resolver->type_count; i++) {
        if (resolver->type_entries[i].type_ref == type_ref) {
            return &resolver->type_entries[i].type;
        }
    }

    return NULL;
}

const ResolvedType *type_resolver_get_cast_target_type(const TypeResolver *resolver,
                                                       const AstExpression *cast_expression) {
    size_t i;

    if (!resolver || !cast_expression) {
        return NULL;
    }

    for (i = 0; i < resolver->cast_count; i++) {
        if (resolver->cast_entries[i].cast_expression == cast_expression) {
            return &resolver->cast_entries[i].target_type;
        }
    }

    return NULL;
}

bool resolved_type_to_string(ResolvedType type, char *buffer, size_t buffer_size) {
    size_t i;
    int written;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    switch (type.kind) {
    case RESOLVED_TYPE_INVALID:
        return snprintf(buffer, buffer_size, "<invalid>") >= 0;
    case RESOLVED_TYPE_VOID:
        return snprintf(buffer, buffer_size, "void") >= 0;
    case RESOLVED_TYPE_VALUE:
        written = snprintf(buffer, buffer_size, "%s", tr_primitive_type_name(type.primitive));
        if (written < 0 || (size_t)written >= buffer_size) {
            return false;
        }
        for (i = 0; i < type.array_depth; i++) {
            written += snprintf(buffer + written, buffer_size - (size_t)written, "[]");
            if (written < 0 || (size_t)written >= buffer_size) {
                return false;
            }
        }
        return true;

    case RESOLVED_TYPE_NAMED:
        written = snprintf(buffer, buffer_size, "%s", type.name ? type.name : "?");
        if (written < 0 || (size_t)written >= buffer_size) {
            return false;
        }
        if (type.generic_arg_count > 0) {
            written += snprintf(buffer + written, buffer_size - (size_t)written,
                                "<...%zu>", type.generic_arg_count);
            if (written < 0 || (size_t)written >= buffer_size) {
                return false;
            }
        }
        for (i = 0; i < type.array_depth; i++) {
            written += snprintf(buffer + written, buffer_size - (size_t)written, "[]");
            if (written < 0 || (size_t)written >= buffer_size) {
                return false;
            }
        }
        return true;
    }

    return false;
}