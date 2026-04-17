#ifndef CALYNDA_SYMBOL_TABLE_H
#define CALYNDA_SYMBOL_TABLE_H

#include "ast.h"
#include "car.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SYMBOL_KIND_PACKAGE = 0,
    SYMBOL_KIND_IMPORT,
    SYMBOL_KIND_TOP_LEVEL_BINDING,
    SYMBOL_KIND_ASM_BINDING,
    SYMBOL_KIND_PARAMETER,
    SYMBOL_KIND_LOCAL,
    SYMBOL_KIND_UNION,
    SYMBOL_KIND_TYPE_ALIAS,
    SYMBOL_KIND_TYPE_PARAMETER,
    SYMBOL_KIND_VARIANT,
    SYMBOL_KIND_LAYOUT
} SymbolKind;

typedef enum {
    SCOPE_KIND_PROGRAM = 0,
    SCOPE_KIND_START,
    SCOPE_KIND_LAMBDA,
    SCOPE_KIND_BLOCK,
    SCOPE_KIND_UNION
} ScopeKind;

typedef struct Scope Scope;
typedef struct Symbol Symbol;
typedef struct OverloadSet OverloadSet;

struct Symbol {
    SymbolKind      kind;
    char           *name;
    char           *qualified_name;
    const AstType  *declared_type;
    bool            is_inferred_type;
    bool            is_final;
    bool            is_exported;
    bool            is_static;
    bool            is_internal;
    bool            is_thread_local;
    size_t          generic_param_count;
    AstSourceSpan   declaration_span;
    const void     *declaration;
    Scope          *scope;
    AstType         external_declared_type;
    bool            has_external_declared_type;
    AstType         external_return_type;
    bool            has_external_return_type;
    AstParameterList external_parameters;
    /* SYMBOL_KIND_VARIANT fields */
    size_t          variant_index;
    bool            variant_has_payload;
    const AstType  *variant_payload_type;
};

struct OverloadSet {
    char         *name;
    Scope        *scope;
    Symbol      **symbols;
    size_t        symbol_count;
    size_t        symbol_capacity;
    AstSourceSpan declaration_span;
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
    OverloadSet **overload_sets;
    size_t       overload_set_count;
    size_t       overload_set_capacity;
};

typedef struct {
    const AstExpression *identifier;
    const Scope         *scope;
    AstSourceSpan        source_span;
    const Symbol        *symbol;
    const OverloadSet   *overload_set;
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
bool symbol_table_build_with_archive_deps(SymbolTable *table,
                                          const AstProgram *program,
                                          const CarArchive *archive_deps,
                                          size_t archive_dep_count);

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
const OverloadSet *scope_lookup_local_overload_set(const Scope *scope, const char *name);
const Symbol *symbol_table_lookup(const SymbolTable *table,
                                  const Scope *scope,
                                  const char *name);
const OverloadSet *symbol_table_lookup_overload_set(const SymbolTable *table,
                                                    const Scope *scope,
                                                    const char *name);
const Symbol *symbol_table_resolve_identifier(const SymbolTable *table,
                                              const AstExpression *identifier);
const OverloadSet *symbol_table_resolve_overload_set(const SymbolTable *table,
                                                     const AstExpression *identifier);
const SymbolResolution *symbol_table_find_resolution(const SymbolTable *table,
                                                     const AstExpression *identifier);
bool symbol_table_select_resolution(SymbolTable *table,
                                    const AstExpression *identifier,
                                    const Symbol *symbol);
const Symbol *symbol_table_find_symbol_for_declaration(const SymbolTable *table,
                                                       const void *declaration);

#endif /* CALYNDA_SYMBOL_TABLE_H */
