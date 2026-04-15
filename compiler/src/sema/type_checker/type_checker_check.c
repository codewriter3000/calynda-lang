#include "type_checker_internal.h"

bool type_checker_check_program(TypeChecker *checker,
                                const AstProgram *program,
                                const SymbolTable *symbols) {
    const SymbolTableError *symbol_error;
    const UnresolvedIdentifier *unresolved;
    const TypeResolutionError *resolution_error;
    const Scope *root_scope;
    size_t i;

    if (!checker || !program || !symbols) {
        return false;
    }

    type_checker_free(checker);
    type_checker_init(checker);
    checker->program = program;
    checker->symbols = symbols;

    symbol_error = symbol_table_get_error(symbols);
    if (symbol_error) {
        checker->has_error = true;
        checker->error.primary_span = symbol_error->primary_span;
        checker->error.related_span = symbol_error->related_span;
        checker->error.has_related_span = symbol_error->has_related_span;
        strncpy(checker->error.message, symbol_error->message,
                sizeof(checker->error.message) - 1);
        checker->error.message[sizeof(checker->error.message) - 1] = '\0';
        return false;
    }

    unresolved = symbol_table_get_unresolved_identifier(symbols, 0);
    if (unresolved) {
        tc_set_error_at(checker, unresolved->source_span, NULL,
                        "Unresolved identifier '%s'.",
                        (unresolved->identifier &&
                         unresolved->identifier->kind == AST_EXPR_IDENTIFIER &&
                         unresolved->identifier->as.identifier)
                            ? unresolved->identifier->as.identifier
                            : "<unknown>");
        return false;
    }

    if (!type_resolver_resolve_program(&checker->resolver, program)) {
        resolution_error = type_resolver_get_error(&checker->resolver);
        checker->has_error = true;
        if (resolution_error) {
            checker->error.primary_span = resolution_error->primary_span;
            checker->error.related_span = resolution_error->related_span;
            checker->error.has_related_span = resolution_error->has_related_span;
            strncpy(checker->error.message,
                    resolution_error->message,
                    sizeof(checker->error.message) - 1);
            checker->error.message[sizeof(checker->error.message) - 1] = '\0';
        } else {
            strncpy(checker->error.message,
                    "Type resolution failed.",
                    sizeof(checker->error.message) - 1);
            checker->error.message[sizeof(checker->error.message) - 1] = '\0';
        }
        return false;
    }

    if (!tc_validate_program_start_decls(checker, program)) {
        return false;
    }

    root_scope = symbol_table_root_scope(symbols);
    if (!root_scope) {
        tc_set_error(checker, "Internal error: missing root scope.");
        return false;
    }

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (decl->kind == AST_TOP_LEVEL_BINDING) {
            const Symbol *symbol = scope_lookup_local(root_scope, decl->as.binding_decl.name);

            if (!symbol) {
                tc_set_error_at(checker,
                                decl->as.binding_decl.name_span,
                                NULL,
                                "Internal error: missing symbol for '%s'.",
                                decl->as.binding_decl.name);
                return false;
            }

            if (!tc_validate_binding_modifiers(checker, &decl->as.binding_decl)) {
                return false;
            }

            if (!tc_resolve_symbol_info(checker, symbol)) {
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_UNION) {
            const Symbol *symbol = scope_lookup_local(root_scope, decl->as.union_decl.name);

            if (!symbol) {
                tc_set_error_at(checker,
                                decl->as.union_decl.name_span,
                                NULL,
                                "Internal error: missing symbol for union '%s'.",
                                decl->as.union_decl.name);
                return false;
            }

            if (!tc_resolve_symbol_info(checker, symbol)) {
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_ASM) {
            const Symbol *symbol = scope_lookup_local(root_scope, decl->as.asm_decl.name);

            if (!symbol) {
                tc_set_error_at(checker,
                                decl->as.asm_decl.name_span,
                                NULL,
                                "Internal error: missing symbol for asm binding '%s'.",
                                decl->as.asm_decl.name);
                return false;
            }

            if (!tc_resolve_symbol_info(checker, symbol)) {
                return false;
            }
        } else if (!tc_check_start_decl(checker, &decl->as.start_decl)) {
            return false;
        }
    }

    return !checker->has_error;
}

bool checked_type_to_string(CheckedType type, char *buffer, size_t buffer_size) {
    size_t i;
    int written;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    switch (type.kind) {
    case CHECKED_TYPE_INVALID:
        return snprintf(buffer, buffer_size, "<invalid>") >= 0;
    case CHECKED_TYPE_VOID:
        return snprintf(buffer, buffer_size, "void") >= 0;
    case CHECKED_TYPE_NULL:
        return snprintf(buffer, buffer_size, "null") >= 0;
    case CHECKED_TYPE_EXTERNAL:
        return snprintf(buffer, buffer_size, "<external>") >= 0;
    case CHECKED_TYPE_VALUE:
        written = snprintf(buffer, buffer_size, "%s", tc_primitive_type_name(type.primitive));
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

    case CHECKED_TYPE_NAMED:
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

    case CHECKED_TYPE_TYPE_PARAM:
        return snprintf(buffer, buffer_size, "%s", type.name ? type.name : "?") >= 0;
    }

    return false;
}
