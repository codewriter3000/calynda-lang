#ifndef CALYNDA_SYMBOL_TABLE_H
#define CALYNDA_SYMBOL_TABLE_H

#include "ast.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SYMBOL_KIND_PACKAGE = 0,
    SYMBOL_KIND_IMPORT,
    SYMBOL_KIND_TOP_LEVEL_BINDING,
    SYMBOL_KIND_PARAMETER,
    SYMBOL_KIND_LOCAL
} SymbolKind;

typedef enum {
    SCOPE_KIND_PROGRAM = 0,
    SCOPE_KIND_START,
    SCOPE_KIND_LAMBDA,
    SCOPE_KIND_BLOCK
} ScopeKind;

typedef struct Scope Scope;
typedef struct Symbol Symbol;

struct Symbol {
    SymbolKind      kind;
    char           *name;
    char           *qualified_name;
    const AstType  *declared_type;
    bool            is_inferred_type;
    bool            is_final;
    AstSourceSpan   declaration_span;
    const void     *declaration;
    Scope          *scope;
};

struct Scope {
    ScopeKind  kind;
    const void *owner;
    Scope      *parent;
    Scope     **children;
    size_t      child_count;
    size_t      child_capacity;
    Symbol    **symbols;
    size_t      symbol_count;
    size_t      symbol_capacity;
};

typedef struct {
    const AstExpression *identifier;
    const Scope         *scope;
    AstSourceSpan        source_span;
    const Symbol        *symbol;
} SymbolResolution;

typedef struct {
    const AstExpression *identifier;
    const Scope         *scope;
    AstSourceSpan        source_span;
} UnresolvedIdentifier;

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} SymbolTableError;

typedef struct {
    const AstProgram      *program;
    Symbol                *package_symbol;
    Symbol               **imports;
    size_t                 import_count;
    size_t                 import_capacity;
    Scope                 *root_scope;
    SymbolResolution      *resolutions;
    size_t                 resolution_count;
    size_t                 resolution_capacity;
    UnresolvedIdentifier  *unresolved_identifiers;
    size_t                 unresolved_count;
    size_t                 unresolved_capacity;
    SymbolTableError       error;
    bool                   has_error;
} SymbolTable;

void symbol_table_init(SymbolTable *table);
void symbol_table_free(SymbolTable *table);
bool symbol_table_build(SymbolTable *table, const AstProgram *program);

const SymbolTableError *symbol_table_get_error(const SymbolTable *table);
const UnresolvedIdentifier *symbol_table_get_unresolved_identifier(const SymbolTable *table,
                                                                   size_t index);
bool symbol_table_format_error(const SymbolTableError *error,
                               char *buffer,
                               size_t buffer_size);
bool symbol_table_format_unresolved_identifier(const UnresolvedIdentifier *unresolved,
                                               char *buffer,
                                               size_t buffer_size);

const char *symbol_kind_name(SymbolKind kind);
const char *scope_kind_name(ScopeKind kind);

const Symbol *symbol_table_get_package(const SymbolTable *table);
const Symbol *symbol_table_get_import(const SymbolTable *table, size_t index);
const Symbol *symbol_table_find_import(const SymbolTable *table, const char *name);
const Scope *symbol_table_root_scope(const SymbolTable *table);
const Scope *symbol_table_find_scope(const SymbolTable *table,
                                     const void *owner,
                                     ScopeKind kind);
const Symbol *scope_lookup_local(const Scope *scope, const char *name);
const Symbol *symbol_table_lookup(const SymbolTable *table,
                                  const Scope *scope,
                                  const char *name);
const Symbol *symbol_table_resolve_identifier(const SymbolTable *table,
                                              const AstExpression *identifier);

#endif /* CALYNDA_SYMBOL_TABLE_H */