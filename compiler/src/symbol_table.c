#include "symbol_table.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void symbol_free(Symbol *symbol);
static void scope_free(Scope *scope);
static Scope *scope_new(SymbolTable *table, ScopeKind kind,
                        const void *owner, Scope *parent);
static bool append_scope_child(SymbolTable *table, Scope *parent, Scope *child);
static char *qualified_name_to_string(const AstQualifiedName *name);
static char *qualified_name_tail(const AstQualifiedName *name);
static Symbol *symbol_new(SymbolTable *table, SymbolKind kind,
                          const char *name, const char *qualified_name,
                          const AstType *declared_type,
                          bool is_inferred_type, bool is_final,
                          AstSourceSpan declaration_span,
                          const void *declaration, Scope *scope);
static bool scope_append_symbol(SymbolTable *table, Scope *scope, Symbol *symbol);
static bool imports_append(SymbolTable *table, Symbol *symbol);
static bool resolutions_append(SymbolTable *table,
                               const AstExpression *identifier,
                               const Scope *scope,
                               const Symbol *symbol);
static bool unresolved_append(SymbolTable *table,
                              const AstExpression *identifier,
                              const Scope *scope);
static void symbol_table_set_error(SymbolTable *table, const char *format, ...);
static void symbol_table_set_error_at(SymbolTable *table,
                                      AstSourceSpan primary_span,
                                      const AstSourceSpan *related_span,
                                      const char *format, ...);
static bool add_package_symbol(SymbolTable *table, const AstProgram *program);
static bool add_import_symbols(SymbolTable *table, const AstProgram *program);
static bool predeclare_top_level_bindings(SymbolTable *table, const AstProgram *program);
static bool analyze_top_level_decl(SymbolTable *table,
                                   const AstTopLevelDecl *decl,
                                   Scope *scope);
static bool analyze_start_decl(SymbolTable *table, const AstStartDecl *start_decl,
                               Scope *parent_scope);
static bool analyze_lambda_expression(SymbolTable *table,
                                      const AstExpression *lambda_expression,
                                      Scope *parent_scope);
static bool analyze_block(SymbolTable *table, const AstBlock *block,
                          Scope *parent_scope);
static bool analyze_statement(SymbolTable *table, const AstStatement *statement,
                              Scope *scope);
static bool analyze_expression(SymbolTable *table, const AstExpression *expression,
                               Scope *scope);
static bool add_parameter_symbols(SymbolTable *table,
                                  const AstParameterList *parameters,
                                  Scope *scope);
static bool add_local_symbol(SymbolTable *table,
                             const AstLocalBindingStatement *binding,
                             Scope *scope);
static bool top_level_binding_is_final(const AstBindingDecl *binding_decl);
static const Symbol *lookup_in_scopes(const Scope *scope, const char *name);
static const Scope *find_scope_recursive(const Scope *scope,
                                        const void *owner,
                                        ScopeKind kind);
static bool source_span_is_valid(AstSourceSpan span);

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

    symbol_free(table->package_symbol);

    for (i = 0; i < table->import_count; i++) {
        symbol_free(table->imports[i]);
    }
    free(table->imports);

    scope_free(table->root_scope);
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

    table->root_scope = scope_new(table, SCOPE_KIND_PROGRAM, program, NULL);
    if (!table->root_scope) {
        return false;
    }

    if (!add_package_symbol(table, program) ||
        !add_import_symbols(table, program) ||
        !predeclare_top_level_bindings(table, program)) {
        return false;
    }

    for (i = 0; i < program->top_level_count; i++) {
        if (!analyze_top_level_decl(table, program->top_level_decls[i], table->root_scope)) {
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

    if (source_span_is_valid(error->primary_span)) {
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

    if (error->has_related_span && source_span_is_valid(error->related_span)) {
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

    if (source_span_is_valid(unresolved->source_span)) {
        return snprintf(buffer, buffer_size, "%d:%d: Unresolved identifier '%s'.",
                        unresolved->source_span.start_line,
                        unresolved->source_span.start_column,
                        name) >= 0;
    }

    return snprintf(buffer, buffer_size, "Unresolved identifier '%s'.", name) >= 0;
}

const char *symbol_kind_name(SymbolKind kind) {
    switch (kind) {
    case SYMBOL_KIND_PACKAGE:
        return "package";
    case SYMBOL_KIND_IMPORT:
        return "import";
    case SYMBOL_KIND_TOP_LEVEL_BINDING:
        return "top-level binding";
    case SYMBOL_KIND_PARAMETER:
        return "parameter";
    case SYMBOL_KIND_LOCAL:
        return "local";
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

    return find_scope_recursive(table->root_scope, owner, kind);
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

    symbol = lookup_in_scopes(scope, name);
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

static void symbol_free(Symbol *symbol) {
    if (!symbol) {
        return;
    }

    free(symbol->name);
    free(symbol->qualified_name);
    free(symbol);
}

static void scope_free(Scope *scope) {
    size_t i;

    if (!scope) {
        return;
    }

    for (i = 0; i < scope->child_count; i++) {
        scope_free(scope->children[i]);
    }

    for (i = 0; i < scope->symbol_count; i++) {
        symbol_free(scope->symbols[i]);
    }

    free(scope->children);
    free(scope->symbols);
    free(scope);
}

static Scope *scope_new(SymbolTable *table, ScopeKind kind,
                        const void *owner, Scope *parent) {
    Scope *scope = calloc(1, sizeof(*scope));

    if (!scope) {
        symbol_table_set_error(table, "Out of memory while creating a scope.");
        return NULL;
    }

    scope->kind = kind;
    scope->owner = owner;
    scope->parent = parent;

    if (parent && !append_scope_child(table, parent, scope)) {
        free(scope);
        return NULL;
    }

    return scope;
}

static bool append_scope_child(SymbolTable *table, Scope *parent, Scope *child) {
    Scope **resized;
    size_t new_capacity;

    if (parent->child_count < parent->child_capacity) {
        parent->children[parent->child_count++] = child;
        return true;
    }

    new_capacity = (parent->child_capacity == 0) ? 4 : parent->child_capacity * 2;
    resized = realloc(parent->children, new_capacity * sizeof(*parent->children));
    if (!resized) {
        symbol_table_set_error(table, "Out of memory while growing child scopes.");
        return false;
    }

    parent->children = resized;
    parent->child_capacity = new_capacity;
    parent->children[parent->child_count++] = child;
    return true;
}

static char *qualified_name_to_string(const AstQualifiedName *name) {
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

static char *qualified_name_tail(const AstQualifiedName *name) {
    if (!name || name->count == 0) {
        return ast_copy_text("");
    }

    return ast_copy_text(name->segments[name->count - 1]);
}

static Symbol *symbol_new(SymbolTable *table, SymbolKind kind,
                          const char *name, const char *qualified_name,
                          const AstType *declared_type,
                          bool is_inferred_type, bool is_final,
                          AstSourceSpan declaration_span,
                          const void *declaration, Scope *scope) {
    Symbol *symbol = calloc(1, sizeof(*symbol));

    if (!symbol) {
        symbol_table_set_error(table, "Out of memory while creating a symbol.");
        return NULL;
    }

    symbol->kind = kind;
    symbol->declared_type = declared_type;
    symbol->is_inferred_type = is_inferred_type;
    symbol->is_final = is_final;
    symbol->declaration_span = declaration_span;
    symbol->declaration = declaration;
    symbol->scope = scope;

    if (name) {
        symbol->name = ast_copy_text(name);
        if (!symbol->name) {
            symbol_table_set_error(table, "Out of memory while copying a symbol name.");
            symbol_free(symbol);
            return NULL;
        }
    }

    if (qualified_name) {
        symbol->qualified_name = ast_copy_text(qualified_name);
        if (!symbol->qualified_name) {
            symbol_table_set_error(table,
                                   "Out of memory while copying a qualified symbol name.");
            symbol_free(symbol);
            return NULL;
        }
    }

    return symbol;
}

static bool scope_append_symbol(SymbolTable *table, Scope *scope, Symbol *symbol) {
    Symbol **resized;
    size_t new_capacity;

    if (scope->symbol_count < scope->symbol_capacity) {
        scope->symbols[scope->symbol_count++] = symbol;
        return true;
    }

    new_capacity = (scope->symbol_capacity == 0) ? 4 : scope->symbol_capacity * 2;
    resized = realloc(scope->symbols, new_capacity * sizeof(*scope->symbols));
    if (!resized) {
        symbol_table_set_error(table, "Out of memory while growing scope symbols.");
        return false;
    }

    scope->symbols = resized;
    scope->symbol_capacity = new_capacity;
    scope->symbols[scope->symbol_count++] = symbol;
    return true;
}

static bool imports_append(SymbolTable *table, Symbol *symbol) {
    Symbol **resized;
    size_t new_capacity;

    if (table->import_count < table->import_capacity) {
        table->imports[table->import_count++] = symbol;
        return true;
    }

    new_capacity = (table->import_capacity == 0) ? 4 : table->import_capacity * 2;
    resized = realloc(table->imports, new_capacity * sizeof(*table->imports));
    if (!resized) {
        symbol_table_set_error(table, "Out of memory while growing imports.");
        return false;
    }

    table->imports = resized;
    table->import_capacity = new_capacity;
    table->imports[table->import_count++] = symbol;
    return true;
}

static bool resolutions_append(SymbolTable *table,
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
        symbol_table_set_error(table, "Out of memory while storing symbol resolutions.");
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

static bool unresolved_append(SymbolTable *table,
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
        symbol_table_set_error(table,
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

static void symbol_table_set_error(SymbolTable *table, const char *format, ...) {
    va_list args;

    if (!table || table->has_error) {
        return;
    }

    table->has_error = true;
    va_start(args, format);
    vsnprintf(table->error.message, sizeof(table->error.message), format, args);
    va_end(args);
}

static void symbol_table_set_error_at(SymbolTable *table,
                                      AstSourceSpan primary_span,
                                      const AstSourceSpan *related_span,
                                      const char *format, ...) {
    va_list args;

    if (!table || table->has_error) {
        return;
    }

    table->has_error = true;
    table->error.primary_span = primary_span;
    if (related_span && source_span_is_valid(*related_span)) {
        table->error.related_span = *related_span;
        table->error.has_related_span = true;
    }

    va_start(args, format);
    vsnprintf(table->error.message, sizeof(table->error.message), format, args);
    va_end(args);
}

static bool add_package_symbol(SymbolTable *table, const AstProgram *program) {
    char *name;
    char *qualified_name;

    if (!program->has_package) {
        return true;
    }

    name = qualified_name_tail(&program->package_name);
    qualified_name = qualified_name_to_string(&program->package_name);
    if (!name || !qualified_name) {
        free(name);
        free(qualified_name);
        symbol_table_set_error(table, "Out of memory while creating package symbol.");
        return false;
    }

    table->package_symbol = symbol_new(table, SYMBOL_KIND_PACKAGE,
                                       name, qualified_name,
                                       NULL, false, false,
                                       program->package_name.tail_span,
                                       &program->package_name, NULL);
    free(name);
    free(qualified_name);
    return table->package_symbol != NULL;
}

static bool add_import_symbols(SymbolTable *table, const AstProgram *program) {
    size_t i;

    for (i = 0; i < program->import_count; i++) {
        char *name = qualified_name_tail(&program->imports[i]);
        char *qualified_name = qualified_name_to_string(&program->imports[i]);
        const Symbol *existing_import = NULL;
        Symbol *symbol;

        if (!name || !qualified_name) {
            free(name);
            free(qualified_name);
            symbol_table_set_error(table, "Out of memory while creating import symbol.");
            return false;
        }

        existing_import = symbol_table_find_import(table, name);
        if (existing_import) {
            symbol_table_set_error_at(table,
                                      program->imports[i].tail_span,
                                      &existing_import->declaration_span,
                                      "Duplicate import alias '%s'.",
                                      name);
            free(name);
            free(qualified_name);
            return false;
        }

        symbol = symbol_new(table, SYMBOL_KIND_IMPORT, name, qualified_name,
                            NULL, false, false,
                            program->imports[i].tail_span,
                            &program->imports[i], table->root_scope);
        free(name);
        free(qualified_name);
        if (!symbol) {
            return false;
        }

        if (!imports_append(table, symbol)) {
            symbol_free(symbol);
            return false;
        }
    }

    return true;
}

static bool predeclare_top_level_bindings(SymbolTable *table, const AstProgram *program) {
    size_t i;

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (decl->kind == AST_TOP_LEVEL_BINDING) {
            const AstBindingDecl *binding_decl = &decl->as.binding_decl;
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  binding_decl->name);
            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, binding_decl->name);
            }
            Symbol *symbol;

            if (conflicting_symbol) {
                symbol_table_set_error_at(table,
                                          binding_decl->name_span,
                                          &conflicting_symbol->declaration_span,
                                          "Duplicate symbol '%s' in %s.",
                                          binding_decl->name,
                                          scope_kind_name(table->root_scope->kind));
                return false;
            }

            symbol = symbol_new(table, SYMBOL_KIND_TOP_LEVEL_BINDING,
                                binding_decl->name, NULL,
                                &binding_decl->declared_type,
                                binding_decl->is_inferred_type,
                                top_level_binding_is_final(binding_decl),
                                binding_decl->name_span,
                                binding_decl,
                                table->root_scope);
            if (!symbol) {
                return false;
            }

            if (!scope_append_symbol(table, table->root_scope, symbol)) {
                symbol_free(symbol);
                return false;
            }
        }
    }

    return true;
}

static bool analyze_top_level_decl(SymbolTable *table,
                                   const AstTopLevelDecl *decl,
                                   Scope *scope) {
    switch (decl->kind) {
    case AST_TOP_LEVEL_START:
        return analyze_start_decl(table, &decl->as.start_decl, scope);
    case AST_TOP_LEVEL_BINDING:
        return analyze_expression(table, decl->as.binding_decl.initializer, scope);
    }

    return false;
}

static bool analyze_start_decl(SymbolTable *table, const AstStartDecl *start_decl,
                               Scope *parent_scope) {
    Scope *start_scope = scope_new(table, SCOPE_KIND_START, start_decl, parent_scope);

    if (!start_scope) {
        return false;
    }

    if (!add_parameter_symbols(table, &start_decl->parameters, start_scope)) {
        return false;
    }

    if (start_decl->body.kind == AST_LAMBDA_BODY_BLOCK) {
        return analyze_block(table, start_decl->body.as.block, start_scope);
    }

    return analyze_expression(table, start_decl->body.as.expression, start_scope);
}

static bool analyze_lambda_expression(SymbolTable *table,
                                      const AstExpression *lambda_expression,
                                      Scope *parent_scope) {
    Scope *lambda_scope = scope_new(table, SCOPE_KIND_LAMBDA,
                                    lambda_expression, parent_scope);

    if (!lambda_scope) {
        return false;
    }

    if (!add_parameter_symbols(table, &lambda_expression->as.lambda.parameters,
                               lambda_scope)) {
        return false;
    }

    if (lambda_expression->as.lambda.body.kind == AST_LAMBDA_BODY_BLOCK) {
        return analyze_block(table, lambda_expression->as.lambda.body.as.block,
                             lambda_scope);
    }

    return analyze_expression(table, lambda_expression->as.lambda.body.as.expression,
                              lambda_scope);
}

static bool analyze_block(SymbolTable *table, const AstBlock *block,
                          Scope *parent_scope) {
    Scope *block_scope = scope_new(table, SCOPE_KIND_BLOCK, block, parent_scope);
    size_t i;

    if (!block_scope) {
        return false;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (!analyze_statement(table, block->statements[i], block_scope)) {
            return false;
        }
    }

    return true;
}

static bool analyze_statement(SymbolTable *table, const AstStatement *statement,
                              Scope *scope) {
    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        if (!analyze_expression(table, statement->as.local_binding.initializer, scope)) {
            return false;
        }
        return add_local_symbol(table, &statement->as.local_binding, scope);

    case AST_STMT_RETURN:
        if (!statement->as.return_expression) {
            return true;
        }
        return analyze_expression(table, statement->as.return_expression, scope);

    case AST_STMT_EXIT:
        return true;

    case AST_STMT_THROW:
        return analyze_expression(table, statement->as.throw_expression, scope);

    case AST_STMT_EXPRESSION:
        return analyze_expression(table, statement->as.expression, scope);
    }

    return false;
}

static bool analyze_expression(SymbolTable *table, const AstExpression *expression,
                               Scope *scope) {
    size_t i;

    if (!expression) {
        return true;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                const AstTemplatePart *part =
                    &expression->as.literal.as.template_parts.items[i];

                if (part->kind == AST_TEMPLATE_PART_EXPRESSION &&
                    !analyze_expression(table, part->as.expression, scope)) {
                    return false;
                }
            }
        }
        return true;

    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *resolved = symbol_table_lookup(table, scope,
                                                         expression->as.identifier);

            if (resolved) {
                return resolutions_append(table, expression, scope, resolved);
            }

            return unresolved_append(table, expression, scope);
        }

    case AST_EXPR_LAMBDA:
        return analyze_lambda_expression(table, expression, scope);

    case AST_EXPR_ASSIGNMENT:
        return analyze_expression(table, expression->as.assignment.target, scope) &&
               analyze_expression(table, expression->as.assignment.value, scope);

    case AST_EXPR_TERNARY:
        return analyze_expression(table, expression->as.ternary.condition, scope) &&
               analyze_expression(table, expression->as.ternary.then_branch, scope) &&
               analyze_expression(table, expression->as.ternary.else_branch, scope);

    case AST_EXPR_BINARY:
        return analyze_expression(table, expression->as.binary.left, scope) &&
               analyze_expression(table, expression->as.binary.right, scope);

    case AST_EXPR_UNARY:
        return analyze_expression(table, expression->as.unary.operand, scope);

    case AST_EXPR_CALL:
        if (!analyze_expression(table, expression->as.call.callee, scope)) {
            return false;
        }
        for (i = 0; i < expression->as.call.arguments.count; i++) {
            if (!analyze_expression(table, expression->as.call.arguments.items[i], scope)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_INDEX:
        return analyze_expression(table, expression->as.index.target, scope) &&
               analyze_expression(table, expression->as.index.index, scope);

    case AST_EXPR_MEMBER:
        return analyze_expression(table, expression->as.member.target, scope);

    case AST_EXPR_CAST:
        return analyze_expression(table, expression->as.cast.expression, scope);

    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            if (!analyze_expression(table,
                                    expression->as.array_literal.elements.items[i],
                                    scope)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_GROUPING:
        return analyze_expression(table, expression->as.grouping.inner, scope);
    }

    return false;
}

static bool add_parameter_symbols(SymbolTable *table,
                                  const AstParameterList *parameters,
                                  Scope *scope) {
    size_t i;

    for (i = 0; i < parameters->count; i++) {
        const AstParameter *parameter = &parameters->items[i];
        const Symbol *conflicting_symbol = scope_lookup_local(scope, parameter->name);
        Symbol *symbol;

        if (conflicting_symbol) {
            symbol_table_set_error_at(table,
                                      parameter->name_span,
                                      &conflicting_symbol->declaration_span,
                                      "Duplicate symbol '%s' in %s.",
                                      parameter->name,
                                      scope_kind_name(scope->kind));
            return false;
        }

        symbol = symbol_new(table, SYMBOL_KIND_PARAMETER,
                            parameter->name, NULL,
                            &parameter->type,
                            false, false,
                            parameter->name_span,
                            parameter, scope);
        if (!symbol) {
            return false;
        }

        if (!scope_append_symbol(table, scope, symbol)) {
            symbol_free(symbol);
            return false;
        }
    }

    return true;
}

static bool add_local_symbol(SymbolTable *table,
                             const AstLocalBindingStatement *binding,
                             Scope *scope) {
    const Symbol *conflicting_symbol = scope_lookup_local(scope, binding->name);
    Symbol *symbol;

    if (conflicting_symbol) {
        symbol_table_set_error_at(table,
                                  binding->name_span,
                                  &conflicting_symbol->declaration_span,
                                  "Duplicate symbol '%s' in %s.",
                                  binding->name,
                                  scope_kind_name(scope->kind));
        return false;
    }

    symbol = symbol_new(table, SYMBOL_KIND_LOCAL,
                        binding->name, NULL,
                        &binding->declared_type,
                        binding->is_inferred_type,
                        binding->is_final,
                        binding->name_span,
                        binding, scope);
    if (!symbol) {
        return false;
    }

    if (!scope_append_symbol(table, scope, symbol)) {
        symbol_free(symbol);
        return false;
    }

    return true;
}

static bool top_level_binding_is_final(const AstBindingDecl *binding_decl) {
    size_t i;

    for (i = 0; i < binding_decl->modifier_count; i++) {
        if (binding_decl->modifiers[i] == AST_MODIFIER_FINAL) {
            return true;
        }
    }

    return false;
}

static const Symbol *lookup_in_scopes(const Scope *scope, const char *name) {
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

static bool source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

static const Scope *find_scope_recursive(const Scope *scope,
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
        const Scope *found = find_scope_recursive(scope->children[i], owner, kind);
        if (found) {
            return found;
        }
    }

    return NULL;
}