#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <string.h>

const char *symbol_kind_name(SymbolKind kind) {
    switch (kind) {
    case SYMBOL_KIND_PACKAGE:
        return "package";
    case SYMBOL_KIND_IMPORT:
        return "import";
    case SYMBOL_KIND_TOP_LEVEL_BINDING:
        return "top-level binding";
    case SYMBOL_KIND_ASM_BINDING:
        return "asm binding";
    case SYMBOL_KIND_PARAMETER:
        return "parameter";
    case SYMBOL_KIND_LOCAL:
        return "local";
    case SYMBOL_KIND_UNION:
        return "union";
    case SYMBOL_KIND_TYPE_ALIAS:
        return "type alias";
    case SYMBOL_KIND_TYPE_PARAMETER:
        return "type parameter";
    case SYMBOL_KIND_VARIANT:
        return "variant";
    case SYMBOL_KIND_LAYOUT:
        return "layout";
    }

    return "unknown";
}

const char *scope_kind_name(ScopeKind kind) {
    switch (kind) {
    case SCOPE_KIND_PROGRAM:
        return "program scope";
    case SCOPE_KIND_START:
        return "start scope";
    case SCOPE_KIND_LAMBDA:
        return "lambda scope";
    case SCOPE_KIND_BLOCK:
        return "block scope";
    case SCOPE_KIND_UNION:
        return "union scope";
    }

    return "unknown scope";
}

const Symbol *symbol_table_get_package(const SymbolTable *table) {
    return table ? table->package_symbol : NULL;
}

const Symbol *symbol_table_get_import(const SymbolTable *table, size_t index) {
    if (!table || index >= table->import_count) {
        return NULL;
    }

    return table->imports[index];
}

const Symbol *symbol_table_find_import(const SymbolTable *table, const char *name) {
    size_t i;

    if (!table || !name) {
        return NULL;
    }

    for (i = 0; i < table->import_count; i++) {
        if (table->imports[i] && table->imports[i]->name &&
            strcmp(table->imports[i]->name, name) == 0) {
            return table->imports[i];
        }
    }

    return NULL;
}

const Scope *symbol_table_root_scope(const SymbolTable *table) {
    return table ? table->root_scope : NULL;
}

const Scope *symbol_table_find_scope(const SymbolTable *table,
                                     const void *owner,
                                     ScopeKind kind) {
    if (!table || !table->root_scope) {
        return NULL;
    }

    return st_find_scope_recursive(table->root_scope, owner, kind);
}

const Symbol *scope_lookup_local(const Scope *scope, const char *name) {
    size_t i;

    if (!scope || !name) {
        return NULL;
    }

    for (i = 0; i < scope->symbol_count; i++) {
        if (scope->symbols[i] && scope->symbols[i]->name &&
            strcmp(scope->symbols[i]->name, name) == 0) {
            return scope->symbols[i];
        }
    }

    return NULL;
}

const Symbol *symbol_table_lookup(const SymbolTable *table,
                                  const Scope *scope,
                                  const char *name) {
    const Symbol *symbol;

    if (!table || !name) {
        return NULL;
    }

    symbol = st_lookup_in_scopes(scope, name);
    if (symbol) {
        return symbol;
    }

    return symbol_table_find_import(table, name);
}

const Symbol *symbol_table_resolve_identifier(const SymbolTable *table,
                                              const AstExpression *identifier) {
    size_t i;

    if (!table || !identifier) {
        return NULL;
    }

    for (i = 0; i < table->resolution_count; i++) {
        if (table->resolutions[i].identifier == identifier) {
            return table->resolutions[i].symbol;
        }
    }

    return NULL;
}

const SymbolResolution *symbol_table_find_resolution(const SymbolTable *table,
                                                     const AstExpression *identifier) {
    size_t i;

    if (!table || !identifier) {
        return NULL;
    }

    for (i = 0; i < table->resolution_count; i++) {
        if (table->resolutions[i].identifier == identifier) {
            return &table->resolutions[i];
        }
    }

    return NULL;
}
