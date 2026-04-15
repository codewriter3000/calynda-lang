#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void symbol_table_init(SymbolTable *table) {
    if (!table) {
        return;
    }

    memset(table, 0, sizeof(*table));
}

void symbol_table_free(SymbolTable *table) {
    size_t i;

    if (!table) {
        return;
    }

    st_symbol_free(table->package_symbol);

    for (i = 0; i < table->import_count; i++) {
        st_symbol_free(table->imports[i]);
    }
    free(table->imports);

    st_scope_free(table->root_scope);
    free(table->resolutions);
    free(table->unresolved_identifiers);

    memset(table, 0, sizeof(*table));
}

bool symbol_table_build(SymbolTable *table, const AstProgram *program) {
    size_t i;

    if (!table || !program) {
        return false;
    }

    symbol_table_free(table);
    symbol_table_init(table);
    table->program = program;

    table->root_scope = st_scope_new(table, SCOPE_KIND_PROGRAM, program, NULL);
    if (!table->root_scope) {
        return false;
    }

    if (!st_add_package_symbol(table, program) ||
        !st_add_import_symbols(table, program) ||
        !st_predeclare_top_level_bindings(table, program)) {
        return false;
    }

    for (i = 0; i < program->top_level_count; i++) {
        if (!st_analyze_top_level_decl(table, program->top_level_decls[i], table->root_scope)) {
            return false;
        }
    }

    return !table->has_error;
}

const SymbolTableError *symbol_table_get_error(const SymbolTable *table) {
    if (!table || !table->has_error) {
        return NULL;
    }

    return &table->error;
}

const UnresolvedIdentifier *symbol_table_get_unresolved_identifier(const SymbolTable *table,
                                                                   size_t index) {
    if (!table || index >= table->unresolved_count) {
        return NULL;
    }

    return &table->unresolved_identifiers[index];
}

bool symbol_table_format_error(const SymbolTableError *error,
                               char *buffer,
                               size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (st_source_span_is_valid(error->primary_span)) {
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

    if (error->has_related_span && st_source_span_is_valid(error->related_span)) {
        written += snprintf(buffer + written, buffer_size - (size_t)written,
                            " Previous declaration at %d:%d.",
                            error->related_span.start_line,
                            error->related_span.start_column);
        if (written < 0 || (size_t)written >= buffer_size) {
            return false;
        }
    }

    return true;
}

bool symbol_table_format_unresolved_identifier(const UnresolvedIdentifier *unresolved,
                                               char *buffer,
                                               size_t buffer_size) {
    const char *name = "<unknown>";

    if (!unresolved || !buffer || buffer_size == 0) {
        return false;
    }

    if (unresolved->identifier && unresolved->identifier->kind == AST_EXPR_IDENTIFIER &&
        unresolved->identifier->as.identifier) {
        name = unresolved->identifier->as.identifier;
    }

    if (st_source_span_is_valid(unresolved->source_span)) {
        return snprintf(buffer, buffer_size, "%d:%d: Unresolved identifier '%s'.",
                        unresolved->source_span.start_line,
                        unresolved->source_span.start_column,
                        name) >= 0;
    }

    return snprintf(buffer, buffer_size, "Unresolved identifier '%s'.", name) >= 0;
}
