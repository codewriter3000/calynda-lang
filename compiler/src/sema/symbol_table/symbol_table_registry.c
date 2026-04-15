#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool st_imports_append(SymbolTable *table, Symbol *symbol) {
    Symbol **resized;
    size_t new_capacity;

    if (table->import_count < table->import_capacity) {
        table->imports[table->import_count++] = symbol;
        return true;
    }

    new_capacity = (table->import_capacity == 0) ? 4 : table->import_capacity * 2;
    resized = realloc(table->imports, new_capacity * sizeof(*table->imports));
    if (!resized) {
        st_set_error(table, "Out of memory while growing imports.");
        return false;
    }

    table->imports = resized;
    table->import_capacity = new_capacity;
    table->imports[table->import_count++] = symbol;
    return true;
}

bool st_resolutions_append(SymbolTable *table,
                           const AstExpression *identifier,
                           const Scope *scope,
                           const Symbol *symbol) {
    SymbolResolution *resized;
    size_t new_capacity;

    if (table->resolution_count < table->resolution_capacity) {
        table->resolutions[table->resolution_count].identifier = identifier;
        table->resolutions[table->resolution_count].scope = scope;
        table->resolutions[table->resolution_count].source_span = identifier->source_span;
        table->resolutions[table->resolution_count].symbol = symbol;
        table->resolution_count++;
        return true;
    }

    new_capacity = (table->resolution_capacity == 0) ? 8 : table->resolution_capacity * 2;
    resized = realloc(table->resolutions, new_capacity * sizeof(*table->resolutions));
    if (!resized) {
        st_set_error(table, "Out of memory while storing symbol resolutions.");
        return false;
    }

    table->resolutions = resized;
    table->resolution_capacity = new_capacity;
    table->resolutions[table->resolution_count].identifier = identifier;
    table->resolutions[table->resolution_count].scope = scope;
    table->resolutions[table->resolution_count].source_span = identifier->source_span;
    table->resolutions[table->resolution_count].symbol = symbol;
    table->resolution_count++;
    return true;
}

bool st_unresolved_append(SymbolTable *table,
                          const AstExpression *identifier,
                          const Scope *scope) {
    UnresolvedIdentifier *resized;
    size_t new_capacity;

    if (table->unresolved_count < table->unresolved_capacity) {
        table->unresolved_identifiers[table->unresolved_count].identifier = identifier;
        table->unresolved_identifiers[table->unresolved_count].scope = scope;
        table->unresolved_identifiers[table->unresolved_count].source_span =
            identifier->source_span;
        table->unresolved_count++;
        return true;
    }

    new_capacity = (table->unresolved_capacity == 0) ? 8 : table->unresolved_capacity * 2;
    resized = realloc(table->unresolved_identifiers,
                      new_capacity * sizeof(*table->unresolved_identifiers));
    if (!resized) {
        st_set_error(table,
                     "Out of memory while storing unresolved identifiers.");
        return false;
    }

    table->unresolved_identifiers = resized;
    table->unresolved_capacity = new_capacity;
    table->unresolved_identifiers[table->unresolved_count].identifier = identifier;
    table->unresolved_identifiers[table->unresolved_count].scope = scope;
    table->unresolved_identifiers[table->unresolved_count].source_span =
        identifier->source_span;
    table->unresolved_count++;
    return true;
}

void st_set_error(SymbolTable *table, const char *format, ...) {
    va_list args;

    if (!table || table->has_error) {
        return;
    }

    table->has_error = true;
    va_start(args, format);
    vsnprintf(table->error.message, sizeof(table->error.message), format, args);
    va_end(args);
}

void st_set_error_at(SymbolTable *table,
                     AstSourceSpan primary_span,
                     const AstSourceSpan *related_span,
                     const char *format, ...) {
    va_list args;

    if (!table || table->has_error) {
        return;
    }

    table->has_error = true;
    table->error.primary_span = primary_span;
    if (related_span && st_source_span_is_valid(*related_span)) {
        table->error.related_span = *related_span;
        table->error.has_related_span = true;
    }

    va_start(args, format);
    vsnprintf(table->error.message, sizeof(table->error.message), format, args);
    va_end(args);
}

bool st_source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

const Symbol *st_lookup_in_scopes(const Scope *scope, const char *name) {
    const Scope *current = scope;

    while (current) {
        const Symbol *symbol = scope_lookup_local(current, name);
        if (symbol) {
            return symbol;
        }
        current = current->parent;
    }

    return NULL;
}

bool st_top_level_binding_is_final(const AstBindingDecl *binding_decl) {
    size_t i;

    for (i = 0; i < binding_decl->modifier_count; i++) {
        if (binding_decl->modifiers[i] == AST_MODIFIER_FINAL) {
            return true;
        }
    }

    return false;
}

const Scope *st_find_scope_recursive(const Scope *scope,
                                     const void *owner,
                                     ScopeKind kind) {
    size_t i;

    if (!scope) {
        return NULL;
    }

    if (scope->owner == owner && scope->kind == kind) {
        return scope;
    }

    for (i = 0; i < scope->child_count; i++) {
        const Scope *found = st_find_scope_recursive(scope->children[i], owner, kind);
        if (found) {
            return found;
        }
    }

    return NULL;
}
