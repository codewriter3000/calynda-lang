#include "semantic_dump_internal.h"

const Symbol *sd_find_shadowed_symbol(const SymbolTable *table, const Symbol *symbol) {
    const Scope *scope;

    if (!table || !symbol || !symbol->scope || !symbol->name) {
        return NULL;
    }

    scope = symbol->scope->parent;
    while (scope) {
        const Symbol *shadowed = scope_lookup_local(scope, symbol->name);

        if (shadowed) {
            return shadowed;
        }

        scope = scope->parent;
    }

    if (symbol->kind == SYMBOL_KIND_LOCAL || symbol->kind == SYMBOL_KIND_PARAMETER) {
        return symbol_table_find_import(table, symbol->name);
    }

    return NULL;
}

size_t sd_count_resolutions_for_scope(const SymbolTable *table, const Scope *scope) {
    size_t count = 0;
    size_t i;

    if (!table || !scope) {
        return 0;
    }

    for (i = 0; i < table->resolution_count; i++) {
        if (table->resolutions[i].scope == scope) {
            count++;
        }
    }

    return count;
}

size_t sd_count_unresolved_for_scope(const SymbolTable *table, const Scope *scope) {
    size_t count = 0;
    size_t i;

    if (!table || !scope) {
        return 0;
    }

    for (i = 0; i < table->unresolved_count; i++) {
        if (table->unresolved_identifiers[i].scope == scope) {
            count++;
        }
    }

    return count;
}

bool sd_dump_package(SemanticDumpBuilder *builder,
                     const SymbolTable *table,
                     const TypeChecker *checker,
                     int indent) {
    const Symbol *package_symbol;

    if (!sd_builder_start_line(builder, indent)) {
        return false;
    }

    if (!table || !table->program || !table->program->has_package) {
        if (!sd_builder_append(builder, "Package: <none>")) {
            return false;
        }
        return sd_builder_finish_line(builder);
    }

    package_symbol = symbol_table_get_package(table);
    if (!sd_builder_append(builder, "Package: ")) {
        return false;
    }

    if (!package_symbol) {
        if (!sd_builder_append(builder, "<missing>")) {
            return false;
        }
        return sd_builder_finish_line(builder);
    }

    if (!sd_builder_appendf(builder,
                            "name=%s qualified=%s type=",
                            package_symbol->name ? package_symbol->name : "<anonymous>",
                            package_symbol->qualified_name ? package_symbol->qualified_name : "")) {
        return false;
    }
    if (!sd_append_effective_symbol_type(builder, package_symbol, checker)) {
        return false;
    }
    if (sd_source_span_is_valid(package_symbol->declaration_span)) {
        if (!sd_builder_append(builder, " span=")) {
            return false;
        }
        if (!sd_builder_append_span(builder, package_symbol->declaration_span)) {
            return false;
        }
    }

    return sd_builder_finish_line(builder);
}

bool sd_dump_imports(SemanticDumpBuilder *builder,
                     const SymbolTable *table,
                     const TypeChecker *checker,
                     int indent) {
    size_t i;

    if (!table || table->import_count == 0) {
        if (!sd_builder_start_line(builder, indent) ||
            !sd_builder_append(builder, "Imports: []")) {
            return false;
        }
        return sd_builder_finish_line(builder);
    }

    if (!sd_builder_start_line(builder, indent) || !sd_builder_append(builder, "Imports:")) {
        return false;
    }
    if (!sd_builder_finish_line(builder)) {
        return false;
    }

    for (i = 0; i < table->import_count; i++) {
        const Symbol *import_symbol = table->imports[i];

        if (!sd_builder_start_line(builder, indent + 1) || !sd_builder_append(builder, "Import ")) {
            return false;
        }
        if (!sd_builder_appendf(builder,
                                "name=%s qualified=%s type=",
                                import_symbol->name ? import_symbol->name : "<anonymous>",
                                import_symbol->qualified_name ? import_symbol->qualified_name : "")) {
            return false;
        }
        if (!sd_append_effective_symbol_type(builder, import_symbol, checker)) {
            return false;
        }
        if (sd_source_span_is_valid(import_symbol->declaration_span)) {
            if (!sd_builder_append(builder, " span=")) {
                return false;
            }
            if (!sd_builder_append_span(builder, import_symbol->declaration_span)) {
                return false;
            }
        }
        if (!sd_builder_finish_line(builder)) {
            return false;
        }
    }

    return true;
}

bool sd_dump_scope(SemanticDumpBuilder *builder,
                   const SymbolTable *table,
                   const TypeChecker *checker,
                   const Scope *scope,
                   int indent) {
    size_t i;

    if (!scope) {
        return false;
    }

    if (!sd_builder_start_line(builder, indent) ||
        !sd_builder_appendf(builder, "Scope kind=%s", sd_scope_kind_label(scope->kind))) {
        return false;
    }

    if (scope->kind == SCOPE_KIND_START && scope->owner) {
        const AstStartDecl *start_decl = scope->owner;

        if (sd_source_span_is_valid(start_decl->start_span)) {
            if (!sd_builder_append(builder, " span=") ||
                !sd_builder_append_span(builder, start_decl->start_span)) {
                return false;
            }
        }
    } else if (scope->kind == SCOPE_KIND_LAMBDA && scope->owner) {
        const AstExpression *expression = scope->owner;

        if (sd_source_span_is_valid(expression->source_span)) {
            if (!sd_builder_append(builder, " span=") ||
                !sd_builder_append_span(builder, expression->source_span)) {
                return false;
            }
        }
    } else if (scope->kind == SCOPE_KIND_BLOCK && scope->owner) {
        const AstBlock *block = scope->owner;

        if (!sd_builder_appendf(builder, " statements=%zu", block->statement_count)) {
            return false;
        }
    }

    if (!sd_builder_finish_line(builder)) {
        return false;
    }

    if (!sd_dump_symbols(builder, table, checker, scope, indent + 1) ||
        !sd_dump_resolutions(builder, table, checker, scope, indent + 1) ||
        !sd_dump_unresolved(builder, table, scope, indent + 1)) {
        return false;
    }

    /* Children */
    if (!scope->child_count) {
        if (!sd_builder_start_line(builder, indent + 1) ||
            !sd_builder_append(builder, "Children: []")) {
            return false;
        }
        return sd_builder_finish_line(builder);
    }

    if (!sd_builder_start_line(builder, indent + 1) ||
        !sd_builder_append(builder, "Children:")) {
        return false;
    }
    if (!sd_builder_finish_line(builder)) {
        return false;
    }

    for (i = 0; i < scope->child_count; i++) {
        if (!sd_dump_scope(builder, table, checker, scope->children[i], indent + 2)) {
            return false;
        }
    }

    return true;
}
