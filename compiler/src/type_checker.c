#include "type_checker.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    BLOCK_CONTEXT_LAMBDA = 0,
    BLOCK_CONTEXT_START
} BlockContextKind;

typedef struct {
    BlockContextKind kind;
    bool             has_expected_return_type;
    CheckedType      expected_return_type;
    AstSourceSpan    owner_span;
    AstSourceSpan    related_span;
    bool             has_related_span;
} BlockContext;

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size);
static const char *primitive_type_name(AstPrimitiveType primitive);
static const char *binary_operator_name(AstBinaryOperator operator);
static const char *assignment_operator_name(AstAssignmentOperator operator);
static const char *block_context_name(BlockContextKind kind);
static CheckedType checked_type_invalid(void);
static CheckedType checked_type_void(void);
static CheckedType checked_type_null(void);
static CheckedType checked_type_external(void);
static CheckedType checked_type_value(AstPrimitiveType primitive, size_t array_depth);
static bool checked_type_equals(CheckedType left, CheckedType right);
static bool checked_type_is_scalar_value(CheckedType type);
static bool checked_type_is_bool(CheckedType type);
static bool checked_type_is_string(CheckedType type);
static bool checked_type_is_numeric(CheckedType type);
static bool checked_type_is_integral(CheckedType type);
static bool checked_type_is_reference_like(CheckedType type);
static bool primitive_is_float(AstPrimitiveType primitive);
static bool primitive_is_integral(AstPrimitiveType primitive);
static int primitive_width(AstPrimitiveType primitive);
static bool primitive_is_signed(AstPrimitiveType primitive);
static AstPrimitiveType signed_primitive_for_width(int width);
static AstPrimitiveType unsigned_primitive_for_width(int width);
static CheckedType promote_numeric_types(CheckedType left, CheckedType right);
static CheckedType checked_type_from_resolved_type(ResolvedType type);
static CheckedType checked_type_from_ast_type(TypeChecker *checker, const AstType *type);
static CheckedType checked_type_from_cast_target(TypeChecker *checker,
                                                 const AstExpression *expression);
static bool checked_type_assignable(CheckedType target, CheckedType source);
static bool merge_types_for_inference(CheckedType left, CheckedType right,
                                      CheckedType *merged);
static bool merge_return_types(CheckedType left, CheckedType right,
                               CheckedType *merged);
static TypeCheckInfo type_check_info_make(CheckedType type);
static TypeCheckInfo type_check_info_make_callable(CheckedType return_type,
                                                   const AstParameterList *parameters);
static TypeCheckInfo type_check_info_make_external_value(void);
static TypeCheckInfo type_check_info_make_external_callable(void);
static CheckedType type_check_source_type(const TypeCheckInfo *info);
static const AstSourceSpan *block_context_related_span(const BlockContext *context,
                                                       AstSourceSpan primary_span);
static bool source_span_is_valid(AstSourceSpan span);
static void type_checker_set_error(TypeChecker *checker, const char *format, ...);
static void type_checker_set_error_at(TypeChecker *checker,
                                      AstSourceSpan primary_span,
                                      const AstSourceSpan *related_span,
                                      const char *format, ...);
static TypeCheckExpressionEntry *ensure_expression_entry(TypeChecker *checker,
                                                         const AstExpression *expression);
static TypeCheckSymbolEntry *ensure_symbol_entry(TypeChecker *checker,
                                                 const Symbol *symbol);
static const TypeCheckInfo *store_expression_info(TypeChecker *checker,
                                                  const AstExpression *expression,
                                                  TypeCheckInfo info);
static bool validate_program_start_decls(TypeChecker *checker,
                                         const AstProgram *program);
static const TypeCheckInfo *resolve_symbol_info(TypeChecker *checker,
                                                const Symbol *symbol);
static bool resolve_binding_symbol(TypeChecker *checker,
                                   const Symbol *symbol,
                                   const AstType *declared_type,
                                   bool is_inferred_type,
                                   const AstExpression *initializer,
                                   TypeCheckInfo *info);
static bool resolve_parameters_in_scope(TypeChecker *checker,
                                        const AstParameterList *parameters,
                                        const Scope *scope);
static const TypeCheckInfo *check_lambda_expression(TypeChecker *checker,
                                                    const AstExpression *expression,
                                                    const CheckedType *expected_return_type,
                                                    const AstSourceSpan *related_span);
static bool check_start_decl(TypeChecker *checker, const AstStartDecl *start_decl);
static bool check_block(TypeChecker *checker, const AstBlock *block,
                        const BlockContext *context,
                        CheckedType *return_type, AstSourceSpan *return_span);
static bool check_binary_operator(TypeChecker *checker,
                                  const AstExpression *expression,
                                  AstBinaryOperator operator,
                                  CheckedType left_type,
                                  CheckedType right_type,
                                  CheckedType *result_type);
static bool map_compound_assignment(AstAssignmentOperator operator,
                                    AstBinaryOperator *binary_operator);
static bool expression_is_assignment_target(TypeChecker *checker,
                                            const AstExpression *expression,
                                            const Symbol **root_symbol);
static const TypeCheckInfo *check_expression(TypeChecker *checker,
                                             const AstExpression *expression);

void type_checker_init(TypeChecker *checker) {
    if (!checker) {
        return;
    }

    memset(checker, 0, sizeof(*checker));
}

void type_checker_free(TypeChecker *checker) {
    if (!checker) {
        return;
    }

    type_resolver_free(&checker->resolver);
    free(checker->expression_entries);
    free(checker->symbol_entries);
    memset(checker, 0, sizeof(*checker));
}

bool type_checker_check_program(TypeChecker *checker,
                                const AstProgram *program,
                                const SymbolTable *symbols) {
    const SymbolTableError *symbol_error;
    const UnresolvedIdentifier *unresolved;
    const TypeResolutionError *resolution_error;
    const Scope *root_scope;
    size_t i;

    if (!checker || !program || !symbols) {
        return false;
    }

    type_checker_free(checker);
    type_checker_init(checker);
    checker->program = program;
    checker->symbols = symbols;

    symbol_error = symbol_table_get_error(symbols);
    if (symbol_error) {
        checker->has_error = true;
        checker->error.primary_span = symbol_error->primary_span;
        checker->error.related_span = symbol_error->related_span;
        checker->error.has_related_span = symbol_error->has_related_span;
        strncpy(checker->error.message, symbol_error->message,
                sizeof(checker->error.message) - 1);
        checker->error.message[sizeof(checker->error.message) - 1] = '\0';
        return false;
    }

    unresolved = symbol_table_get_unresolved_identifier(symbols, 0);
    if (unresolved) {
        type_checker_set_error_at(checker, unresolved->source_span, NULL,
                                  "Unresolved identifier '%s'.",
                                  (unresolved->identifier &&
                                   unresolved->identifier->kind == AST_EXPR_IDENTIFIER &&
                                   unresolved->identifier->as.identifier)
                                      ? unresolved->identifier->as.identifier
                                      : "<unknown>");
        return false;
    }

    if (!type_resolver_resolve_program(&checker->resolver, program)) {
        resolution_error = type_resolver_get_error(&checker->resolver);
        checker->has_error = true;
        if (resolution_error) {
            checker->error.primary_span = resolution_error->primary_span;
            checker->error.related_span = resolution_error->related_span;
            checker->error.has_related_span = resolution_error->has_related_span;
            strncpy(checker->error.message,
                    resolution_error->message,
                    sizeof(checker->error.message) - 1);
            checker->error.message[sizeof(checker->error.message) - 1] = '\0';
        } else {
            strncpy(checker->error.message,
                    "Type resolution failed.",
                    sizeof(checker->error.message) - 1);
            checker->error.message[sizeof(checker->error.message) - 1] = '\0';
        }
        return false;
    }

    if (!validate_program_start_decls(checker, program)) {
        return false;
    }

    root_scope = symbol_table_root_scope(symbols);
    if (!root_scope) {
        type_checker_set_error(checker, "Internal error: missing root scope.");
        return false;
    }

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (decl->kind == AST_TOP_LEVEL_BINDING) {
            const Symbol *symbol = scope_lookup_local(root_scope, decl->as.binding_decl.name);

            if (!symbol) {
                type_checker_set_error_at(checker,
                                          decl->as.binding_decl.name_span,
                                          NULL,
                                          "Internal error: missing symbol for '%s'.",
                                          decl->as.binding_decl.name);
                return false;
            }

            if (!resolve_symbol_info(checker, symbol)) {
                return false;
            }
        } else if (!check_start_decl(checker, &decl->as.start_decl)) {
            return false;
        }
    }

    return !checker->has_error;
}

const TypeCheckError *type_checker_get_error(const TypeChecker *checker) {
    if (!checker || !checker->has_error) {
        return NULL;
    }

    return &checker->error;
}

bool type_checker_format_error(const TypeCheckError *error,
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

const TypeCheckInfo *type_checker_get_expression_info(const TypeChecker *checker,
                                                      const AstExpression *expression) {
    size_t i;

    if (!checker || !expression) {
        return NULL;
    }

    for (i = 0; i < checker->expression_count; i++) {
        if (checker->expression_entries[i].expression == expression) {
            return &checker->expression_entries[i].info;
        }
    }

    return NULL;
}

const TypeCheckInfo *type_checker_get_symbol_info(const TypeChecker *checker,
                                                  const Symbol *symbol) {
    size_t i;

    if (!checker || !symbol) {
        return NULL;
    }

    for (i = 0; i < checker->symbol_count; i++) {
        if (checker->symbol_entries[i].symbol == symbol &&
            checker->symbol_entries[i].is_resolved) {
            return &checker->symbol_entries[i].info;
        }
    }

    return NULL;
}

bool checked_type_to_string(CheckedType type, char *buffer, size_t buffer_size) {
    size_t i;
    int written;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    switch (type.kind) {
    case CHECKED_TYPE_INVALID:
        return snprintf(buffer, buffer_size, "<invalid>") >= 0;
    case CHECKED_TYPE_VOID:
        return snprintf(buffer, buffer_size, "void") >= 0;
    case CHECKED_TYPE_NULL:
        return snprintf(buffer, buffer_size, "null") >= 0;
    case CHECKED_TYPE_EXTERNAL:
        return snprintf(buffer, buffer_size, "<external>") >= 0;
    case CHECKED_TYPE_VALUE:
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

static const char *binary_operator_name(AstBinaryOperator operator) {
    switch (operator) {
    case AST_BINARY_OP_LOGICAL_OR:
        return "||";
    case AST_BINARY_OP_LOGICAL_AND:
        return "&&";
    case AST_BINARY_OP_BIT_OR:
        return "|";
    case AST_BINARY_OP_BIT_NAND:
        return "~&";
    case AST_BINARY_OP_BIT_XOR:
        return "^";
    case AST_BINARY_OP_BIT_XNOR:
        return "~^";
    case AST_BINARY_OP_BIT_AND:
        return "&";
    case AST_BINARY_OP_EQUAL:
        return "==";
    case AST_BINARY_OP_NOT_EQUAL:
        return "!=";
    case AST_BINARY_OP_LESS:
        return "<";
    case AST_BINARY_OP_GREATER:
        return ">";
    case AST_BINARY_OP_LESS_EQUAL:
        return "<=";
    case AST_BINARY_OP_GREATER_EQUAL:
        return ">=";
    case AST_BINARY_OP_SHIFT_LEFT:
        return "<<";
    case AST_BINARY_OP_SHIFT_RIGHT:
        return ">>";
    case AST_BINARY_OP_ADD:
        return "+";
    case AST_BINARY_OP_SUBTRACT:
        return "-";
    case AST_BINARY_OP_MULTIPLY:
        return "*";
    case AST_BINARY_OP_DIVIDE:
        return "/";
    case AST_BINARY_OP_MODULO:
        return "%";
    }

    return "?";
}

static const char *assignment_operator_name(AstAssignmentOperator operator) {
    switch (operator) {
    case AST_ASSIGN_OP_ASSIGN:
        return "=";
    case AST_ASSIGN_OP_ADD:
        return "+=";
    case AST_ASSIGN_OP_SUBTRACT:
        return "-=";
    case AST_ASSIGN_OP_MULTIPLY:
        return "*=";
    case AST_ASSIGN_OP_DIVIDE:
        return "/=";
    case AST_ASSIGN_OP_MODULO:
        return "%=";
    case AST_ASSIGN_OP_BIT_AND:
        return "&=";
    case AST_ASSIGN_OP_BIT_OR:
        return "|=";
    case AST_ASSIGN_OP_BIT_XOR:
        return "^=";
    case AST_ASSIGN_OP_SHIFT_LEFT:
        return "<<=";
    case AST_ASSIGN_OP_SHIFT_RIGHT:
        return ">>=";
    }

    return "?=";
}

static const char *block_context_name(BlockContextKind kind) {
    switch (kind) {
    case BLOCK_CONTEXT_LAMBDA:
        return "lambda body";
    case BLOCK_CONTEXT_START:
        return "start";
    }

    return "block";
}

static CheckedType checked_type_invalid(void) {
    CheckedType type;

    memset(&type, 0, sizeof(type));
    type.kind = CHECKED_TYPE_INVALID;
    return type;
}

static CheckedType checked_type_void(void) {
    CheckedType type = checked_type_invalid();
    type.kind = CHECKED_TYPE_VOID;
    return type;
}

static CheckedType checked_type_null(void) {
    CheckedType type = checked_type_invalid();
    type.kind = CHECKED_TYPE_NULL;
    return type;
}

static CheckedType checked_type_external(void) {
    CheckedType type = checked_type_invalid();
    type.kind = CHECKED_TYPE_EXTERNAL;
    return type;
}

static CheckedType checked_type_value(AstPrimitiveType primitive, size_t array_depth) {
    CheckedType type = checked_type_invalid();
    type.kind = CHECKED_TYPE_VALUE;
    type.primitive = primitive;
    type.array_depth = array_depth;
    return type;
}

static bool checked_type_equals(CheckedType left, CheckedType right) {
    return left.kind == right.kind &&
           left.primitive == right.primitive &&
           left.array_depth == right.array_depth;
}

static bool checked_type_is_scalar_value(CheckedType type) {
    return type.kind == CHECKED_TYPE_VALUE && type.array_depth == 0;
}

static bool checked_type_is_bool(CheckedType type) {
    return checked_type_is_scalar_value(type) && type.primitive == AST_PRIMITIVE_BOOL;
}

static bool checked_type_is_string(CheckedType type) {
    return checked_type_is_scalar_value(type) && type.primitive == AST_PRIMITIVE_STRING;
}

static bool checked_type_is_numeric(CheckedType type) {
    return checked_type_is_scalar_value(type) && primitive_is_integral(type.primitive);
}

static bool checked_type_is_integral(CheckedType type) {
    return checked_type_is_scalar_value(type) &&
           primitive_is_integral(type.primitive) &&
           !primitive_is_float(type.primitive);
}

static bool checked_type_is_reference_like(CheckedType type) {
    return (type.kind == CHECKED_TYPE_VALUE &&
            (type.array_depth > 0 || type.primitive == AST_PRIMITIVE_STRING)) ||
           type.kind == CHECKED_TYPE_EXTERNAL;
}

static bool primitive_is_float(AstPrimitiveType primitive) {
    return primitive == AST_PRIMITIVE_FLOAT32 || primitive == AST_PRIMITIVE_FLOAT64;
}

static bool primitive_is_integral(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_INT64:
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_UINT16:
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_UINT64:
    case AST_PRIMITIVE_FLOAT32:
    case AST_PRIMITIVE_FLOAT64:
    case AST_PRIMITIVE_CHAR:
        return true;
    case AST_PRIMITIVE_BOOL:
    case AST_PRIMITIVE_STRING:
        return false;
    }

    return false;
}

static int primitive_width(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_CHAR:
        return 8;
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_UINT16:
        return 16;
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_FLOAT32:
        return 32;
    case AST_PRIMITIVE_INT64:
    case AST_PRIMITIVE_UINT64:
    case AST_PRIMITIVE_FLOAT64:
        return 64;
    case AST_PRIMITIVE_BOOL:
    case AST_PRIMITIVE_STRING:
        return 0;
    }

    return 0;
}

static bool primitive_is_signed(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_INT64:
    case AST_PRIMITIVE_FLOAT32:
    case AST_PRIMITIVE_FLOAT64:
    case AST_PRIMITIVE_CHAR:
        return true;
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_UINT16:
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_UINT64:
    case AST_PRIMITIVE_BOOL:
    case AST_PRIMITIVE_STRING:
        return false;
    }

    return false;
}

static AstPrimitiveType signed_primitive_for_width(int width) {
    if (width <= 8) {
        return AST_PRIMITIVE_INT8;
    }
    if (width <= 16) {
        return AST_PRIMITIVE_INT16;
    }
    if (width <= 32) {
        return AST_PRIMITIVE_INT32;
    }
    return AST_PRIMITIVE_INT64;
}

static AstPrimitiveType unsigned_primitive_for_width(int width) {
    if (width <= 8) {
        return AST_PRIMITIVE_UINT8;
    }
    if (width <= 16) {
        return AST_PRIMITIVE_UINT16;
    }
    if (width <= 32) {
        return AST_PRIMITIVE_UINT32;
    }
    return AST_PRIMITIVE_UINT64;
}

static CheckedType promote_numeric_types(CheckedType left, CheckedType right) {
    int width;

    if (!checked_type_is_numeric(left) || !checked_type_is_numeric(right)) {
        return checked_type_invalid();
    }

    if (primitive_is_float(left.primitive) || primitive_is_float(right.primitive)) {
        if (left.primitive == AST_PRIMITIVE_FLOAT64 || right.primitive == AST_PRIMITIVE_FLOAT64) {
            return checked_type_value(AST_PRIMITIVE_FLOAT64, 0);
        }
        return checked_type_value(AST_PRIMITIVE_FLOAT32, 0);
    }

    width = primitive_width(left.primitive);
    if (primitive_width(right.primitive) > width) {
        width = primitive_width(right.primitive);
    }

    if (primitive_is_signed(left.primitive) == primitive_is_signed(right.primitive)) {
        return primitive_is_signed(left.primitive)
                   ? checked_type_value(signed_primitive_for_width(width), 0)
                   : checked_type_value(unsigned_primitive_for_width(width), 0);
    }

    if (width < 64) {
        return checked_type_value(AST_PRIMITIVE_INT64, 0);
    }

    if (!primitive_is_signed(left.primitive) || !primitive_is_signed(right.primitive)) {
        return checked_type_value(AST_PRIMITIVE_UINT64, 0);
    }

    return checked_type_value(AST_PRIMITIVE_INT64, 0);
}

static CheckedType checked_type_from_resolved_type(ResolvedType type) {
    switch (type.kind) {
    case RESOLVED_TYPE_INVALID:
        return checked_type_invalid();
    case RESOLVED_TYPE_VOID:
        return checked_type_void();
    case RESOLVED_TYPE_VALUE:
        return checked_type_value(type.primitive, type.array_depth);
    }

    return checked_type_invalid();
}

static CheckedType checked_type_from_ast_type(TypeChecker *checker, const AstType *type) {
    const ResolvedType *resolved_type;

    if (!checker || !type) {
        return checked_type_invalid();
    }

    resolved_type = type_resolver_get_type(&checker->resolver, type);
    if (!resolved_type) {
        type_checker_set_error(checker, "Internal error: missing resolved source type.");
        return checked_type_invalid();
    }

    return checked_type_from_resolved_type(*resolved_type);
}

static CheckedType checked_type_from_cast_target(TypeChecker *checker,
                                                 const AstExpression *expression) {
    const ResolvedType *resolved_type;

    if (!checker || !expression) {
        return checked_type_invalid();
    }

    resolved_type = type_resolver_get_cast_target_type(&checker->resolver, expression);
    if (!resolved_type) {
        type_checker_set_error(checker,
                               "Internal error: missing resolved cast target type.");
        return checked_type_invalid();
    }

    return checked_type_from_resolved_type(*resolved_type);
}

static bool checked_type_assignable(CheckedType target, CheckedType source) {
    if (target.kind == CHECKED_TYPE_INVALID || source.kind == CHECKED_TYPE_INVALID) {
        return false;
    }

    if (checked_type_equals(target, source)) {
        return true;
    }

    if (target.kind == CHECKED_TYPE_VOID) {
        return source.kind == CHECKED_TYPE_VOID ||
               source.kind == CHECKED_TYPE_NULL ||
               source.kind == CHECKED_TYPE_EXTERNAL;
    }

    if (source.kind == CHECKED_TYPE_VOID) {
        return false;
    }

    if (target.kind == CHECKED_TYPE_EXTERNAL) {
        return source.kind != CHECKED_TYPE_VOID;
    }

    if (source.kind == CHECKED_TYPE_EXTERNAL) {
        return target.kind != CHECKED_TYPE_VOID;
    }

    if (target.kind == CHECKED_TYPE_VALUE && source.kind == CHECKED_TYPE_NULL) {
        return checked_type_is_reference_like(target);
    }

    if (target.kind == CHECKED_TYPE_VALUE && source.kind == CHECKED_TYPE_VALUE) {
        if (target.array_depth != source.array_depth) {
            return false;
        }

        if (target.array_depth == 0 &&
            checked_type_is_numeric(target) && checked_type_is_numeric(source)) {
            return true;
        }
    }

    return false;
}

static bool merge_types_for_inference(CheckedType left, CheckedType right,
                                      CheckedType *merged) {
    if (!merged) {
        return false;
    }

    if (checked_type_equals(left, right)) {
        *merged = left;
        return true;
    }

    if (left.kind == CHECKED_TYPE_EXTERNAL && right.kind == CHECKED_TYPE_EXTERNAL) {
        *merged = checked_type_external();
        return true;
    }

    if (left.kind == CHECKED_TYPE_VALUE && right.kind == CHECKED_TYPE_VALUE &&
        left.array_depth == 0 && right.array_depth == 0 &&
        checked_type_is_numeric(left) && checked_type_is_numeric(right)) {
        *merged = promote_numeric_types(left, right);
        return true;
    }

    return false;
}

static bool merge_return_types(CheckedType left, CheckedType right,
                               CheckedType *merged) {
    if ((left.kind == CHECKED_TYPE_VOID && right.kind == CHECKED_TYPE_NULL) ||
        (left.kind == CHECKED_TYPE_NULL && right.kind == CHECKED_TYPE_VOID)) {
        *merged = checked_type_void();
        return true;
    }

    return merge_types_for_inference(left, right, merged);
}

static TypeCheckInfo type_check_info_make(CheckedType type) {
    TypeCheckInfo info;

    memset(&info, 0, sizeof(info));
    info.type = type;
    return info;
}

static TypeCheckInfo type_check_info_make_callable(CheckedType return_type,
                                                   const AstParameterList *parameters) {
    TypeCheckInfo info = type_check_info_make(return_type);
    info.is_callable = true;
    info.callable_return_type = return_type;
    info.parameters = parameters;
    return info;
}

static TypeCheckInfo type_check_info_make_external_value(void) {
    return type_check_info_make(checked_type_external());
}

static TypeCheckInfo type_check_info_make_external_callable(void) {
    TypeCheckInfo info = type_check_info_make_callable(checked_type_external(), NULL);
    info.type = checked_type_external();
    return info;
}

static CheckedType type_check_source_type(const TypeCheckInfo *info) {
    if (!info) {
        return checked_type_invalid();
    }

    return info->is_callable ? info->callable_return_type : info->type;
}

static const AstSourceSpan *block_context_related_span(const BlockContext *context,
                                                       AstSourceSpan primary_span) {
    if (!context || !context->has_related_span ||
        !source_span_is_valid(context->related_span)) {
        return NULL;
    }

    if (source_span_is_valid(primary_span) &&
        primary_span.start_line == context->related_span.start_line &&
        primary_span.start_column == context->related_span.start_column) {
        return NULL;
    }

    return &context->related_span;
}

static bool source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

static void type_checker_set_error(TypeChecker *checker, const char *format, ...) {
    va_list args;

    if (!checker || checker->has_error) {
        return;
    }

    checker->has_error = true;
    va_start(args, format);
    vsnprintf(checker->error.message, sizeof(checker->error.message), format, args);
    va_end(args);
}

static void type_checker_set_error_at(TypeChecker *checker,
                                      AstSourceSpan primary_span,
                                      const AstSourceSpan *related_span,
                                      const char *format, ...) {
    va_list args;

    if (!checker || checker->has_error) {
        return;
    }

    checker->has_error = true;
    checker->error.primary_span = primary_span;
    if (related_span && source_span_is_valid(*related_span)) {
        checker->error.related_span = *related_span;
        checker->error.has_related_span = true;
    }

    va_start(args, format);
    vsnprintf(checker->error.message, sizeof(checker->error.message), format, args);
    va_end(args);
}

static TypeCheckExpressionEntry *ensure_expression_entry(TypeChecker *checker,
                                                         const AstExpression *expression) {
    size_t i;

    for (i = 0; i < checker->expression_count; i++) {
        if (checker->expression_entries[i].expression == expression) {
            return &checker->expression_entries[i];
        }
    }

    if (!reserve_items((void **)&checker->expression_entries,
                       &checker->expression_capacity,
                       checker->expression_count + 1,
                       sizeof(*checker->expression_entries))) {
        type_checker_set_error(checker,
                               "Out of memory while storing expression type information.");
        return NULL;
    }

    checker->expression_entries[checker->expression_count].expression = expression;
    memset(&checker->expression_entries[checker->expression_count].info, 0,
           sizeof(checker->expression_entries[checker->expression_count].info));
    checker->expression_count++;
    return &checker->expression_entries[checker->expression_count - 1];
}

static TypeCheckSymbolEntry *ensure_symbol_entry(TypeChecker *checker,
                                                 const Symbol *symbol) {
    size_t i;

    for (i = 0; i < checker->symbol_count; i++) {
        if (checker->symbol_entries[i].symbol == symbol) {
            return &checker->symbol_entries[i];
        }
    }

    if (!reserve_items((void **)&checker->symbol_entries,
                       &checker->symbol_capacity,
                       checker->symbol_count + 1,
                       sizeof(*checker->symbol_entries))) {
        type_checker_set_error(checker,
                               "Out of memory while storing symbol type information.");
        return NULL;
    }

    checker->symbol_entries[checker->symbol_count].symbol = symbol;
    memset(&checker->symbol_entries[checker->symbol_count].info, 0,
           sizeof(checker->symbol_entries[checker->symbol_count].info));
    checker->symbol_entries[checker->symbol_count].is_resolved = false;
    checker->symbol_entries[checker->symbol_count].is_resolving = false;
    checker->symbol_count++;
    return &checker->symbol_entries[checker->symbol_count - 1];
}

static const TypeCheckInfo *store_expression_info(TypeChecker *checker,
                                                  const AstExpression *expression,
                                                  TypeCheckInfo info) {
    TypeCheckExpressionEntry *entry = ensure_expression_entry(checker, expression);

    if (!entry) {
        return NULL;
    }

    entry->info = info;
    return &entry->info;
}

static bool validate_program_start_decls(TypeChecker *checker,
                                         const AstProgram *program) {
    const AstStartDecl *first_start = NULL;
    const AstStartDecl *duplicate_start = NULL;
    size_t i;

    if (!checker || !program) {
        return false;
    }

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (!decl || decl->kind != AST_TOP_LEVEL_START) {
            continue;
        }

        if (!first_start) {
            first_start = &decl->as.start_decl;
        } else {
            duplicate_start = &decl->as.start_decl;
            break;
        }
    }

    if (!first_start) {
        type_checker_set_error(checker,
                               "Program must declare exactly one start entry point.");
        return false;
    }

    if (duplicate_start) {
        type_checker_set_error_at(checker,
                                  duplicate_start->start_span,
                                  &first_start->start_span,
                                  "Program cannot declare multiple start entry points.");
        return false;
    }

    return true;
}

static const TypeCheckInfo *resolve_symbol_info(TypeChecker *checker,
                                                const Symbol *symbol) {
    TypeCheckSymbolEntry *entry;
    TypeCheckInfo resolved_info;

    if (!checker || !symbol) {
        return NULL;
    }

    entry = ensure_symbol_entry(checker, symbol);
    if (!entry) {
        return NULL;
    }

    if (entry->is_resolved) {
        return &entry->info;
    }

    if (entry->is_resolving) {
        type_checker_set_error_at(checker, symbol->declaration_span, NULL,
                                  "Circular definition involving '%s'.",
                                  symbol->name ? symbol->name : "<anonymous>");
        return NULL;
    }

    entry->is_resolving = true;
    resolved_info = type_check_info_make(checked_type_invalid());

    switch (symbol->kind) {
    case SYMBOL_KIND_PACKAGE:
    case SYMBOL_KIND_IMPORT:
        resolved_info = type_check_info_make_external_value();
        break;

    case SYMBOL_KIND_PARAMETER:
        resolved_info = type_check_info_make(checked_type_from_ast_type(checker,
                                                                        symbol->declared_type));
        if (checker->has_error) {
            entry->is_resolving = false;
            return NULL;
        }
        if (resolved_info.type.kind == CHECKED_TYPE_VOID) {
            type_checker_set_error_at(checker, symbol->declaration_span, NULL,
                                      "Parameter '%s' cannot have type void.",
                                      symbol->name ? symbol->name : "<anonymous>");
            entry->is_resolving = false;
            return NULL;
        }
        break;

    case SYMBOL_KIND_TOP_LEVEL_BINDING:
        if (!resolve_binding_symbol(checker,
                                    symbol,
                                    &((const AstBindingDecl *)symbol->declaration)->declared_type,
                                    ((const AstBindingDecl *)symbol->declaration)->is_inferred_type,
                                    ((const AstBindingDecl *)symbol->declaration)->initializer,
                                    &resolved_info)) {
            entry->is_resolving = false;
            return NULL;
        }
        break;

    case SYMBOL_KIND_LOCAL:
        if (!resolve_binding_symbol(checker,
                                    symbol,
                                    &((const AstLocalBindingStatement *)symbol->declaration)->declared_type,
                                    ((const AstLocalBindingStatement *)symbol->declaration)->is_inferred_type,
                                    ((const AstLocalBindingStatement *)symbol->declaration)->initializer,
                                    &resolved_info)) {
            entry->is_resolving = false;
            return NULL;
        }
        break;
    }

    entry->info = resolved_info;
    entry->is_resolving = false;
    entry->is_resolved = true;
    return &entry->info;
}

static bool resolve_binding_symbol(TypeChecker *checker,
                                   const Symbol *symbol,
                                   const AstType *declared_type,
                                   bool is_inferred_type,
                                   const AstExpression *initializer,
                                   TypeCheckInfo *info) {
    const TypeCheckInfo *initializer_info;
    CheckedType target_type;
    CheckedType source_type;
    char source_text[64];
    char target_text[64];

    if (is_inferred_type) {
        initializer_info = check_expression(checker, initializer);
        if (!initializer_info) {
            return false;
        }

        if (!initializer_info->is_callable &&
            (initializer_info->type.kind == CHECKED_TYPE_VOID ||
             initializer_info->type.kind == CHECKED_TYPE_NULL)) {
            type_checker_set_error_at(checker,
                                      initializer ? initializer->source_span : symbol->declaration_span,
                                      &symbol->declaration_span,
                                      "Cannot infer a type for %s '%s' from %s.",
                                      symbol_kind_name(symbol->kind),
                                      symbol->name ? symbol->name : "<anonymous>",
                                      initializer_info->type.kind == CHECKED_TYPE_VOID
                                          ? "a void expression"
                                          : "null");
            return false;
        }

        *info = *initializer_info;
        return true;
    }

    target_type = checked_type_from_ast_type(checker, declared_type);
    if (checker->has_error) {
        return false;
    }

    if (initializer && initializer->kind == AST_EXPR_LAMBDA) {
        initializer_info = check_lambda_expression(checker,
                                                   initializer,
                                                   &target_type,
                                                   &symbol->declaration_span);
    } else {
        initializer_info = check_expression(checker, initializer);
    }
    if (!initializer_info) {
        return false;
    }

    if (target_type.kind == CHECKED_TYPE_VOID && !initializer_info->is_callable) {
        type_checker_set_error_at(checker, symbol->declaration_span, NULL,
                                  "%s '%s' cannot have type void unless initialized with a callable expression.",
                                  symbol_kind_name(symbol->kind),
                                  symbol->name ? symbol->name : "<anonymous>");
        return false;
    }

    source_type = type_check_source_type(initializer_info);
    if (!(initializer_info->is_callable && source_type.kind == CHECKED_TYPE_EXTERNAL) &&
        !checked_type_assignable(target_type, source_type)) {
        checked_type_to_string(source_type, source_text, sizeof(source_text));
        checked_type_to_string(target_type, target_text, sizeof(target_text));
        type_checker_set_error_at(checker,
                                  initializer ? initializer->source_span : symbol->declaration_span,
                                  &symbol->declaration_span,
                                  "Cannot assign expression of type %s to %s '%s' of type %s.",
                                  source_text,
                                  symbol_kind_name(symbol->kind),
                                  symbol->name ? symbol->name : "<anonymous>",
                                  target_text);
        return false;
    }

    *info = *initializer_info;
    info->type = target_type;
    if (initializer_info->is_callable) {
        info->callable_return_type = target_type;
    }

    return true;
}

static bool resolve_parameters_in_scope(TypeChecker *checker,
                                        const AstParameterList *parameters,
                                        const Scope *scope) {
    size_t i;

    if (!parameters || !scope) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        const Symbol *symbol = scope_lookup_local(scope, parameters->items[i].name);

        if (!symbol) {
            type_checker_set_error_at(checker,
                                      parameters->items[i].name_span,
                                      NULL,
                                      "Internal error: missing parameter symbol '%s'.",
                                      parameters->items[i].name);
            return false;
        }

        if (!resolve_symbol_info(checker, symbol)) {
            return false;
        }
    }

    return true;
}

static const TypeCheckInfo *check_lambda_expression(TypeChecker *checker,
                                                    const AstExpression *expression,
                                                    const CheckedType *expected_return_type,
                                                    const AstSourceSpan *related_span) {
    const Scope *lambda_scope;
    CheckedType return_type;
    AstSourceSpan return_span;
    TypeCheckInfo info;

    if (!checker || !expression || expression->kind != AST_EXPR_LAMBDA) {
        return NULL;
    }

    if (!expected_return_type) {
        const TypeCheckInfo *cached_info = type_checker_get_expression_info(checker, expression);

        if (cached_info) {
            return cached_info;
        }
    }

    lambda_scope = symbol_table_find_scope(checker->symbols, expression, SCOPE_KIND_LAMBDA);
    if (!lambda_scope) {
        type_checker_set_error_at(checker,
                                  expression->source_span,
                                  NULL,
                                  "Internal error: missing lambda scope.");
        return NULL;
    }

    if (!resolve_parameters_in_scope(checker,
                                     &expression->as.lambda.parameters,
                                     lambda_scope)) {
        return NULL;
    }

    memset(&return_span, 0, sizeof(return_span));
    if (expression->as.lambda.body.kind == AST_LAMBDA_BODY_BLOCK) {
        BlockContext context;

        memset(&context, 0, sizeof(context));
        context.kind = BLOCK_CONTEXT_LAMBDA;
        context.has_expected_return_type = (expected_return_type != NULL);
        if (expected_return_type) {
            context.expected_return_type = *expected_return_type;
        }
        context.owner_span = expression->source_span;
        if (related_span && source_span_is_valid(*related_span)) {
            context.related_span = *related_span;
            context.has_related_span = true;
        }

        if (!check_block(checker,
                         expression->as.lambda.body.as.block,
                         &context,
                         &return_type,
                         &return_span)) {
            return NULL;
        }
    } else {
        const TypeCheckInfo *body_info = check_expression(checker,
                                                          expression->as.lambda.body.as.expression);

        if (!body_info) {
            return NULL;
        }

        return_type = type_check_source_type(body_info);
        return_span = expression->as.lambda.body.as.expression->source_span;

        if (expected_return_type &&
            !checked_type_assignable(*expected_return_type, return_type)) {
            char expected_text[64];
            char actual_text[64];

            checked_type_to_string(*expected_return_type,
                                   expected_text,
                                   sizeof(expected_text));
            checked_type_to_string(return_type, actual_text, sizeof(actual_text));
            type_checker_set_error_at(checker,
                                      return_span,
                                      related_span,
                                      "Lambda body must produce %s but got %s.",
                                      expected_text,
                                      actual_text);
            return NULL;
        }
    }

    info = type_check_info_make_callable(return_type, &expression->as.lambda.parameters);
    return store_expression_info(checker, expression, info);
}

static bool check_start_decl(TypeChecker *checker, const AstStartDecl *start_decl) {
    const Scope *start_scope;
    CheckedType body_type;
    AstSourceSpan body_span;
    CheckedType expected_type = checked_type_value(AST_PRIMITIVE_INT32, 0);
    char body_text[64];

    start_scope = symbol_table_find_scope(checker->symbols, start_decl, SCOPE_KIND_START);
    if (!start_scope) {
        type_checker_set_error_at(checker, start_decl->start_span, NULL,
                                  "Internal error: missing start scope.");
        return false;
    }

    if (!resolve_parameters_in_scope(checker, &start_decl->parameters, start_scope)) {
        return false;
    }

    if (start_decl->parameters.count != 1) {
        type_checker_set_error_at(checker,
                                  start_decl->start_span,
                                  NULL,
                                  "start must declare exactly one parameter of type string[].");
        return false;
    }

    if (start_decl->parameters.items[0].type.kind != AST_TYPE_PRIMITIVE ||
        start_decl->parameters.items[0].type.primitive != AST_PRIMITIVE_STRING ||
        start_decl->parameters.items[0].type.dimension_count != 1 ||
        start_decl->parameters.items[0].type.dimensions[0].has_size) {
        type_checker_set_error_at(checker,
                                  start_decl->parameters.items[0].name_span,
                                  &start_decl->start_span,
                                  "start parameter must have type string[].");
        return false;
    }

    body_span = start_decl->start_span;
    if (start_decl->body.kind == AST_LAMBDA_BODY_BLOCK) {
        BlockContext context;

        memset(&context, 0, sizeof(context));
        context.kind = BLOCK_CONTEXT_START;
        context.has_expected_return_type = true;
        context.expected_return_type = expected_type;
        context.owner_span = start_decl->start_span;
        context.related_span = start_decl->start_span;
        context.has_related_span = true;

        if (!check_block(checker,
                         start_decl->body.as.block,
                         &context,
                         &body_type,
                         &body_span)) {
            return false;
        }
    } else {
        const TypeCheckInfo *body_info = check_expression(checker, start_decl->body.as.expression);

        if (!body_info) {
            return false;
        }

        body_type = type_check_source_type(body_info);
        body_span = start_decl->body.as.expression->source_span;
    }

    if (!checked_type_assignable(expected_type, body_type)) {
        checked_type_to_string(body_type, body_text, sizeof(body_text));
        type_checker_set_error_at(checker,
                                  body_span,
                                  &start_decl->start_span,
                                  "start body must produce int32 but got %s.",
                                  body_text);
        return false;
    }

    return true;
}

static bool check_block(TypeChecker *checker, const AstBlock *block,
                        const BlockContext *context,
                        CheckedType *return_type, AstSourceSpan *return_span) {
    const Scope *block_scope;
    CheckedType current_return_type = checked_type_void();
    AstSourceSpan first_return_span;
    bool saw_return = false;
    size_t i;

    memset(&first_return_span, 0, sizeof(first_return_span));
    block_scope = symbol_table_find_scope(checker->symbols, block, SCOPE_KIND_BLOCK);
    if (!block_scope) {
        type_checker_set_error(checker, "Internal error: missing block scope.");
        return false;
    }

    for (i = 0; i < block->statement_count; i++) {
        const AstStatement *statement = block->statements[i];

        switch (statement->kind) {
        case AST_STMT_LOCAL_BINDING:
            {
                const Symbol *symbol = scope_lookup_local(block_scope,
                                                          statement->as.local_binding.name);

                if (!symbol) {
                    type_checker_set_error_at(checker,
                                              statement->as.local_binding.name_span,
                                              NULL,
                                              "Internal error: missing local symbol '%s'.",
                                              statement->as.local_binding.name);
                    return false;
                }

                if (!resolve_symbol_info(checker, symbol)) {
                    return false;
                }
            }
            break;

        case AST_STMT_RETURN:
            {
                CheckedType statement_return_type;
                AstSourceSpan statement_span;

                if (statement->as.return_expression) {
                    const TypeCheckInfo *info = check_expression(checker,
                                                                 statement->as.return_expression);
                    if (!info) {
                        return false;
                    }

                    statement_return_type = type_check_source_type(info);
                    statement_span = statement->as.return_expression->source_span;
                } else {
                    statement_return_type = checked_type_void();
                    statement_span = statement->source_span;
                }

                if (context && context->has_expected_return_type) {
                    char expected_text[64];

                    checked_type_to_string(context->expected_return_type,
                                           expected_text,
                                           sizeof(expected_text));

                    if (!statement->as.return_expression &&
                        context->expected_return_type.kind != CHECKED_TYPE_VOID) {
                        type_checker_set_error_at(checker,
                                                  statement->source_span,
                                                  block_context_related_span(context,
                                                                             statement->source_span),
                                                  "Return statement in %s must produce %s.",
                                                  block_context_name(context->kind),
                                                  expected_text);
                        return false;
                    }

                    if (statement->as.return_expression &&
                        !checked_type_assignable(context->expected_return_type,
                                                 statement_return_type)) {
                        char actual_text[64];

                        checked_type_to_string(statement_return_type,
                                               actual_text,
                                               sizeof(actual_text));
                        type_checker_set_error_at(checker,
                                                  statement->as.return_expression->source_span,
                                                  block_context_related_span(context,
                                                                             statement->as.return_expression->source_span),
                                                  "Return statement in %s must produce %s but got %s.",
                                                  block_context_name(context->kind),
                                                  expected_text,
                                                  actual_text);
                        return false;
                    }
                }

                if (!saw_return) {
                    current_return_type = statement_return_type;
                    first_return_span = statement_span;
                    saw_return = true;
                } else {
                    CheckedType merged_type;
                    char first_text[64];
                    char second_text[64];

                    if (!merge_return_types(current_return_type,
                                            statement_return_type,
                                            &merged_type)) {
                        checked_type_to_string(current_return_type,
                                               first_text,
                                               sizeof(first_text));
                        checked_type_to_string(statement_return_type,
                                               second_text,
                                               sizeof(second_text));
                        type_checker_set_error_at(checker,
                                                  statement_span,
                                                  source_span_is_valid(first_return_span)
                                                      ? &first_return_span
                                                      : NULL,
                                                  "Inconsistent return types in block: %s and %s.",
                                                  first_text,
                                                  second_text);
                        return false;
                    }

                    current_return_type = merged_type;
                }
            }
            break;

        case AST_STMT_EXIT:
            if (context &&
                (context->kind != BLOCK_CONTEXT_LAMBDA ||
                 (context->has_expected_return_type &&
                  context->expected_return_type.kind != CHECKED_TYPE_VOID))) {
                type_checker_set_error_at(checker,
                                          statement->source_span,
                                          block_context_related_span(context,
                                                                     statement->source_span),
                                          "exit is only permitted in void-typed lambda blocks.");
                return false;
            }

            if (!saw_return) {
                current_return_type = checked_type_void();
                first_return_span = statement->source_span;
                saw_return = true;
            } else {
                CheckedType merged_type;

                if (!merge_return_types(current_return_type,
                                        checked_type_void(),
                                        &merged_type)) {
                    char first_text[64];
                    char second_text[64];

                    checked_type_to_string(current_return_type, first_text, sizeof(first_text));
                    checked_type_to_string(checked_type_void(), second_text, sizeof(second_text));
                    type_checker_set_error_at(checker,
                                              first_return_span,
                                              NULL,
                                              "Inconsistent return types in block: %s and %s.",
                                              first_text,
                                              second_text);
                    return false;
                }

                current_return_type = merged_type;
            }
            break;

        case AST_STMT_THROW:
            if (statement->as.throw_expression &&
                !check_expression(checker, statement->as.throw_expression)) {
                return false;
            }
            break;

        case AST_STMT_EXPRESSION:
            if (statement->as.expression &&
                !check_expression(checker, statement->as.expression)) {
                return false;
            }
            break;
        }
    }

    if (context && context->has_expected_return_type &&
        !checked_type_assignable(context->expected_return_type,
                                 current_return_type)) {
        AstSourceSpan primary_span = source_span_is_valid(first_return_span)
                                         ? first_return_span
                                         : context->owner_span;
        char expected_text[64];
        char actual_text[64];

        checked_type_to_string(context->expected_return_type,
                               expected_text,
                               sizeof(expected_text));
        checked_type_to_string(current_return_type,
                               actual_text,
                               sizeof(actual_text));
        type_checker_set_error_at(checker,
                                  primary_span,
                                  block_context_related_span(context, primary_span),
                                  "%s body must produce %s but got %s.",
                                  context->kind == BLOCK_CONTEXT_START ? "start" : "Lambda",
                                  expected_text,
                                  actual_text);
        return false;
    }

    if (return_type) {
        *return_type = current_return_type;
    }
    if (return_span) {
        *return_span = first_return_span;
    }

    return true;
}

static bool check_binary_operator(TypeChecker *checker,
                                  const AstExpression *expression,
                                  AstBinaryOperator operator,
                                  CheckedType left_type,
                                  CheckedType right_type,
                                  CheckedType *result_type) {
    char left_text[64];
    char right_text[64];

    if (!result_type) {
        return false;
    }

    switch (operator) {
    case AST_BINARY_OP_LOGICAL_OR:
    case AST_BINARY_OP_LOGICAL_AND:
        if (checked_type_is_bool(left_type) && checked_type_is_bool(right_type)) {
            *result_type = checked_type_value(AST_PRIMITIVE_BOOL, 0);
            return true;
        }
        break;

    case AST_BINARY_OP_BIT_OR:
    case AST_BINARY_OP_BIT_NAND:
    case AST_BINARY_OP_BIT_XOR:
    case AST_BINARY_OP_BIT_XNOR:
    case AST_BINARY_OP_BIT_AND:
        if (checked_type_is_integral(left_type) && checked_type_is_integral(right_type)) {
            *result_type = promote_numeric_types(left_type, right_type);
            return true;
        }
        break;

    case AST_BINARY_OP_EQUAL:
    case AST_BINARY_OP_NOT_EQUAL:
        if (checked_type_equals(left_type, right_type) ||
            (checked_type_is_numeric(left_type) && checked_type_is_numeric(right_type)) ||
            ((left_type.kind == CHECKED_TYPE_NULL && checked_type_is_reference_like(right_type)) ||
             (right_type.kind == CHECKED_TYPE_NULL && checked_type_is_reference_like(left_type))) ||
            (left_type.kind == CHECKED_TYPE_EXTERNAL || right_type.kind == CHECKED_TYPE_EXTERNAL)) {
            *result_type = checked_type_value(AST_PRIMITIVE_BOOL, 0);
            return true;
        }
        break;

    case AST_BINARY_OP_LESS:
    case AST_BINARY_OP_GREATER:
    case AST_BINARY_OP_LESS_EQUAL:
    case AST_BINARY_OP_GREATER_EQUAL:
        if (checked_type_is_numeric(left_type) && checked_type_is_numeric(right_type)) {
            *result_type = checked_type_value(AST_PRIMITIVE_BOOL, 0);
            return true;
        }
        break;

    case AST_BINARY_OP_SHIFT_LEFT:
    case AST_BINARY_OP_SHIFT_RIGHT:
        if (checked_type_is_integral(left_type) && checked_type_is_integral(right_type)) {
            *result_type = left_type;
            return true;
        }
        break;

    case AST_BINARY_OP_ADD:
        if (checked_type_is_string(left_type) && checked_type_is_string(right_type)) {
            *result_type = checked_type_value(AST_PRIMITIVE_STRING, 0);
            return true;
        }
        if (checked_type_is_numeric(left_type) && checked_type_is_numeric(right_type)) {
            *result_type = promote_numeric_types(left_type, right_type);
            return true;
        }
        break;

    case AST_BINARY_OP_SUBTRACT:
    case AST_BINARY_OP_MULTIPLY:
    case AST_BINARY_OP_DIVIDE:
        if (checked_type_is_numeric(left_type) && checked_type_is_numeric(right_type)) {
            *result_type = promote_numeric_types(left_type, right_type);
            return true;
        }
        break;

    case AST_BINARY_OP_MODULO:
        if (checked_type_is_integral(left_type) && checked_type_is_integral(right_type)) {
            *result_type = promote_numeric_types(left_type, right_type);
            return true;
        }
        break;
    }

    checked_type_to_string(left_type, left_text, sizeof(left_text));
    checked_type_to_string(right_type, right_text, sizeof(right_text));
    type_checker_set_error_at(checker,
                              expression->source_span,
                              NULL,
                              "Operator '%s' cannot be applied to types %s and %s.",
                              binary_operator_name(operator),
                              left_text,
                              right_text);
    return false;
}

static bool map_compound_assignment(AstAssignmentOperator operator,
                                    AstBinaryOperator *binary_operator) {
    if (!binary_operator) {
        return false;
    }

    switch (operator) {
    case AST_ASSIGN_OP_ADD:
        *binary_operator = AST_BINARY_OP_ADD;
        return true;
    case AST_ASSIGN_OP_SUBTRACT:
        *binary_operator = AST_BINARY_OP_SUBTRACT;
        return true;
    case AST_ASSIGN_OP_MULTIPLY:
        *binary_operator = AST_BINARY_OP_MULTIPLY;
        return true;
    case AST_ASSIGN_OP_DIVIDE:
        *binary_operator = AST_BINARY_OP_DIVIDE;
        return true;
    case AST_ASSIGN_OP_MODULO:
        *binary_operator = AST_BINARY_OP_MODULO;
        return true;
    case AST_ASSIGN_OP_BIT_AND:
        *binary_operator = AST_BINARY_OP_BIT_AND;
        return true;
    case AST_ASSIGN_OP_BIT_OR:
        *binary_operator = AST_BINARY_OP_BIT_OR;
        return true;
    case AST_ASSIGN_OP_BIT_XOR:
        *binary_operator = AST_BINARY_OP_BIT_XOR;
        return true;
    case AST_ASSIGN_OP_SHIFT_LEFT:
        *binary_operator = AST_BINARY_OP_SHIFT_LEFT;
        return true;
    case AST_ASSIGN_OP_SHIFT_RIGHT:
        *binary_operator = AST_BINARY_OP_SHIFT_RIGHT;
        return true;
    case AST_ASSIGN_OP_ASSIGN:
        return false;
    }

    return false;
}

static bool expression_is_assignment_target(TypeChecker *checker,
                                            const AstExpression *expression,
                                            const Symbol **root_symbol) {
    if (root_symbol) {
        *root_symbol = NULL;
    }

    if (!checker || !expression) {
        return false;
    }

    switch (expression->kind) {
    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *symbol = symbol_table_resolve_identifier(checker->symbols, expression);

            if (root_symbol) {
                *root_symbol = symbol;
            }

            return symbol &&
                   (symbol->kind == SYMBOL_KIND_TOP_LEVEL_BINDING ||
                    symbol->kind == SYMBOL_KIND_PARAMETER ||
                    symbol->kind == SYMBOL_KIND_LOCAL);
        }

    case AST_EXPR_INDEX:
        {
            const TypeCheckInfo *target_info = type_checker_get_expression_info(checker,
                                                                                expression->as.index.target);

            if (!target_info) {
                return false;
            }

            if (type_check_source_type(target_info).kind == CHECKED_TYPE_EXTERNAL) {
                return true;
            }

            return expression_is_assignment_target(checker,
                                                   expression->as.index.target,
                                                   root_symbol);
        }

    case AST_EXPR_MEMBER:
        {
            const TypeCheckInfo *target_info = type_checker_get_expression_info(checker,
                                                                                expression->as.member.target);

            if (!target_info) {
                return false;
            }

            if (type_check_source_type(target_info).kind == CHECKED_TYPE_EXTERNAL) {
                return true;
            }

            return expression_is_assignment_target(checker,
                                                   expression->as.member.target,
                                                   root_symbol);
        }

    case AST_EXPR_GROUPING:
        return expression_is_assignment_target(checker,
                                               expression->as.grouping.inner,
                                               root_symbol);

    default:
        return false;
    }
}

static const TypeCheckInfo *check_expression(TypeChecker *checker,
                                             const AstExpression *expression) {
    TypeCheckInfo info;
    const TypeCheckInfo *cached_info;

    if (!checker || !expression) {
        return NULL;
    }

    cached_info = type_checker_get_expression_info(checker, expression);
    if (cached_info) {
        return cached_info;
    }

    info = type_check_info_make(checked_type_invalid());

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        switch (expression->as.literal.kind) {
        case AST_LITERAL_INTEGER:
            info = type_check_info_make(checked_type_value(AST_PRIMITIVE_INT32, 0));
            break;
        case AST_LITERAL_FLOAT:
            info = type_check_info_make(checked_type_value(AST_PRIMITIVE_FLOAT64, 0));
            break;
        case AST_LITERAL_BOOL:
            info = type_check_info_make(checked_type_value(AST_PRIMITIVE_BOOL, 0));
            break;
        case AST_LITERAL_CHAR:
            info = type_check_info_make(checked_type_value(AST_PRIMITIVE_CHAR, 0));
            break;
        case AST_LITERAL_STRING:
        case AST_LITERAL_TEMPLATE:
            if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
                size_t i;

                for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                    const AstTemplatePart *part =
                        &expression->as.literal.as.template_parts.items[i];

                    if (part->kind == AST_TEMPLATE_PART_EXPRESSION) {
                        const TypeCheckInfo *part_info = check_expression(checker,
                                                                         part->as.expression);

                        if (!part_info) {
                            return NULL;
                        }
                        if (part_info->is_callable &&
                            part_info->parameters &&
                            part_info->parameters->count == 0 &&
                            part_info->callable_return_type.kind == CHECKED_TYPE_VOID) {
                            type_checker_set_error_at(
                                checker,
                                part->as.expression->source_span,
                                NULL,
                                "Template interpolation cannot auto-call a zero-argument callable returning void.");
                            return NULL;
                        }
                        if (type_check_source_type(part_info).kind == CHECKED_TYPE_VOID) {
                            type_checker_set_error_at(checker,
                                                      part->as.expression->source_span,
                                                      NULL,
                                                      "Template interpolation cannot use a void expression.");
                            return NULL;
                        }
                    }
                }
            }
            info = type_check_info_make(checked_type_value(AST_PRIMITIVE_STRING, 0));
            break;
        case AST_LITERAL_NULL:
            info = type_check_info_make(checked_type_null());
            break;
        }
        break;

    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *symbol = symbol_table_resolve_identifier(checker->symbols, expression);

            if (!symbol) {
                type_checker_set_error_at(checker,
                                          expression->source_span,
                                          NULL,
                                          "Unresolved identifier '%s'.",
                                          expression->as.identifier ? expression->as.identifier : "<unknown>");
                return NULL;
            }

            cached_info = resolve_symbol_info(checker, symbol);
            if (!cached_info) {
                return NULL;
            }

            info = *cached_info;
        }
        break;

    case AST_EXPR_LAMBDA:
        return check_lambda_expression(checker, expression, NULL, NULL);

    case AST_EXPR_ASSIGNMENT:
        {
            const TypeCheckInfo *target_info;
            const TypeCheckInfo *value_info;
            const Symbol *target_symbol = NULL;
            CheckedType source_type;
            bool assignable;

            target_info = check_expression(checker, expression->as.assignment.target);
            if (!target_info) {
                return NULL;
            }

            value_info = check_expression(checker, expression->as.assignment.value);
            if (!value_info) {
                return NULL;
            }

            if (!expression_is_assignment_target(checker,
                                                 expression->as.assignment.target,
                                                 &target_symbol)) {
                const AstSourceSpan *related_span = NULL;

                if (target_symbol && source_span_is_valid(target_symbol->declaration_span)) {
                    related_span = &target_symbol->declaration_span;
                }

                type_checker_set_error_at(checker,
                                          expression->as.assignment.target->source_span,
                                          related_span,
                                          "Operator '%s' requires an assignable target.",
                                          assignment_operator_name(expression->as.assignment.operator));
                return NULL;
            }

            if (target_symbol && target_symbol->is_final) {
                type_checker_set_error_at(checker,
                                          expression->as.assignment.target->source_span,
                                          &target_symbol->declaration_span,
                                          "Cannot assign to final symbol '%s'.",
                                          target_symbol->name ? target_symbol->name : "<anonymous>");
                return NULL;
            }

            source_type = type_check_source_type(value_info);
            if (expression->as.assignment.operator == AST_ASSIGN_OP_ASSIGN) {
                assignable = (value_info->is_callable && source_type.kind == CHECKED_TYPE_EXTERNAL) ||
                             checked_type_assignable(target_info->type, source_type);
            } else {
                AstBinaryOperator binary_operator;
                CheckedType result_type;

                if (!map_compound_assignment(expression->as.assignment.operator, &binary_operator) ||
                    !check_binary_operator(checker,
                                           expression,
                                           binary_operator,
                                           target_info->type,
                                           source_type,
                                           &result_type)) {
                    return NULL;
                }

                assignable = checked_type_assignable(target_info->type, result_type);
            }

            if (!assignable) {
                char source_text[64];
                char target_text[64];

                checked_type_to_string(source_type, source_text, sizeof(source_text));
                checked_type_to_string(target_info->type, target_text, sizeof(target_text));
                type_checker_set_error_at(checker,
                                          expression->as.assignment.value->source_span,
                                          &expression->as.assignment.target->source_span,
                                          "Operator '%s' cannot assign %s to %s.",
                                          assignment_operator_name(expression->as.assignment.operator),
                                          source_text,
                                          target_text);
                return NULL;
            }

            info = *value_info;
            info.type = target_info->type;
            if (value_info->is_callable) {
                info.callable_return_type = target_info->type;
            }
        }
        break;

    case AST_EXPR_TERNARY:
        {
            const TypeCheckInfo *condition_info = check_expression(checker,
                                                                   expression->as.ternary.condition);
            const TypeCheckInfo *then_info;
            const TypeCheckInfo *else_info;
            CheckedType merged_type;
            char then_text[64];
            char else_text[64];

            if (!condition_info) {
                return NULL;
            }

            if (!checked_type_is_bool(type_check_source_type(condition_info))) {
                char condition_text[64];

                checked_type_to_string(type_check_source_type(condition_info),
                                       condition_text,
                                       sizeof(condition_text));
                type_checker_set_error_at(checker,
                                          expression->as.ternary.condition->source_span,
                                          NULL,
                                          "Ternary condition must have type bool but got %s.",
                                          condition_text);
                return NULL;
            }

            then_info = check_expression(checker, expression->as.ternary.then_branch);
            else_info = check_expression(checker, expression->as.ternary.else_branch);
            if (!then_info || !else_info) {
                return NULL;
            }

            if (!merge_types_for_inference(type_check_source_type(then_info),
                                           type_check_source_type(else_info),
                                           &merged_type)) {
                checked_type_to_string(type_check_source_type(then_info),
                                       then_text,
                                       sizeof(then_text));
                checked_type_to_string(type_check_source_type(else_info),
                                       else_text,
                                       sizeof(else_text));
                type_checker_set_error_at(checker,
                                          expression->as.ternary.else_branch->source_span,
                                          &expression->as.ternary.then_branch->source_span,
                                          "Ternary branches must have compatible types, but got %s and %s.",
                                          then_text,
                                          else_text);
                return NULL;
            }

            info = type_check_info_make(merged_type);
        }
        break;

    case AST_EXPR_BINARY:
        {
            const TypeCheckInfo *left_info = check_expression(checker, expression->as.binary.left);
            const TypeCheckInfo *right_info = check_expression(checker, expression->as.binary.right);
            CheckedType result_type;

            if (!left_info || !right_info) {
                return NULL;
            }

            if (!check_binary_operator(checker,
                                       expression,
                                       expression->as.binary.operator,
                                       type_check_source_type(left_info),
                                       type_check_source_type(right_info),
                                       &result_type)) {
                return NULL;
            }

            info = type_check_info_make(result_type);
        }
        break;

    case AST_EXPR_UNARY:
        {
            const TypeCheckInfo *operand_info = check_expression(checker,
                                                                 expression->as.unary.operand);
            CheckedType operand_type;
            char operand_text[64];

            if (!operand_info) {
                return NULL;
            }

            operand_type = type_check_source_type(operand_info);
            switch (expression->as.unary.operator) {
            case AST_UNARY_OP_LOGICAL_NOT:
                if (!checked_type_is_bool(operand_type)) {
                    checked_type_to_string(operand_type, operand_text, sizeof(operand_text));
                    type_checker_set_error_at(checker,
                                              expression->source_span,
                                              NULL,
                                              "Operator '!' requires bool but got %s.",
                                              operand_text);
                    return NULL;
                }
                info = type_check_info_make(checked_type_value(AST_PRIMITIVE_BOOL, 0));
                break;

            case AST_UNARY_OP_BITWISE_NOT:
                if (!checked_type_is_integral(operand_type)) {
                    checked_type_to_string(operand_type, operand_text, sizeof(operand_text));
                    type_checker_set_error_at(checker,
                                              expression->source_span,
                                              NULL,
                                              "Operator '~' requires an integral operand but got %s.",
                                              operand_text);
                    return NULL;
                }
                info = type_check_info_make(operand_type);
                break;

            case AST_UNARY_OP_NEGATE:
            case AST_UNARY_OP_PLUS:
                if (!checked_type_is_numeric(operand_type)) {
                    checked_type_to_string(operand_type, operand_text, sizeof(operand_text));
                    type_checker_set_error_at(checker,
                                              expression->source_span,
                                              NULL,
                                              "Unary operator requires a numeric operand but got %s.",
                                              operand_text);
                    return NULL;
                }
                info = type_check_info_make(operand_type);
                break;
            }
        }
        break;

    case AST_EXPR_CALL:
        {
            const TypeCheckInfo *callee_info = check_expression(checker,
                                                                expression->as.call.callee);
            size_t i;

            if (!callee_info) {
                return NULL;
            }

            if (!callee_info->is_callable) {
                char callee_text[64];

                checked_type_to_string(callee_info->type, callee_text, sizeof(callee_text));
                type_checker_set_error_at(checker,
                                          expression->as.call.callee->source_span,
                                          NULL,
                                          "Expression of type %s is not callable.",
                                          callee_text);
                return NULL;
            }

            if (callee_info->parameters &&
                callee_info->parameters->count != expression->as.call.arguments.count) {
                type_checker_set_error_at(checker,
                                          expression->source_span,
                                          NULL,
                                          "Call expects %zu argument%s but got %zu.",
                                          callee_info->parameters->count,
                                          callee_info->parameters->count == 1 ? "" : "s",
                                          expression->as.call.arguments.count);
                return NULL;
            }

            for (i = 0; i < expression->as.call.arguments.count; i++) {
                const TypeCheckInfo *argument_info = check_expression(checker,
                                                                      expression->as.call.arguments.items[i]);

                if (!argument_info) {
                    return NULL;
                }

                if (callee_info->parameters) {
                    CheckedType parameter_type = checked_type_from_ast_type(checker,
                        &callee_info->parameters->items[i].type);
                    CheckedType argument_type = type_check_source_type(argument_info);

                    if (checker->has_error) {
                        return NULL;
                    }

                    if (!checked_type_assignable(parameter_type, argument_type)) {
                        char expected_text[64];
                        char actual_text[64];

                        checked_type_to_string(parameter_type,
                                               expected_text,
                                               sizeof(expected_text));
                        checked_type_to_string(argument_type,
                                               actual_text,
                                               sizeof(actual_text));
                        type_checker_set_error_at(checker,
                                                  expression->as.call.arguments.items[i]->source_span,
                                                  &callee_info->parameters->items[i].name_span,
                                                  "Argument %zu to call expects %s but got %s.",
                                                  i + 1,
                                                  expected_text,
                                                  actual_text);
                        return NULL;
                    }
                } else if (type_check_source_type(argument_info).kind == CHECKED_TYPE_VOID) {
                    type_checker_set_error_at(checker,
                                              expression->as.call.arguments.items[i]->source_span,
                                              NULL,
                                              "Call arguments cannot have type void.");
                    return NULL;
                }
            }

            info = type_check_info_make(callee_info->callable_return_type);
        }
        break;

    case AST_EXPR_INDEX:
        {
            const TypeCheckInfo *target_info = check_expression(checker,
                                                                expression->as.index.target);
            const TypeCheckInfo *index_info = check_expression(checker,
                                                               expression->as.index.index);
            CheckedType target_type;
            CheckedType index_type;
            char target_text[64];
            char index_text[64];

            if (!target_info || !index_info) {
                return NULL;
            }

            target_type = type_check_source_type(target_info);
            index_type = type_check_source_type(index_info);
            if (!checked_type_is_integral(index_type)) {
                checked_type_to_string(index_type, index_text, sizeof(index_text));
                type_checker_set_error_at(checker,
                                          expression->as.index.index->source_span,
                                          NULL,
                                          "Index expression must have an integral type but got %s.",
                                          index_text);
                return NULL;
            }

            if (target_type.kind == CHECKED_TYPE_EXTERNAL) {
                info = type_check_info_make_external_value();
                break;
            }

            if (target_type.kind == CHECKED_TYPE_VALUE && target_type.array_depth > 0) {
                info = type_check_info_make(checked_type_value(target_type.primitive,
                                                               target_type.array_depth - 1));
                break;
            }

            checked_type_to_string(target_type, target_text, sizeof(target_text));
            type_checker_set_error_at(checker,
                                      expression->as.index.target->source_span,
                                      NULL,
                                      "Cannot index expression of type %s.",
                                      target_text);
            return NULL;
        }

    case AST_EXPR_MEMBER:
        {
            const TypeCheckInfo *target_info = check_expression(checker,
                                                                expression->as.member.target);
            CheckedType target_type;
            char target_text[64];

            if (!target_info) {
                return NULL;
            }

            target_type = type_check_source_type(target_info);
            if (target_type.kind == CHECKED_TYPE_EXTERNAL) {
                info = type_check_info_make_external_callable();
                break;
            }

            checked_type_to_string(target_type, target_text, sizeof(target_text));
            type_checker_set_error_at(checker,
                                      expression->as.member.target->source_span,
                                      NULL,
                                      "Member access requires an imported or external target, but got %s.",
                                      target_text);
            return NULL;
        }

    case AST_EXPR_CAST:
        {
            const TypeCheckInfo *operand_info = check_expression(checker,
                                                                 expression->as.cast.expression);
            CheckedType operand_type;
            CheckedType target_type;
            char operand_text[64];
            char target_text[64];

            if (!operand_info) {
                return NULL;
            }

            operand_type = type_check_source_type(operand_info);
            target_type = checked_type_from_cast_target(checker, expression);
            if (checker->has_error) {
                return NULL;
            }
            if (operand_type.kind == CHECKED_TYPE_EXTERNAL || checked_type_is_numeric(operand_type)) {
                info = type_check_info_make(target_type);
                break;
            }

            checked_type_to_string(operand_type, operand_text, sizeof(operand_text));
            checked_type_to_string(target_type,
                                   target_text,
                                   sizeof(target_text));
            type_checker_set_error_at(checker,
                                      expression->as.cast.expression->source_span,
                                      NULL,
                                      "Cannot cast expression of type %s to %s.",
                                      operand_text,
                                      target_text);
            return NULL;
        }

    case AST_EXPR_ARRAY_LITERAL:
        if (expression->as.array_literal.elements.count == 0) {
            type_checker_set_error_at(checker,
                                      expression->source_span,
                                      NULL,
                                      "Cannot infer the element type of an empty array literal.");
            return NULL;
        }
        {
            CheckedType element_type = checked_type_invalid();
            size_t i;

            for (i = 0; i < expression->as.array_literal.elements.count; i++) {
                const TypeCheckInfo *element_info = check_expression(checker,
                                                                     expression->as.array_literal.elements.items[i]);
                CheckedType candidate_type;

                if (!element_info) {
                    return NULL;
                }

                candidate_type = type_check_source_type(element_info);
                if (candidate_type.kind == CHECKED_TYPE_VOID ||
                    candidate_type.kind == CHECKED_TYPE_NULL) {
                    type_checker_set_error_at(checker,
                                              expression->as.array_literal.elements.items[i]->source_span,
                                              NULL,
                                              "Array literal elements cannot have type %s.",
                                              candidate_type.kind == CHECKED_TYPE_VOID
                                                  ? "void"
                                                  : "null");
                    return NULL;
                }

                if (i == 0) {
                    element_type = candidate_type;
                } else if (!merge_types_for_inference(element_type,
                                                      candidate_type,
                                                      &element_type)) {
                    char left_text[64];
                    char right_text[64];

                    checked_type_to_string(element_type, left_text, sizeof(left_text));
                    checked_type_to_string(candidate_type, right_text, sizeof(right_text));
                    type_checker_set_error_at(checker,
                                              expression->as.array_literal.elements.items[i]->source_span,
                                              &expression->as.array_literal.elements.items[0]->source_span,
                                              "Array literal elements must have compatible types, but got %s and %s.",
                                              left_text,
                                              right_text);
                    return NULL;
                }
            }

            if (element_type.kind != CHECKED_TYPE_VALUE) {
                type_checker_set_error_at(checker,
                                          expression->source_span,
                                          NULL,
                                          "Array literal element type cannot be inferred from external values.");
                return NULL;
            }

            info = type_check_info_make(checked_type_value(element_type.primitive,
                                                           element_type.array_depth + 1));
        }
        break;

    case AST_EXPR_GROUPING:
        {
            const TypeCheckInfo *inner_info = check_expression(checker,
                                                               expression->as.grouping.inner);
            if (!inner_info) {
                return NULL;
            }
            info = *inner_info;
        }
        break;
    }

    return store_expression_info(checker, expression, info);
}