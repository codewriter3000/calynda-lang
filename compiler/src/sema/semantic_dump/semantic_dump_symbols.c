#include "semantic_dump_internal.h"

bool sd_dump_symbols(SemanticDumpBuilder *builder,
                     const SymbolTable *table,
                     const TypeChecker *checker,
                     const Scope *scope,
                     int indent) {
    size_t i;

    if (!scope || scope->symbol_count == 0) {
        if (!sd_builder_start_line(builder, indent) || !sd_builder_append(builder, "Symbols: []")) {
            return false;
        }
        return sd_builder_finish_line(builder);
    }

    if (!sd_builder_start_line(builder, indent) || !sd_builder_append(builder, "Symbols:")) {
        return false;
    }
    if (!sd_builder_finish_line(builder)) {
        return false;
    }

    for (i = 0; i < scope->symbol_count; i++) {
        const Symbol *symbol = scope->symbols[i];
        const TypeCheckInfo *info = checker ? type_checker_get_symbol_info(checker, symbol) : NULL;
        const Symbol *shadowed;

        if (!sd_builder_start_line(builder, indent + 1) ||
            !sd_builder_appendf(builder,
                                "Symbol name=%s kind=%s declared=",
                                symbol->name ? symbol->name : "<anonymous>",
                                symbol_kind_name(symbol->kind))) {
            return false;
        }
        if (!sd_append_ast_type(builder, symbol->declared_type, symbol->is_inferred_type)) {
            return false;
        }
        if (!sd_builder_append(builder, " type=") ||
            !sd_append_effective_symbol_type(builder, symbol, checker)) {
            return false;
        }
        if (info && !sd_append_callable_signature(builder, info)) {
            return false;
        }
        if (!sd_builder_appendf(builder, " final=%s", symbol->is_final ? "true" : "false")) {
            return false;
        }
        if (symbol->is_exported) {
            if (!sd_builder_append(builder, " exported=true")) {
                return false;
            }
        }
        if (symbol->is_static) {
            if (!sd_builder_append(builder, " static=true")) {
                return false;
            }
        }
        if (symbol->is_internal) {
            if (!sd_builder_append(builder, " internal=true")) {
                return false;
            }
        }
        if (sd_source_span_is_valid(symbol->declaration_span)) {
            if (!sd_builder_append(builder, " span=") ||
                !sd_builder_append_span(builder, symbol->declaration_span)) {
                return false;
            }
        }

        shadowed = sd_find_shadowed_symbol(table, symbol);
        if (shadowed) {
            if (!sd_builder_append(builder, " shadows=") ||
                !sd_builder_append(builder, symbol_kind_name(shadowed->kind))) {
                return false;
            }
            if (shadowed->name && !sd_builder_appendf(builder, " %s", shadowed->name)) {
                return false;
            }
            if (sd_source_span_is_valid(shadowed->declaration_span)) {
                if (!sd_builder_append(builder, " at ") ||
                    !sd_builder_append_span(builder, shadowed->declaration_span)) {
                    return false;
                }
            }
        }

        if (!sd_builder_finish_line(builder)) {
            return false;
        }
    }

    return true;
}

bool sd_dump_resolutions(SemanticDumpBuilder *builder,
                         const SymbolTable *table,
                         const TypeChecker *checker,
                         const Scope *scope,
                         int indent) {
    size_t i;

    if (!table || !scope || sd_count_resolutions_for_scope(table, scope) == 0) {
        if (!sd_builder_start_line(builder, indent) || !sd_builder_append(builder, "Resolutions: []")) {
            return false;
        }
        return sd_builder_finish_line(builder);
    }

    if (!sd_builder_start_line(builder, indent) || !sd_builder_append(builder, "Resolutions:")) {
        return false;
    }
    if (!sd_builder_finish_line(builder)) {
        return false;
    }

    for (i = 0; i < table->resolution_count; i++) {
        const SymbolResolution *resolution = &table->resolutions[i];
        const char *name = "<unknown>";

        if (resolution->scope != scope) {
            continue;
        }

        if (resolution->identifier &&
            resolution->identifier->kind == AST_EXPR_IDENTIFIER &&
            resolution->identifier->as.identifier) {
            name = resolution->identifier->as.identifier;
        }

        if (!sd_builder_start_line(builder, indent + 1) ||
            !sd_builder_appendf(builder, "Resolution name=%s at=", name)) {
            return false;
        }
        if (!sd_builder_append_span(builder, resolution->source_span) ||
            !sd_builder_append(builder, " -> ") ||
            !sd_append_symbol_reference(builder, resolution->symbol, checker) ||
            !sd_builder_finish_line(builder)) {
            return false;
        }
    }

    return true;
}

bool sd_dump_unresolved(SemanticDumpBuilder *builder,
                        const SymbolTable *table,
                        const Scope *scope,
                        int indent) {
    size_t i;

    if (!table || !scope || sd_count_unresolved_for_scope(table, scope) == 0) {
        if (!sd_builder_start_line(builder, indent) || !sd_builder_append(builder, "Unresolved: []")) {
            return false;
        }
        return sd_builder_finish_line(builder);
    }

    if (!sd_builder_start_line(builder, indent) || !sd_builder_append(builder, "Unresolved:")) {
        return false;
    }
    if (!sd_builder_finish_line(builder)) {
        return false;
    }

    for (i = 0; i < table->unresolved_count; i++) {
        const UnresolvedIdentifier *unresolved = &table->unresolved_identifiers[i];
        const char *name = "<unknown>";

        if (unresolved->scope != scope) {
            continue;
        }

        if (unresolved->identifier &&
            unresolved->identifier->kind == AST_EXPR_IDENTIFIER &&
            unresolved->identifier->as.identifier) {
            name = unresolved->identifier->as.identifier;
        }

        if (!sd_builder_start_line(builder, indent + 1) ||
            !sd_builder_appendf(builder, "Unresolved name=%s at=", name) ||
            !sd_builder_append_span(builder, unresolved->source_span) ||
            !sd_builder_finish_line(builder)) {
            return false;
        }
    }

    return true;
}
