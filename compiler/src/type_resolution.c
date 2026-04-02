#include "type_resolution.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size);
static const char *primitive_type_name(AstPrimitiveType primitive);
static ResolvedType resolved_type_invalid(void);
static ResolvedType resolved_type_void(void);
static ResolvedType resolved_type_value(AstPrimitiveType primitive, size_t array_depth);
static bool source_span_is_valid(AstSourceSpan span);
static void type_resolver_set_error(TypeResolver *resolver, const char *format, ...);
static void type_resolver_set_error_at(TypeResolver *resolver,
                                       AstSourceSpan primary_span,
                                       const AstSourceSpan *related_span,
                                       const char *format,
                                       ...);
static bool append_type_entry(TypeResolver *resolver,
                              const AstType *type_ref,
                              ResolvedType type);
static bool append_cast_entry(TypeResolver *resolver,
                              const AstExpression *cast_expression,
                              ResolvedType target_type);
static bool parse_array_size_literal(const char *text, unsigned long long *value_out);
static bool validate_array_dimensions(TypeResolver *resolver,
                                      const AstType *type,
                                      AstSourceSpan primary_span,
                                      const char *subject_kind,
                                      const char *subject_name);
static bool resolve_declared_type(TypeResolver *resolver,
                                  const AstType *type,
                                  AstSourceSpan primary_span,
                                  const char *subject_kind,
                                  const char *subject_name,
                                  bool allow_void);
static bool resolve_parameter(TypeResolver *resolver, const AstParameter *parameter);
static bool resolve_binding_decl(TypeResolver *resolver, const AstBindingDecl *binding_decl);
static bool resolve_start_decl(TypeResolver *resolver, const AstStartDecl *start_decl);
static bool resolve_block(TypeResolver *resolver, const AstBlock *block);
static bool resolve_statement(TypeResolver *resolver, const AstStatement *statement);
static bool resolve_expression(TypeResolver *resolver, const AstExpression *expression);

void type_resolver_init(TypeResolver *resolver) {
    if (!resolver) {
        return;
    }

    memset(resolver, 0, sizeof(*resolver));
}

void type_resolver_free(TypeResolver *resolver) {
    if (!resolver) {
        return;
    }

    free(resolver->type_entries);
    free(resolver->cast_entries);
    memset(resolver, 0, sizeof(*resolver));
}

bool type_resolver_resolve_program(TypeResolver *resolver, const AstProgram *program) {
    size_t i;

    if (!resolver || !program) {
        return false;
    }

    type_resolver_free(resolver);
    type_resolver_init(resolver);
    resolver->program = program;

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (!decl) {
            continue;
        }

        if (decl->kind == AST_TOP_LEVEL_BINDING) {
            if (!resolve_binding_decl(resolver, &decl->as.binding_decl) ||
                !resolve_expression(resolver, decl->as.binding_decl.initializer)) {
                return false;
            }
        } else if (!resolve_start_decl(resolver, &decl->as.start_decl)) {
            return false;
        }
    }

    return !resolver->has_error;
}

const TypeResolutionError *type_resolver_get_error(const TypeResolver *resolver) {
    if (!resolver || !resolver->has_error) {
        return NULL;
    }

    return &resolver->error;
}

bool type_resolver_format_error(const TypeResolutionError *error,
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
                            " Related location at %d:%d.",
                            error->related_span.start_line,
                            error->related_span.start_column);
        if (written < 0 || (size_t)written >= buffer_size) {
            return false;
        }
    }

    return true;
}

const ResolvedType *type_resolver_get_type(const TypeResolver *resolver,
                                           const AstType *type_ref) {
    size_t i;

    if (!resolver || !type_ref) {
        return NULL;
    }

    for (i = 0; i < resolver->type_count; i++) {
        if (resolver->type_entries[i].type_ref == type_ref) {
            return &resolver->type_entries[i].type;
        }
    }

    return NULL;
}

const ResolvedType *type_resolver_get_cast_target_type(const TypeResolver *resolver,
                                                       const AstExpression *cast_expression) {
    size_t i;

    if (!resolver || !cast_expression) {
        return NULL;
    }

    for (i = 0; i < resolver->cast_count; i++) {
        if (resolver->cast_entries[i].cast_expression == cast_expression) {
            return &resolver->cast_entries[i].target_type;
        }
    }

    return NULL;
}

bool resolved_type_to_string(ResolvedType type, char *buffer, size_t buffer_size) {
    size_t i;
    int written;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    switch (type.kind) {
    case RESOLVED_TYPE_INVALID:
        return snprintf(buffer, buffer_size, "<invalid>") >= 0;
    case RESOLVED_TYPE_VOID:
        return snprintf(buffer, buffer_size, "void") >= 0;
    case RESOLVED_TYPE_VALUE:
        written = snprintf(buffer, buffer_size, "%s", primitive_type_name(type.primitive));
        if (written < 0 || (size_t)written >= buffer_size) {
            return false;
        }
        for (i = 0; i < type.array_depth; i++) {
            written += snprintf(buffer + written, buffer_size - (size_t)written, "[]");
            if (written < 0 || (size_t)written >= buffer_size) {
                return false;
            }
        }
        return true;
    }

    return false;
}

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size) {
    size_t new_capacity;
    void *resized;

    if (needed <= *capacity) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 8 : *capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }

    resized = realloc(*items, new_capacity * item_size);
    if (!resized) {
        return false;
    }

    *items = resized;
    *capacity = new_capacity;
    return true;
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

static ResolvedType resolved_type_invalid(void) {
    ResolvedType type;

    memset(&type, 0, sizeof(type));
    type.kind = RESOLVED_TYPE_INVALID;
    return type;
}

static ResolvedType resolved_type_void(void) {
    ResolvedType type = resolved_type_invalid();
    type.kind = RESOLVED_TYPE_VOID;
    return type;
}

static ResolvedType resolved_type_value(AstPrimitiveType primitive, size_t array_depth) {
    ResolvedType type = resolved_type_invalid();
    type.kind = RESOLVED_TYPE_VALUE;
    type.primitive = primitive;
    type.array_depth = array_depth;
    return type;
}

static bool source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

static void type_resolver_set_error(TypeResolver *resolver, const char *format, ...) {
    va_list args;

    if (!resolver || resolver->has_error) {
        return;
    }

    resolver->has_error = true;
    va_start(args, format);
    vsnprintf(resolver->error.message, sizeof(resolver->error.message), format, args);
    va_end(args);
}

static void type_resolver_set_error_at(TypeResolver *resolver,
                                       AstSourceSpan primary_span,
                                       const AstSourceSpan *related_span,
                                       const char *format,
                                       ...) {
    va_list args;

    if (!resolver || resolver->has_error) {
        return;
    }

    resolver->has_error = true;
    resolver->error.primary_span = primary_span;
    if (related_span && source_span_is_valid(*related_span)) {
        resolver->error.related_span = *related_span;
        resolver->error.has_related_span = true;
    }

    va_start(args, format);
    vsnprintf(resolver->error.message, sizeof(resolver->error.message), format, args);
    va_end(args);
}

static bool append_type_entry(TypeResolver *resolver,
                              const AstType *type_ref,
                              ResolvedType type) {
    if (!reserve_items((void **)&resolver->type_entries,
                       &resolver->type_capacity,
                       resolver->type_count + 1,
                       sizeof(*resolver->type_entries))) {
        type_resolver_set_error(resolver,
                                "Out of memory while storing resolved type information.");
        return false;
    }

    resolver->type_entries[resolver->type_count].type_ref = type_ref;
    resolver->type_entries[resolver->type_count].type = type;
    resolver->type_count++;
    return true;
}

static bool append_cast_entry(TypeResolver *resolver,
                              const AstExpression *cast_expression,
                              ResolvedType target_type) {
    if (!reserve_items((void **)&resolver->cast_entries,
                       &resolver->cast_capacity,
                       resolver->cast_count + 1,
                       sizeof(*resolver->cast_entries))) {
        type_resolver_set_error(resolver,
                                "Out of memory while storing resolved cast types.");
        return false;
    }

    resolver->cast_entries[resolver->cast_count].cast_expression = cast_expression;
    resolver->cast_entries[resolver->cast_count].target_type = target_type;
    resolver->cast_count++;
    return true;
}

static bool parse_array_size_literal(const char *text, unsigned long long *value_out) {
    char *end = NULL;
    unsigned long long value;

    if (!text || text[0] == '\0') {
        return false;
    }

    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno != 0 || !end || *end != '\0') {
        return false;
    }

    if (value_out) {
        *value_out = value;
    }
    return true;
}

static bool validate_array_dimensions(TypeResolver *resolver,
                                      const AstType *type,
                                      AstSourceSpan primary_span,
                                      const char *subject_kind,
                                      const char *subject_name) {
    size_t i;

    if (!type) {
        return false;
    }

    if (type->kind == AST_TYPE_VOID && type->dimension_count > 0) {
        type_resolver_set_error_at(resolver,
                                   primary_span,
                                   NULL,
                                   "%s '%s' cannot declare arrays of void.",
                                   subject_kind,
                                   subject_name ? subject_name : "<anonymous>");
        return false;
    }

    for (i = 0; i < type->dimension_count; i++) {
        if (type->dimensions[i].has_size) {
            unsigned long long size_value = 0;

            if (!parse_array_size_literal(type->dimensions[i].size_literal, &size_value) ||
                size_value == 0) {
                type_resolver_set_error_at(resolver,
                                           primary_span,
                                           NULL,
                                           "%s '%s' has invalid array dimension '%s'; sizes must be positive integers.",
                                           subject_kind,
                                           subject_name ? subject_name : "<anonymous>",
                                           type->dimensions[i].size_literal
                                               ? type->dimensions[i].size_literal
                                               : "<missing>");
                return false;
            }
        }
    }

    return true;
}

static bool resolve_declared_type(TypeResolver *resolver,
                                  const AstType *type,
                                  AstSourceSpan primary_span,
                                  const char *subject_kind,
                                  const char *subject_name,
                                  bool allow_void) {
    ResolvedType resolved_type;

    if (!type) {
        type_resolver_set_error(resolver, "Internal error: missing type to resolve.");
        return false;
    }

    if (!validate_array_dimensions(resolver, type, primary_span, subject_kind, subject_name)) {
        return false;
    }

    if (type->kind == AST_TYPE_VOID) {
        if (!allow_void) {
            type_resolver_set_error_at(resolver,
                                       primary_span,
                                       NULL,
                                       "%s '%s' cannot have type void.",
                                       subject_kind,
                                       subject_name ? subject_name : "<anonymous>");
            return false;
        }

        resolved_type = resolved_type_void();
    } else {
        resolved_type = resolved_type_value(type->primitive, type->dimension_count);
    }

    return append_type_entry(resolver, type, resolved_type);
}

static bool resolve_parameter(TypeResolver *resolver, const AstParameter *parameter) {
    if (!parameter) {
        return true;
    }

    return resolve_declared_type(resolver,
                                 &parameter->type,
                                 parameter->name_span,
                                 "Parameter",
                                 parameter->name,
                                 false);
}

static bool resolve_binding_decl(TypeResolver *resolver, const AstBindingDecl *binding_decl) {
    if (!binding_decl) {
        return true;
    }

    if (binding_decl->is_inferred_type) {
        return true;
    }

    return resolve_declared_type(resolver,
                                 &binding_decl->declared_type,
                                 binding_decl->name_span,
                                 "Top-level binding",
                                 binding_decl->name,
                                 true);
}

static bool resolve_start_decl(TypeResolver *resolver, const AstStartDecl *start_decl) {
    size_t i;

    if (!start_decl) {
        return true;
    }

    for (i = 0; i < start_decl->parameters.count; i++) {
        if (!resolve_parameter(resolver, &start_decl->parameters.items[i])) {
            return false;
        }
    }

    if (start_decl->body.kind == AST_LAMBDA_BODY_BLOCK) {
        return resolve_block(resolver, start_decl->body.as.block);
    }

    return resolve_expression(resolver, start_decl->body.as.expression);
}

static bool resolve_block(TypeResolver *resolver, const AstBlock *block) {
    size_t i;

    if (!block) {
        return true;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (!resolve_statement(resolver, block->statements[i])) {
            return false;
        }
    }

    return true;
}

static bool resolve_statement(TypeResolver *resolver, const AstStatement *statement) {
    if (!statement) {
        return true;
    }

    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        if (!statement->as.local_binding.is_inferred_type &&
            !resolve_declared_type(resolver,
                                   &statement->as.local_binding.declared_type,
                                   statement->as.local_binding.name_span,
                                   "Local",
                                   statement->as.local_binding.name,
                                   true)) {
            return false;
        }
        return resolve_expression(resolver, statement->as.local_binding.initializer);

    case AST_STMT_RETURN:
        return resolve_expression(resolver, statement->as.return_expression);

    case AST_STMT_EXIT:
        return true;

    case AST_STMT_THROW:
        return resolve_expression(resolver, statement->as.throw_expression);

    case AST_STMT_EXPRESSION:
        return resolve_expression(resolver, statement->as.expression);
    }

    return false;
}

static bool resolve_expression(TypeResolver *resolver, const AstExpression *expression) {
    size_t i;

    if (!expression) {
        return true;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                const AstTemplatePart *part = &expression->as.literal.as.template_parts.items[i];

                if (part->kind == AST_TEMPLATE_PART_EXPRESSION &&
                    !resolve_expression(resolver, part->as.expression)) {
                    return false;
                }
            }
        }
        return true;

    case AST_EXPR_IDENTIFIER:
        return true;

    case AST_EXPR_LAMBDA:
        for (i = 0; i < expression->as.lambda.parameters.count; i++) {
            if (!resolve_parameter(resolver, &expression->as.lambda.parameters.items[i])) {
                return false;
            }
        }

        if (expression->as.lambda.body.kind == AST_LAMBDA_BODY_BLOCK) {
            return resolve_block(resolver, expression->as.lambda.body.as.block);
        }

        return resolve_expression(resolver, expression->as.lambda.body.as.expression);

    case AST_EXPR_ASSIGNMENT:
        return resolve_expression(resolver, expression->as.assignment.target) &&
               resolve_expression(resolver, expression->as.assignment.value);

    case AST_EXPR_TERNARY:
        return resolve_expression(resolver, expression->as.ternary.condition) &&
               resolve_expression(resolver, expression->as.ternary.then_branch) &&
               resolve_expression(resolver, expression->as.ternary.else_branch);

    case AST_EXPR_BINARY:
        return resolve_expression(resolver, expression->as.binary.left) &&
               resolve_expression(resolver, expression->as.binary.right);

    case AST_EXPR_UNARY:
        return resolve_expression(resolver, expression->as.unary.operand);

    case AST_EXPR_CALL:
        if (!resolve_expression(resolver, expression->as.call.callee)) {
            return false;
        }
        for (i = 0; i < expression->as.call.arguments.count; i++) {
            if (!resolve_expression(resolver, expression->as.call.arguments.items[i])) {
                return false;
            }
        }
        return true;

    case AST_EXPR_INDEX:
        return resolve_expression(resolver, expression->as.index.target) &&
               resolve_expression(resolver, expression->as.index.index);

    case AST_EXPR_MEMBER:
        return resolve_expression(resolver, expression->as.member.target);

    case AST_EXPR_CAST:
        if (!append_cast_entry(resolver,
                               expression,
                               resolved_type_value(expression->as.cast.target_type, 0))) {
            return false;
        }
        return resolve_expression(resolver, expression->as.cast.expression);

    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            if (!resolve_expression(resolver, expression->as.array_literal.elements.items[i])) {
                return false;
            }
        }
        return true;

    case AST_EXPR_GROUPING:
        return resolve_expression(resolver, expression->as.grouping.inner);
    }

    return false;
}