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

static bool builder_reserve(SemanticDumpBuilder *builder, size_t needed);
static bool builder_append_n(SemanticDumpBuilder *builder, const char *text, size_t length);
static bool builder_append(SemanticDumpBuilder *builder, const char *text);
static bool builder_append_char(SemanticDumpBuilder *builder, char value);
static bool builder_appendf(SemanticDumpBuilder *builder, const char *format, ...);
static bool builder_indent(SemanticDumpBuilder *builder, int indent);
static bool builder_start_line(SemanticDumpBuilder *builder, int indent);
static bool builder_finish_line(SemanticDumpBuilder *builder);

static const char *primitive_type_name(AstPrimitiveType primitive);
static const char *scope_kind_label(ScopeKind kind);
static bool source_span_is_valid(AstSourceSpan span);
static bool builder_append_span(SemanticDumpBuilder *builder, AstSourceSpan span);
static bool append_ast_type(SemanticDumpBuilder *builder,
                            const AstType *type,
                            bool is_inferred);
static bool append_checked_type(SemanticDumpBuilder *builder, CheckedType type);
static bool append_effective_symbol_type(SemanticDumpBuilder *builder,
                                         const Symbol *symbol,
                                         const TypeChecker *checker);
static bool append_callable_signature(SemanticDumpBuilder *builder,
                                      const TypeCheckInfo *info);
static bool append_symbol_reference(SemanticDumpBuilder *builder,
                                    const Symbol *symbol,
                                    const TypeChecker *checker);
static const Symbol *find_shadowed_symbol(const SymbolTable *table, const Symbol *symbol);
static size_t count_resolutions_for_scope(const SymbolTable *table, const Scope *scope);
static size_t count_unresolved_for_scope(const SymbolTable *table, const Scope *scope);
static bool dump_package(SemanticDumpBuilder *builder,
                         const SymbolTable *table,
                         const TypeChecker *checker,
                         int indent);
static bool dump_imports(SemanticDumpBuilder *builder,
                         const SymbolTable *table,
                         const TypeChecker *checker,
                         int indent);
static bool dump_scope(SemanticDumpBuilder *builder,
                       const SymbolTable *table,
                       const TypeChecker *checker,
                       const Scope *scope,
                       int indent);
static bool dump_symbols(SemanticDumpBuilder *builder,
                         const SymbolTable *table,
                         const TypeChecker *checker,
                         const Scope *scope,
                         int indent);
static bool dump_resolutions(SemanticDumpBuilder *builder,
                             const SymbolTable *table,
                             const TypeChecker *checker,
                             const Scope *scope,
                             int indent);
static bool dump_unresolved(SemanticDumpBuilder *builder,
                            const SymbolTable *table,
                            const Scope *scope,
                            int indent);
static bool dump_children(SemanticDumpBuilder *builder,
                          const SymbolTable *table,
                          const TypeChecker *checker,
                          const Scope *scope,
                          int indent);

static bool builder_reserve(SemanticDumpBuilder *builder, size_t needed) {
    char *resized;
    size_t new_capacity;

    if (needed <= builder->capacity) {
        return true;
    }

    new_capacity = (builder->capacity == 0) ? 64 : builder->capacity;
    while (new_capacity < needed) {
        if (new_capacity > SIZE_MAX / 2) {
            new_capacity = needed;
            break;
        }
        new_capacity *= 2;
    }

    resized = realloc(builder->data, new_capacity);
    if (!resized) {
        return false;
    }

    builder->data = resized;
    builder->capacity = new_capacity;
    return true;
}

static bool builder_append_n(SemanticDumpBuilder *builder, const char *text, size_t length) {
    if (!builder || !text) {
        return false;
    }

    if (!builder_reserve(builder, builder->length + length + 1)) {
        return false;
    }

    if (length > 0) {
        memcpy(builder->data + builder->length, text, length);
    }

    builder->length += length;
    builder->data[builder->length] = '\0';
    return true;
}

static bool builder_append(SemanticDumpBuilder *builder, const char *text) {
    return builder_append_n(builder, text, strlen(text));
}

static bool builder_append_char(SemanticDumpBuilder *builder, char value) {
    return builder_append_n(builder, &value, 1);
}

static bool builder_appendf(SemanticDumpBuilder *builder, const char *format, ...) {
    int written;
    va_list args;
    va_list copied_args;

    va_start(args, format);
    va_copy(copied_args, args);
    written = vsnprintf(NULL, 0, format, copied_args);
    va_end(copied_args);

    if (written < 0) {
        va_end(args);
        return false;
    }

    if (!builder_reserve(builder, builder->length + (size_t)written + 1)) {
        va_end(args);
        return false;
    }

    vsnprintf(builder->data + builder->length,
              builder->capacity - builder->length,
              format,
              args);
    builder->length += (size_t)written;
    va_end(args);
    return true;
}

static bool builder_indent(SemanticDumpBuilder *builder, int indent) {
    int i;

    for (i = 0; i < indent; i++) {
        if (!builder_append(builder, "  ")) {
            return false;
        }
    }

    return true;
}

static bool builder_start_line(SemanticDumpBuilder *builder, int indent) {
    return builder_indent(builder, indent);
}

static bool builder_finish_line(SemanticDumpBuilder *builder) {
    return builder_append_char(builder, '\n');
}

static const char *primitive_type_name(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
        return "int8";
    case AST_PRIMITIVE_INT16:
        return "int16";
    case AST_PRIMITIVE_INT32:
        return "int32";
    case AST_PRIMITIVE_INT64:
        return "int64";
    case AST_PRIMITIVE_UINT8:
        return "uint8";
    case AST_PRIMITIVE_UINT16:
        return "uint16";
    case AST_PRIMITIVE_UINT32:
        return "uint32";
    case AST_PRIMITIVE_UINT64:
        return "uint64";
    case AST_PRIMITIVE_FLOAT32:
        return "float32";
    case AST_PRIMITIVE_FLOAT64:
        return "float64";
    case AST_PRIMITIVE_BOOL:
        return "bool";
    case AST_PRIMITIVE_CHAR:
        return "char";
    case AST_PRIMITIVE_STRING:
        return "string";
    }

    return "unknown";
}

static const char *scope_kind_label(ScopeKind kind) {
    switch (kind) {
    case SCOPE_KIND_PROGRAM:
        return "program";
    case SCOPE_KIND_START:
        return "start";
    case SCOPE_KIND_LAMBDA:
        return "lambda";
    case SCOPE_KIND_BLOCK:
        return "block";
    }

    return "unknown";
}

static bool source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

static bool builder_append_span(SemanticDumpBuilder *builder, AstSourceSpan span) {
    if (!source_span_is_valid(span)) {
        return builder_append(builder, "?");
    }

    return builder_appendf(builder, "%d:%d", span.start_line, span.start_column);
}

static bool append_ast_type(SemanticDumpBuilder *builder,
                            const AstType *type,
                            bool is_inferred) {
    size_t i;

    if (is_inferred) {
        return builder_append(builder, "var");
    }

    if (!type) {
        return builder_append(builder, "<none>");
    }

    switch (type->kind) {
    case AST_TYPE_VOID:
        if (!builder_append(builder, "void")) {
            return false;
        }
        break;
    case AST_TYPE_PRIMITIVE:
        if (!builder_append(builder, primitive_type_name(type->primitive))) {
            return false;
        }
        break;
    }

    for (i = 0; i < type->dimension_count; i++) {
        if (type->dimensions[i].has_size && type->dimensions[i].size_literal) {
            if (!builder_appendf(builder, "[%s]", type->dimensions[i].size_literal)) {
                return false;
            }
        } else if (!builder_append(builder, "[]")) {
            return false;
        }
    }

    return true;
}

static bool append_checked_type(SemanticDumpBuilder *builder, CheckedType type) {
    size_t i;

    switch (type.kind) {
    case CHECKED_TYPE_INVALID:
        return builder_append(builder, "<invalid>");
    case CHECKED_TYPE_VOID:
        return builder_append(builder, "void");
    case CHECKED_TYPE_NULL:
        return builder_append(builder, "null");
    case CHECKED_TYPE_EXTERNAL:
        return builder_append(builder, "<external>");
    case CHECKED_TYPE_VALUE:
        if (!builder_append(builder, primitive_type_name(type.primitive))) {
            return false;
        }
        for (i = 0; i < type.array_depth; i++) {
            if (!builder_append(builder, "[]")) {
                return false;
            }
        }
        return true;
    }

    return false;
}

static bool append_effective_symbol_type(SemanticDumpBuilder *builder,
                                         const Symbol *symbol,
                                         const TypeChecker *checker) {
    const TypeCheckInfo *info;

    if (!symbol) {
        return builder_append(builder, "<unknown>");
    }

    info = checker ? type_checker_get_symbol_info(checker, symbol) : NULL;
    if (info) {
        return append_checked_type(builder, info->type);
    }

    if (symbol->kind == SYMBOL_KIND_PACKAGE || symbol->kind == SYMBOL_KIND_IMPORT) {
        return builder_append(builder, "<external>");
    }

    if (!symbol->is_inferred_type && symbol->declared_type) {
        return append_ast_type(builder, symbol->declared_type, false);
    }

    return builder_append(builder, "<untyped>");
}

static bool append_callable_signature(SemanticDumpBuilder *builder,
                                      const TypeCheckInfo *info) {
    size_t i;

    if (!info || !info->is_callable) {
        return true;
    }

    if (!builder_append(builder, " callable=(")) {
        return false;
    }

    if (!info->parameters) {
        if (!builder_append(builder, "...")) {
            return false;
        }
    } else {
        for (i = 0; i < info->parameters->count; i++) {
            if (i > 0 && !builder_append(builder, ", ")) {
                return false;
            }
            if (!append_ast_type(builder, &info->parameters->items[i].type, false)) {
                return false;
            }
        }
    }

    if (!builder_append(builder, ") -> ")) {
        return false;
    }

    return append_checked_type(builder, info->callable_return_type);
}

static bool append_symbol_reference(SemanticDumpBuilder *builder,
                                    const Symbol *symbol,
                                    const TypeChecker *checker) {
    const TypeCheckInfo *info;

    if (!symbol) {
        return builder_append(builder, "<unknown>");
    }

    if (!builder_append(builder, symbol_kind_name(symbol->kind))) {
        return false;
    }

    if (symbol->name && !builder_appendf(builder, " %s", symbol->name)) {
        return false;
    }

    if (symbol->qualified_name && !builder_appendf(builder, " qualified=%s", symbol->qualified_name)) {
        return false;
    }

    if (!builder_append(builder, " type=")) {
        return false;
    }
    if (!append_effective_symbol_type(builder, symbol, checker)) {
        return false;
    }

    info = checker ? type_checker_get_symbol_info(checker, symbol) : NULL;
    if (info && !append_callable_signature(builder, info)) {
        return false;
    }

    return true;
}

static const Symbol *find_shadowed_symbol(const SymbolTable *table, const Symbol *symbol) {
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

static size_t count_resolutions_for_scope(const SymbolTable *table, const Scope *scope) {
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

static size_t count_unresolved_for_scope(const SymbolTable *table, const Scope *scope) {
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

static bool dump_package(SemanticDumpBuilder *builder,
                         const SymbolTable *table,
                         const TypeChecker *checker,
                         int indent) {
    const Symbol *package_symbol;

    if (!builder_start_line(builder, indent)) {
        return false;
    }

    if (!table || !table->program || !table->program->has_package) {
        if (!builder_append(builder, "Package: <none>")) {
            return false;
        }
        return builder_finish_line(builder);
    }

    package_symbol = symbol_table_get_package(table);
    if (!builder_append(builder, "Package: ")) {
        return false;
    }

    if (!package_symbol) {
        if (!builder_append(builder, "<missing>")) {
            return false;
        }
        return builder_finish_line(builder);
    }

    if (!builder_appendf(builder,
                         "name=%s qualified=%s type=",
                         package_symbol->name ? package_symbol->name : "<anonymous>",
                         package_symbol->qualified_name ? package_symbol->qualified_name : "")) {
        return false;
    }
    if (!append_effective_symbol_type(builder, package_symbol, checker)) {
        return false;
    }
    if (source_span_is_valid(package_symbol->declaration_span)) {
        if (!builder_append(builder, " span=")) {
            return false;
        }
        if (!builder_append_span(builder, package_symbol->declaration_span)) {
            return false;
        }
    }

    return builder_finish_line(builder);
}

static bool dump_imports(SemanticDumpBuilder *builder,
                         const SymbolTable *table,
                         const TypeChecker *checker,
                         int indent) {
    size_t i;

    if (!table || table->import_count == 0) {
        if (!builder_start_line(builder, indent) ||
            !builder_append(builder, "Imports: []")) {
            return false;
        }
        return builder_finish_line(builder);
    }

    if (!builder_start_line(builder, indent) || !builder_append(builder, "Imports:")) {
        return false;
    }
    if (!builder_finish_line(builder)) {
        return false;
    }

    for (i = 0; i < table->import_count; i++) {
        const Symbol *import_symbol = table->imports[i];

        if (!builder_start_line(builder, indent + 1) || !builder_append(builder, "Import ")) {
            return false;
        }
        if (!builder_appendf(builder,
                             "name=%s qualified=%s type=",
                             import_symbol->name ? import_symbol->name : "<anonymous>",
                             import_symbol->qualified_name ? import_symbol->qualified_name : "")) {
            return false;
        }
        if (!append_effective_symbol_type(builder, import_symbol, checker)) {
            return false;
        }
        if (source_span_is_valid(import_symbol->declaration_span)) {
            if (!builder_append(builder, " span=")) {
                return false;
            }
            if (!builder_append_span(builder, import_symbol->declaration_span)) {
                return false;
            }
        }
        if (!builder_finish_line(builder)) {
            return false;
        }
    }

    return true;
}

static bool dump_scope(SemanticDumpBuilder *builder,
                       const SymbolTable *table,
                       const TypeChecker *checker,
                       const Scope *scope,
                       int indent) {
    if (!scope) {
        return false;
    }

    if (!builder_start_line(builder, indent) ||
        !builder_appendf(builder, "Scope kind=%s", scope_kind_label(scope->kind))) {
        return false;
    }

    if (scope->kind == SCOPE_KIND_START && scope->owner) {
        const AstStartDecl *start_decl = scope->owner;

        if (source_span_is_valid(start_decl->start_span)) {
            if (!builder_append(builder, " span=") ||
                !builder_append_span(builder, start_decl->start_span)) {
                return false;
            }
        }
    } else if (scope->kind == SCOPE_KIND_LAMBDA && scope->owner) {
        const AstExpression *expression = scope->owner;

        if (source_span_is_valid(expression->source_span)) {
            if (!builder_append(builder, " span=") ||
                !builder_append_span(builder, expression->source_span)) {
                return false;
            }
        }
    } else if (scope->kind == SCOPE_KIND_BLOCK && scope->owner) {
        const AstBlock *block = scope->owner;

        if (!builder_appendf(builder, " statements=%zu", block->statement_count)) {
            return false;
        }
    }

    if (!builder_finish_line(builder)) {
        return false;
    }

    if (!dump_symbols(builder, table, checker, scope, indent + 1) ||
        !dump_resolutions(builder, table, checker, scope, indent + 1) ||
        !dump_unresolved(builder, table, scope, indent + 1) ||
        !dump_children(builder, table, checker, scope, indent + 1)) {
        return false;
    }

    return true;
}

static bool dump_symbols(SemanticDumpBuilder *builder,
                         const SymbolTable *table,
                         const TypeChecker *checker,
                         const Scope *scope,
                         int indent) {
    size_t i;

    if (!scope || scope->symbol_count == 0) {
        if (!builder_start_line(builder, indent) || !builder_append(builder, "Symbols: []")) {
            return false;
        }
        return builder_finish_line(builder);
    }

    if (!builder_start_line(builder, indent) || !builder_append(builder, "Symbols:")) {
        return false;
    }
    if (!builder_finish_line(builder)) {
        return false;
    }

    for (i = 0; i < scope->symbol_count; i++) {
        const Symbol *symbol = scope->symbols[i];
        const TypeCheckInfo *info = checker ? type_checker_get_symbol_info(checker, symbol) : NULL;
        const Symbol *shadowed;

        if (!builder_start_line(builder, indent + 1) ||
            !builder_appendf(builder,
                             "Symbol name=%s kind=%s declared=",
                             symbol->name ? symbol->name : "<anonymous>",
                             symbol_kind_name(symbol->kind))) {
            return false;
        }
        if (!append_ast_type(builder, symbol->declared_type, symbol->is_inferred_type)) {
            return false;
        }
        if (!builder_append(builder, " type=") ||
            !append_effective_symbol_type(builder, symbol, checker)) {
            return false;
        }
        if (info && !append_callable_signature(builder, info)) {
            return false;
        }
        if (!builder_appendf(builder, " final=%s", symbol->is_final ? "true" : "false")) {
            return false;
        }
        if (source_span_is_valid(symbol->declaration_span)) {
            if (!builder_append(builder, " span=") ||
                !builder_append_span(builder, symbol->declaration_span)) {
                return false;
            }
        }

        shadowed = find_shadowed_symbol(table, symbol);
        if (shadowed) {
            if (!builder_append(builder, " shadows=") ||
                !builder_append(builder, symbol_kind_name(shadowed->kind))) {
                return false;
            }
            if (shadowed->name && !builder_appendf(builder, " %s", shadowed->name)) {
                return false;
            }
            if (source_span_is_valid(shadowed->declaration_span)) {
                if (!builder_append(builder, " at ") ||
                    !builder_append_span(builder, shadowed->declaration_span)) {
                    return false;
                }
            }
        }

        if (!builder_finish_line(builder)) {
            return false;
        }
    }

    return true;
}

static bool dump_resolutions(SemanticDumpBuilder *builder,
                             const SymbolTable *table,
                             const TypeChecker *checker,
                             const Scope *scope,
                             int indent) {
    size_t i;

    if (!table || !scope || count_resolutions_for_scope(table, scope) == 0) {
        if (!builder_start_line(builder, indent) || !builder_append(builder, "Resolutions: []")) {
            return false;
        }
        return builder_finish_line(builder);
    }

    if (!builder_start_line(builder, indent) || !builder_append(builder, "Resolutions:")) {
        return false;
    }
    if (!builder_finish_line(builder)) {
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

        if (!builder_start_line(builder, indent + 1) ||
            !builder_appendf(builder, "Resolution name=%s at=", name)) {
            return false;
        }
        if (!builder_append_span(builder, resolution->source_span) ||
            !builder_append(builder, " -> ") ||
            !append_symbol_reference(builder, resolution->symbol, checker) ||
            !builder_finish_line(builder)) {
            return false;
        }
    }

    return true;
}

static bool dump_unresolved(SemanticDumpBuilder *builder,
                            const SymbolTable *table,
                            const Scope *scope,
                            int indent) {
    size_t i;

    if (!table || !scope || count_unresolved_for_scope(table, scope) == 0) {
        if (!builder_start_line(builder, indent) || !builder_append(builder, "Unresolved: []")) {
            return false;
        }
        return builder_finish_line(builder);
    }

    if (!builder_start_line(builder, indent) || !builder_append(builder, "Unresolved:")) {
        return false;
    }
    if (!builder_finish_line(builder)) {
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

        if (!builder_start_line(builder, indent + 1) ||
            !builder_appendf(builder, "Unresolved name=%s at=", name) ||
            !builder_append_span(builder, unresolved->source_span) ||
            !builder_finish_line(builder)) {
            return false;
        }
    }

    return true;
}

static bool dump_children(SemanticDumpBuilder *builder,
                          const SymbolTable *table,
                          const TypeChecker *checker,
                          const Scope *scope,
                          int indent) {
    size_t i;

    if (!scope || scope->child_count == 0) {
        if (!builder_start_line(builder, indent) || !builder_append(builder, "Children: []")) {
            return false;
        }
        return builder_finish_line(builder);
    }

    if (!builder_start_line(builder, indent) || !builder_append(builder, "Children:")) {
        return false;
    }
    if (!builder_finish_line(builder)) {
        return false;
    }

    for (i = 0; i < scope->child_count; i++) {
        if (!dump_scope(builder, table, checker, scope->children[i], indent + 1)) {
            return false;
        }
    }

    return true;
}

bool semantic_dump_program(FILE *out,
                           const SymbolTable *symbols,
                           const TypeChecker *checker) {
    char *dump;
    int written;

    if (!out) {
        return false;
    }

    dump = semantic_dump_program_to_string(symbols, checker);
    if (!dump) {
        return false;
    }

    written = fputs(dump, out);
    free(dump);
    return written >= 0;
}

char *semantic_dump_program_to_string(const SymbolTable *symbols,
                                      const TypeChecker *checker) {
    SemanticDumpBuilder builder = {0};
    const TypeCheckError *type_error;
    char error_buffer[256];

    if (!symbols) {
        return NULL;
    }

    if (!builder_start_line(&builder, 0) ||
        !builder_append(&builder, "SemanticProgram") ||
        !builder_finish_line(&builder) ||
        !dump_package(&builder, symbols, checker, 1) ||
        !dump_imports(&builder, symbols, checker, 1)) {
        free(builder.data);
        return NULL;
    }

    type_error = checker ? type_checker_get_error(checker) : NULL;
    if (type_error) {
        if (!type_checker_format_error(type_error, error_buffer, sizeof(error_buffer))) {
            strncpy(error_buffer, type_error->message, sizeof(error_buffer) - 1);
            error_buffer[sizeof(error_buffer) - 1] = '\0';
        }
        if (!builder_start_line(&builder, 1) ||
            !builder_appendf(&builder, "TypeCheckError: %s", error_buffer) ||
            !builder_finish_line(&builder)) {
            free(builder.data);
            return NULL;
        }
    }

    if (!builder_start_line(&builder, 1) || !builder_append(&builder, "Scopes:")) {
        free(builder.data);
        return NULL;
    }
    if (!builder_finish_line(&builder)) {
        free(builder.data);
        return NULL;
    }

    if (!symbols->root_scope) {
        if (!builder_start_line(&builder, 2) ||
            !builder_append(&builder, "<none>") ||
            !builder_finish_line(&builder)) {
            free(builder.data);
            return NULL;
        }
        return builder.data;
    }

    if (!dump_scope(&builder, symbols, checker, symbols->root_scope, 2)) {
        free(builder.data);
        return NULL;
    }

    return builder.data;
}