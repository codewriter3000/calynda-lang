#ifndef SYMBOL_TABLE_INTERNAL_H
#define SYMBOL_TABLE_INTERNAL_H

#include "symbol_table.h"

/* symbol_table_core.c */
void st_symbol_free(Symbol *symbol);
void st_scope_free(Scope *scope);
Scope *st_scope_new(SymbolTable *table, ScopeKind kind,
                    const void *owner, Scope *parent);
bool st_append_scope_child(SymbolTable *table, Scope *parent, Scope *child);
char *st_qualified_name_to_string(const AstQualifiedName *name);
char *st_qualified_name_tail(const AstQualifiedName *name);
Symbol *st_symbol_new(SymbolTable *table, SymbolKind kind,
                      const char *name, const char *qualified_name,
                      const AstType *declared_type,
                      bool is_inferred_type, bool is_final,
                      bool is_exported, bool is_static,
                      bool is_internal,
                      AstSourceSpan declaration_span,
                      const void *declaration, Scope *scope);
bool st_scope_append_symbol(SymbolTable *table, Scope *scope, Symbol *symbol);

/* symbol_table_registry.c */
bool st_imports_append(SymbolTable *table, Symbol *symbol);
bool st_resolutions_append(SymbolTable *table,
                           const AstExpression *identifier,
                           const Scope *scope,
                           const Symbol *symbol);
bool st_unresolved_append(SymbolTable *table,
                          const AstExpression *identifier,
                          const Scope *scope);
void st_set_error(SymbolTable *table, const char *format, ...);
void st_set_error_at(SymbolTable *table,
                     AstSourceSpan primary_span,
                     const AstSourceSpan *related_span,
                     const char *format, ...);
bool st_source_span_is_valid(AstSourceSpan span);
const Symbol *st_lookup_in_scopes(const Scope *scope, const char *name);
bool st_top_level_binding_is_final(const AstBindingDecl *binding_decl);
const Scope *st_find_scope_recursive(const Scope *scope,
                                     const void *owner,
                                     ScopeKind kind);

/* symbol_table_imports.c */
bool st_add_package_symbol(SymbolTable *table, const AstProgram *program);
bool st_add_import_symbols(SymbolTable *table, const AstProgram *program);

/* symbol_table_predecl.c */
bool st_predeclare_top_level_bindings(SymbolTable *table, const AstProgram *program);

/* symbol_table_analyze.c */
bool st_analyze_top_level_decl(SymbolTable *table,
                               const AstTopLevelDecl *decl,
                               Scope *scope);
bool st_analyze_start_decl(SymbolTable *table, const AstStartDecl *start_decl,
                           Scope *parent_scope);
bool st_analyze_union_decl(SymbolTable *table, const AstUnionDecl *union_decl,
                           Scope *parent_scope);
bool st_analyze_lambda_expression(SymbolTable *table,
                                  const AstExpression *lambda_expression,
                                  Scope *parent_scope);
bool st_analyze_block(SymbolTable *table, const AstBlock *block,
                      Scope *parent_scope);
bool st_analyze_statement(SymbolTable *table, const AstStatement *statement,
                          Scope *scope);

/* symbol_table_analyze_expr.c */
bool st_analyze_expression(SymbolTable *table, const AstExpression *expression,
                           Scope *scope);
bool st_add_parameter_symbols(SymbolTable *table,
                              const AstParameterList *parameters,
                              Scope *scope);
bool st_add_local_symbol(SymbolTable *table,
                         const AstLocalBindingStatement *binding,
                         Scope *scope);

#endif
