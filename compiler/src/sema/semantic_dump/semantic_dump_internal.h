#ifndef SEMANTIC_DUMP_INTERNAL_H
#define SEMANTIC_DUMP_INTERNAL_H

#include "semantic_dump.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char   *data;
    size_t length;
    size_t capacity;
} SemanticDumpBuilder;

/* Builder functions (semantic_dump_builder.c) */
bool sd_builder_append_n(SemanticDumpBuilder *builder, const char *text, size_t length);
bool sd_builder_append(SemanticDumpBuilder *builder, const char *text);
bool sd_builder_append_char(SemanticDumpBuilder *builder, char value);
bool sd_builder_appendf(SemanticDumpBuilder *builder, const char *format, ...);
bool sd_builder_indent(SemanticDumpBuilder *builder, int indent);
bool sd_builder_start_line(SemanticDumpBuilder *builder, int indent);
bool sd_builder_finish_line(SemanticDumpBuilder *builder);
const char *sd_primitive_type_name(AstPrimitiveType primitive);
const char *sd_scope_kind_label(ScopeKind kind);
bool sd_source_span_is_valid(AstSourceSpan span);
bool sd_builder_append_span(SemanticDumpBuilder *builder, AstSourceSpan span);

/* Type formatting (semantic_dump_types.c) */
bool sd_append_ast_type(SemanticDumpBuilder *builder,
                        const AstType *type,
                        bool is_inferred);
bool sd_append_checked_type(SemanticDumpBuilder *builder, CheckedType type);
bool sd_append_effective_symbol_type(SemanticDumpBuilder *builder,
                                     const Symbol *symbol,
                                     const TypeChecker *checker);
bool sd_append_callable_signature(SemanticDumpBuilder *builder,
                                  const TypeCheckInfo *info);
bool sd_append_symbol_reference(SemanticDumpBuilder *builder,
                                const Symbol *symbol,
                                const TypeChecker *checker);

/* Scope dumps (semantic_dump_scope.c) */
const Symbol *sd_find_shadowed_symbol(const SymbolTable *table, const Symbol *symbol);
size_t sd_count_resolutions_for_scope(const SymbolTable *table, const Scope *scope);
size_t sd_count_unresolved_for_scope(const SymbolTable *table, const Scope *scope);
bool sd_dump_package(SemanticDumpBuilder *builder, const SymbolTable *table,
                     const TypeChecker *checker, int indent);
bool sd_dump_imports(SemanticDumpBuilder *builder, const SymbolTable *table,
                     const TypeChecker *checker, int indent);
bool sd_dump_scope(SemanticDumpBuilder *builder, const SymbolTable *table,
                   const TypeChecker *checker, const Scope *scope, int indent);

/* Symbol detail dumps (semantic_dump_symbols.c) */
bool sd_dump_symbols(SemanticDumpBuilder *builder, const SymbolTable *table,
                     const TypeChecker *checker, const Scope *scope, int indent);
bool sd_dump_resolutions(SemanticDumpBuilder *builder, const SymbolTable *table,
                         const TypeChecker *checker, const Scope *scope, int indent);
bool sd_dump_unresolved(SemanticDumpBuilder *builder, const SymbolTable *table,
                        const Scope *scope, int indent);

#endif
