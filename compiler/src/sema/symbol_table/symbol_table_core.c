#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdlib.h>
#include <string.h>

void st_symbol_free(Symbol *symbol) {
    if (!symbol) {
        return;
    }

    free(symbol->name);
    free(symbol->qualified_name);
    free(symbol);
}

void st_scope_free(Scope *scope) {
    size_t i;

    if (!scope) {
        return;
    }

    for (i = 0; i < scope->child_count; i++) {
        st_scope_free(scope->children[i]);
    }

    for (i = 0; i < scope->symbol_count; i++) {
        st_symbol_free(scope->symbols[i]);
    }

    free(scope->children);
    free(scope->symbols);
    free(scope);
}

Scope *st_scope_new(SymbolTable *table, ScopeKind kind,
                    const void *owner, Scope *parent) {
    Scope *scope = calloc(1, sizeof(*scope));

    if (!scope) {
        st_set_error(table, "Out of memory while creating a scope.");
        return NULL;
    }

    scope->kind = kind;
    scope->owner = owner;
    scope->parent = parent;

    if (parent && !st_append_scope_child(table, parent, scope)) {
        free(scope);
        return NULL;
    }

    return scope;
}

bool st_append_scope_child(SymbolTable *table, Scope *parent, Scope *child) {
    Scope **resized;
    size_t new_capacity;

    if (parent->child_count < parent->child_capacity) {
        parent->children[parent->child_count++] = child;
        return true;
    }

    new_capacity = (parent->child_capacity == 0) ? 4 : parent->child_capacity * 2;
    resized = realloc(parent->children, new_capacity * sizeof(*parent->children));
    if (!resized) {
        st_set_error(table, "Out of memory while growing child scopes.");
        return false;
    }

    parent->children = resized;
    parent->child_capacity = new_capacity;
    parent->children[parent->child_count++] = child;
    return true;
}

char *st_qualified_name_to_string(const AstQualifiedName *name) {
    size_t i;
    size_t length = 0;
    char *buffer;
    char *cursor;

    if (!name || name->count == 0) {
        return ast_copy_text("");
    }

    for (i = 0; i < name->count; i++) {
        length += strlen(name->segments[i]);
        if (i + 1 < name->count) {
            length++;
        }
    }

    buffer = malloc(length + 1);
    if (!buffer) {
        return NULL;
    }

    cursor = buffer;
    for (i = 0; i < name->count; i++) {
        size_t segment_length = strlen(name->segments[i]);

        memcpy(cursor, name->segments[i], segment_length);
        cursor += segment_length;

        if (i + 1 < name->count) {
            *cursor++ = '.';
        }
    }

    *cursor = '\0';
    return buffer;
}

char *st_qualified_name_tail(const AstQualifiedName *name) {
    if (!name || name->count == 0) {
        return ast_copy_text("");
    }

    return ast_copy_text(name->segments[name->count - 1]);
}

Symbol *st_symbol_new(SymbolTable *table, SymbolKind kind,
                      const char *name, const char *qualified_name,
                      const AstType *declared_type,
                      bool is_inferred_type, bool is_final,
                      bool is_exported, bool is_static,
                      bool is_internal,
                      AstSourceSpan declaration_span,
                      const void *declaration, Scope *scope) {
    Symbol *symbol = calloc(1, sizeof(*symbol));

    if (!symbol) {
        st_set_error(table, "Out of memory while creating a symbol.");
        return NULL;
    }

    symbol->kind = kind;
    symbol->declared_type = declared_type;
    symbol->is_inferred_type = is_inferred_type;
    symbol->is_final = is_final;
    symbol->is_exported = is_exported;
    symbol->is_static = is_static;
    symbol->is_internal = is_internal;
    symbol->declaration_span = declaration_span;
    symbol->declaration = declaration;
    symbol->scope = scope;

    if (name) {
        symbol->name = ast_copy_text(name);
        if (!symbol->name) {
            st_set_error(table, "Out of memory while copying a symbol name.");
            st_symbol_free(symbol);
            return NULL;
        }
    }

    if (qualified_name) {
        symbol->qualified_name = ast_copy_text(qualified_name);
        if (!symbol->qualified_name) {
            st_set_error(table,
                         "Out of memory while copying a qualified symbol name.");
            st_symbol_free(symbol);
            return NULL;
        }
    }

    return symbol;
}

bool st_scope_append_symbol(SymbolTable *table, Scope *scope, Symbol *symbol) {
    Symbol **resized;
    size_t new_capacity;

    if (scope->symbol_count < scope->symbol_capacity) {
        scope->symbols[scope->symbol_count++] = symbol;
        return true;
    }

    new_capacity = (scope->symbol_capacity == 0) ? 4 : scope->symbol_capacity * 2;
    resized = realloc(scope->symbols, new_capacity * sizeof(*scope->symbols));
    if (!resized) {
        st_set_error(table, "Out of memory while growing scope symbols.");
        return false;
    }

    scope->symbols = resized;
    scope->symbol_capacity = new_capacity;
    scope->symbols[scope->symbol_count++] = symbol;
    return true;
}
