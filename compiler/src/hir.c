#include "hir.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    HirProgram         *program;
    const AstProgram   *ast_program;
    const SymbolTable  *symbols;
    const TypeChecker  *checker;
} HirBuildContext;

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size);
static bool source_span_is_valid(AstSourceSpan span);
static void hir_set_error(HirBuildContext *context,
                          AstSourceSpan primary_span,
                          const AstSourceSpan *related_span,
                          const char *format,
                          ...);
static char *qualified_name_to_string(const AstQualifiedName *name);
static bool append_named_symbol(HirNamedSymbol **items,
                                size_t *count,
                                size_t *capacity,
                                HirNamedSymbol symbol);
static bool append_top_level_decl(HirProgram *program, HirTopLevelDecl *decl);
static bool append_parameter(HirParameterList *list, HirParameter parameter);
static bool append_template_part(HirTemplatePartList *list, HirTemplatePart part);
static bool append_argument(HirCallExpression *call, HirExpression *argument);
static bool append_array_element(HirArrayLiteralExpression *array_literal,
                                 HirExpression *element);
static bool append_statement(HirBlock *block, HirStatement *statement);
static void free_callable_signature(HirCallableSignature *signature);
static void free_named_symbol(HirNamedSymbol *symbol);
static void free_parameter_list(HirParameterList *list);
static void free_template_part_list(HirTemplatePartList *list);
static void free_call_expression(HirCallExpression *call);
static void free_array_literal_expression(HirArrayLiteralExpression *array_literal);
void hir_block_free(HirBlock *block);
void hir_statement_free(HirStatement *statement);
void hir_expression_free(HirExpression *expression);
static HirBlock *hir_block_new(void);
static HirStatement *hir_statement_new(HirStatementKind kind);
static HirExpression *hir_expression_new(HirExpressionKind kind);
static HirTopLevelDecl *hir_top_level_decl_new(HirTopLevelDeclKind kind);
static HirBlock *lower_block(HirBuildContext *context, const AstBlock *block);
static HirStatement *lower_statement(HirBuildContext *context,
                                     const AstStatement *statement,
                                     const Scope *scope);
static HirExpression *lower_expression(HirBuildContext *context,
                                       const AstExpression *expression);
static HirBlock *lower_body_to_block(HirBuildContext *context,
                                     const AstLambdaBody *body);
static bool lower_parameters(HirBuildContext *context,
                             HirParameterList *out,
                             const AstParameterList *parameters,
                             const Scope *scope);
static CheckedType checked_type_from_resolved_type(ResolvedType type);
static bool lower_callable_signature(HirBuildContext *context,
                                     const TypeCheckInfo *info,
                                     HirCallableSignature *signature);
static CheckedType checked_type_void_value(void);

void hir_program_init(HirProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
}

void hir_program_free(HirProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    if (program->has_package) {
        free_named_symbol(&program->package);
    }

    for (i = 0; i < program->import_count; i++) {
        free_named_symbol(&program->imports[i]);
    }
    free(program->imports);

    for (i = 0; i < program->top_level_count; i++) {
        HirTopLevelDecl *decl = program->top_level_decls[i];

        if (!decl) {
            continue;
        }

        if (decl->kind == HIR_TOP_LEVEL_BINDING) {
            free(decl->as.binding.name);
            free_callable_signature(&decl->as.binding.callable_signature);
            if (decl->as.binding.initializer) {
                hir_expression_free(decl->as.binding.initializer);
            }
        } else {
            free_parameter_list(&decl->as.start.parameters);
            if (decl->as.start.body) {
                hir_block_free(decl->as.start.body);
            }
        }

        free(decl);
    }
    free(program->top_level_decls);
    memset(program, 0, sizeof(*program));
}

static void free_named_symbol(HirNamedSymbol *symbol) {
    if (!symbol) {
        return;
    }

    free(symbol->name);
    free(symbol->qualified_name);
    memset(symbol, 0, sizeof(*symbol));
}

static void free_callable_signature(HirCallableSignature *signature) {
    if (!signature) {
        return;
    }

    free(signature->parameter_types);
    memset(signature, 0, sizeof(*signature));
}

static void free_parameter_list(HirParameterList *list) {
    size_t i;

    if (!list) {
        return;
    }

    for (i = 0; i < list->count; i++) {
        free(list->items[i].name);
    }

    free(list->items);
    memset(list, 0, sizeof(*list));
}

static void free_template_part_list(HirTemplatePartList *list) {
    size_t i;

    if (!list) {
        return;
    }

    for (i = 0; i < list->count; i++) {
        if (list->items[i].kind == AST_TEMPLATE_PART_TEXT) {
            free(list->items[i].as.text);
        } else if (list->items[i].as.expression) {
            hir_expression_free(list->items[i].as.expression);
        }
    }

    free(list->items);
    memset(list, 0, sizeof(*list));
}

static void free_call_expression(HirCallExpression *call) {
    size_t i;

    if (!call) {
        return;
    }

    if (call->callee) {
        hir_expression_free(call->callee);
    }
    for (i = 0; i < call->argument_count; i++) {
        hir_expression_free(call->arguments[i]);
    }
    free(call->arguments);
    memset(call, 0, sizeof(*call));
}

static void free_array_literal_expression(HirArrayLiteralExpression *array_literal) {
    size_t i;

    if (!array_literal) {
        return;
    }

    for (i = 0; i < array_literal->element_count; i++) {
        hir_expression_free(array_literal->elements[i]);
    }
    free(array_literal->elements);
    memset(array_literal, 0, sizeof(*array_literal));
}

void hir_block_free(HirBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    for (i = 0; i < block->statement_count; i++) {
        hir_statement_free(block->statements[i]);
    }
    free(block->statements);
    free(block);
}

void hir_statement_free(HirStatement *statement) {
    if (!statement) {
        return;
    }

    switch (statement->kind) {
    case HIR_STMT_LOCAL_BINDING:
        free(statement->as.local_binding.name);
        if (statement->as.local_binding.is_callable) {
            free_callable_signature(&statement->as.local_binding.callable_signature);
        }
        hir_expression_free(statement->as.local_binding.initializer);
        break;
    case HIR_STMT_RETURN:
        hir_expression_free(statement->as.return_expression);
        break;
    case HIR_STMT_THROW:
        hir_expression_free(statement->as.throw_expression);
        break;
    case HIR_STMT_EXPRESSION:
        hir_expression_free(statement->as.expression);
        break;
    case HIR_STMT_EXIT:
        break;
    }

    free(statement);
}

void hir_expression_free(HirExpression *expression) {
    if (!expression) {
        return;
    }

    switch (expression->kind) {
    case HIR_EXPR_LITERAL:
        if (expression->as.literal.kind != AST_LITERAL_BOOL &&
            expression->as.literal.kind != AST_LITERAL_NULL) {
            free(expression->as.literal.as.text);
        }
        break;
    case HIR_EXPR_TEMPLATE:
        free_template_part_list(&expression->as.template_parts);
        break;
    case HIR_EXPR_SYMBOL:
        free(expression->as.symbol.name);
        break;
    case HIR_EXPR_LAMBDA:
        free_parameter_list(&expression->as.lambda.parameters);
        hir_block_free(expression->as.lambda.body);
        break;
    case HIR_EXPR_ASSIGNMENT:
        hir_expression_free(expression->as.assignment.target);
        hir_expression_free(expression->as.assignment.value);
        break;
    case HIR_EXPR_TERNARY:
        hir_expression_free(expression->as.ternary.condition);
        hir_expression_free(expression->as.ternary.then_branch);
        hir_expression_free(expression->as.ternary.else_branch);
        break;
    case HIR_EXPR_BINARY:
        hir_expression_free(expression->as.binary.left);
        hir_expression_free(expression->as.binary.right);
        break;
    case HIR_EXPR_UNARY:
        hir_expression_free(expression->as.unary.operand);
        break;
    case HIR_EXPR_CALL:
        free_call_expression(&expression->as.call);
        break;
    case HIR_EXPR_INDEX:
        hir_expression_free(expression->as.index.target);
        hir_expression_free(expression->as.index.index);
        break;
    case HIR_EXPR_MEMBER:
        hir_expression_free(expression->as.member.target);
        free(expression->as.member.member);
        break;
    case HIR_EXPR_CAST:
        hir_expression_free(expression->as.cast.expression);
        break;
    case HIR_EXPR_ARRAY_LITERAL:
        free_array_literal_expression(&expression->as.array_literal);
        break;
    }

    if (expression->is_callable) {
        free_callable_signature(&expression->callable_signature);
    }

    free(expression);
}

static HirBlock *hir_block_new(void) {
    HirBlock *block = malloc(sizeof(*block));

    if (!block) {
        return NULL;
    }

    memset(block, 0, sizeof(*block));
    return block;
}

static HirStatement *hir_statement_new(HirStatementKind kind) {
    HirStatement *statement = malloc(sizeof(*statement));

    if (!statement) {
        return NULL;
    }

    memset(statement, 0, sizeof(*statement));
    statement->kind = kind;
    return statement;
}

static HirExpression *hir_expression_new(HirExpressionKind kind) {
    HirExpression *expression = malloc(sizeof(*expression));

    if (!expression) {
        return NULL;
    }

    memset(expression, 0, sizeof(*expression));
    expression->kind = kind;
    expression->type = checked_type_void_value();
    expression->callable_signature.return_type = checked_type_void_value();
    return expression;
}

static HirTopLevelDecl *hir_top_level_decl_new(HirTopLevelDeclKind kind) {
    HirTopLevelDecl *decl = malloc(sizeof(*decl));

    if (!decl) {
        return NULL;
    }

    memset(decl, 0, sizeof(*decl));
    decl->kind = kind;
    return decl;
}

const HirBuildError *hir_get_error(const HirProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool hir_format_error(const HirBuildError *error,
                      char *buffer,
                      size_t buffer_size) {
    int written;

    if (!error || !buffer || buffer_size == 0) {
        return false;
    }

    if (source_span_is_valid(error->primary_span)) {
        if (error->has_related_span && source_span_is_valid(error->related_span)) {
            written = snprintf(buffer,
                               buffer_size,
                               "%d:%d: %s Related location at %d:%d.",
                               error->primary_span.start_line,
                               error->primary_span.start_column,
                               error->message,
                               error->related_span.start_line,
                               error->related_span.start_column);
        } else {
            written = snprintf(buffer,
                               buffer_size,
                               "%d:%d: %s",
                               error->primary_span.start_line,
                               error->primary_span.start_column,
                               error->message);
        }
    } else {
        written = snprintf(buffer, buffer_size, "%s", error->message);
    }

    return written >= 0 && (size_t)written < buffer_size;
}

static bool source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

static void hir_set_error(HirBuildContext *context,
                          AstSourceSpan primary_span,
                          const AstSourceSpan *related_span,
                          const char *format,
                          ...) {
    va_list args;

    if (!context || !context->program || context->program->has_error) {
        return;
    }

    context->program->has_error = true;
    context->program->error.primary_span = primary_span;
    if (related_span && source_span_is_valid(*related_span)) {
        context->program->error.related_span = *related_span;
        context->program->error.has_related_span = true;
    }

    va_start(args, format);
    vsnprintf(context->program->error.message,
              sizeof(context->program->error.message),
              format,
              args);
    va_end(args);
}

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size) {
    void *resized;
    size_t new_capacity;

    if (*capacity >= needed) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 4 : *capacity;
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

static char *qualified_name_to_string(const AstQualifiedName *name) {
    size_t i;
    size_t length = 0;
    char *joined;
    char *cursor;

    if (!name || name->count == 0) {
        return ast_copy_text("");
    }

    for (i = 0; i < name->count; i++) {
        length += strlen(name->segments[i]);
        if (i + 1 < name->count) {
            length += 1;
        }
    }

    joined = malloc(length + 1);
    if (!joined) {
        return NULL;
    }

    cursor = joined;
    for (i = 0; i < name->count; i++) {
        size_t segment_length = strlen(name->segments[i]);

        memcpy(cursor, name->segments[i], segment_length);
        cursor += segment_length;
        if (i + 1 < name->count) {
            *cursor++ = '.';
        }
    }
    *cursor = '\0';
    return joined;
}

static bool append_named_symbol(HirNamedSymbol **items,
                                size_t *count,
                                size_t *capacity,
                                HirNamedSymbol symbol) {
    if (!reserve_items((void **)items, capacity, *count + 1, sizeof(**items))) {
        return false;
    }

    (*items)[(*count)++] = symbol;
    return true;
}

static bool append_top_level_decl(HirProgram *program, HirTopLevelDecl *decl) {
    if (!reserve_items((void **)&program->top_level_decls,
                       &program->top_level_capacity,
                       program->top_level_count + 1,
                       sizeof(*program->top_level_decls))) {
        return false;
    }

    program->top_level_decls[program->top_level_count++] = decl;
    return true;
}

static bool append_parameter(HirParameterList *list, HirParameter parameter) {
    if (!reserve_items((void **)&list->items,
                       &list->capacity,
                       list->count + 1,
                       sizeof(*list->items))) {
        return false;
    }

    list->items[list->count++] = parameter;
    return true;
}

static bool append_template_part(HirTemplatePartList *list, HirTemplatePart part) {
    if (!reserve_items((void **)&list->items,
                       &list->capacity,
                       list->count + 1,
                       sizeof(*list->items))) {
        return false;
    }

    list->items[list->count++] = part;
    return true;
}

static bool append_argument(HirCallExpression *call, HirExpression *argument) {
    if (!reserve_items((void **)&call->arguments,
                       &call->argument_capacity,
                       call->argument_count + 1,
                       sizeof(*call->arguments))) {
        return false;
    }

    call->arguments[call->argument_count++] = argument;
    return true;
}

static bool append_array_element(HirArrayLiteralExpression *array_literal,
                                 HirExpression *element) {
    if (!reserve_items((void **)&array_literal->elements,
                       &array_literal->element_capacity,
                       array_literal->element_count + 1,
                       sizeof(*array_literal->elements))) {
        return false;
    }

    array_literal->elements[array_literal->element_count++] = element;
    return true;
}

static bool append_statement(HirBlock *block, HirStatement *statement) {
    if (!reserve_items((void **)&block->statements,
                       &block->statement_capacity,
                       block->statement_count + 1,
                       sizeof(*block->statements))) {
        return false;
    }

    block->statements[block->statement_count++] = statement;
    return true;
}

static CheckedType checked_type_from_resolved_type(ResolvedType type) {
    CheckedType checked;

    memset(&checked, 0, sizeof(checked));
    switch (type.kind) {
    case RESOLVED_TYPE_VOID:
        checked.kind = CHECKED_TYPE_VOID;
        break;
    case RESOLVED_TYPE_VALUE:
        checked.kind = CHECKED_TYPE_VALUE;
        checked.primitive = type.primitive;
        checked.array_depth = type.array_depth;
        break;
    case RESOLVED_TYPE_INVALID:
    default:
        checked.kind = CHECKED_TYPE_INVALID;
        break;
    }

    return checked;
}

static bool lower_callable_signature(HirBuildContext *context,
                                     const TypeCheckInfo *info,
                                     HirCallableSignature *signature) {
    size_t i;

    if (!signature) {
        return false;
    }

    memset(signature, 0, sizeof(*signature));
    if (!info || !info->is_callable) {
        return true;
    }

    signature->return_type = info->callable_return_type;
    if (!info->parameters) {
        return true;
    }

    signature->has_parameter_types = true;
    signature->parameter_count = info->parameters->count;
    if (signature->parameter_count == 0) {
        return true;
    }

    signature->parameter_types = calloc(signature->parameter_count,
                                        sizeof(*signature->parameter_types));
    if (!signature->parameter_types) {
        hir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering HIR callable signatures.");
        return false;
    }

    for (i = 0; i < info->parameters->count; i++) {
        const ResolvedType *resolved = type_resolver_get_type(&context->checker->resolver,
                                                              &info->parameters->items[i].type);

        if (!resolved) {
            hir_set_error(context,
                          info->parameters->items[i].name_span,
                          NULL,
                          "Internal error: missing resolved type for callable parameter '%s'.",
                          info->parameters->items[i].name);
            return false;
        }

        signature->parameter_types[i] = checked_type_from_resolved_type(*resolved);
    }

    return true;
}

static CheckedType checked_type_void_value(void) {
    CheckedType type;

    memset(&type, 0, sizeof(type));
    type.kind = CHECKED_TYPE_VOID;
    return type;
}

static bool lower_parameters(HirBuildContext *context,
                             HirParameterList *out,
                             const AstParameterList *parameters,
                             const Scope *scope) {
    size_t i;

    if (!parameters || !scope) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    for (i = 0; i < parameters->count; i++) {
        const Symbol *symbol = scope_lookup_local(scope, parameters->items[i].name);
        const TypeCheckInfo *info;
        HirParameter parameter;

        if (!symbol) {
            hir_set_error(context,
                          parameters->items[i].name_span,
                          NULL,
                          "Internal error: missing symbol for parameter '%s'.",
                          parameters->items[i].name);
            return false;
        }

        info = type_checker_get_symbol_info(context->checker, symbol);
        if (!info) {
            hir_set_error(context,
                          parameters->items[i].name_span,
                          &symbol->declaration_span,
                          "Internal error: missing type info for parameter '%s'.",
                          parameters->items[i].name);
            return false;
        }

        memset(&parameter, 0, sizeof(parameter));
        parameter.name = ast_copy_text(parameters->items[i].name);
        if (!parameter.name) {
            hir_set_error(context,
                          parameters->items[i].name_span,
                          NULL,
                          "Out of memory while lowering HIR parameters.");
            return false;
        }
        parameter.symbol = symbol;
        parameter.type = info->type;
        parameter.source_span = parameters->items[i].name_span;

        if (!append_parameter(out, parameter)) {
            free(parameter.name);
            hir_set_error(context,
                          parameters->items[i].name_span,
                          NULL,
                          "Out of memory while lowering HIR parameters.");
            return false;
        }
    }

    return true;
}

static HirBlock *lower_body_to_block(HirBuildContext *context,
                                     const AstLambdaBody *body) {
    HirBlock *block;

    if (!body) {
        return NULL;
    }

    if (body->kind == AST_LAMBDA_BODY_BLOCK) {
        return lower_block(context, body->as.block);
    }

    block = hir_block_new();
    if (!block) {
        hir_set_error(context,
                      body->as.expression ? body->as.expression->source_span : (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering HIR blocks.");
        return NULL;
    }

    if (body->as.expression) {
        HirStatement *statement = hir_statement_new(HIR_STMT_RETURN);

        if (!statement) {
            hir_block_free(block);
            hir_set_error(context,
                          body->as.expression->source_span,
                          NULL,
                          "Out of memory while lowering HIR statements.");
            return NULL;
        }

        statement->source_span = body->as.expression->source_span;
        statement->as.return_expression = lower_expression(context, body->as.expression);
        if (!statement->as.return_expression || !append_statement(block, statement)) {
            hir_statement_free(statement);
            hir_block_free(block);
            if (!context->program->has_error) {
                hir_set_error(context,
                              body->as.expression->source_span,
                              NULL,
                              "Out of memory while lowering HIR statements.");
            }
            return NULL;
        }
    }

    return block;
}

static HirStatement *lower_statement(HirBuildContext *context,
                                     const AstStatement *statement,
                                     const Scope *scope) {
    HirStatement *hir_statement;
    HirStatementKind hir_kind;

    if (!statement) {
        return NULL;
    }

    hir_kind = (statement->kind == AST_STMT_EXIT) ? HIR_STMT_RETURN
                                                  : (HirStatementKind)statement->kind;
    hir_statement = hir_statement_new(hir_kind);
    if (!hir_statement) {
        hir_set_error(context,
                      statement->source_span,
                      NULL,
                      "Out of memory while lowering HIR statements.");
        return NULL;
    }

    hir_statement->source_span = statement->source_span;
    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        {
            const Symbol *symbol = scope_lookup_local(scope, statement->as.local_binding.name);
            const TypeCheckInfo *info;

            if (!symbol) {
                hir_set_error(context,
                              statement->as.local_binding.name_span,
                              NULL,
                              "Internal error: missing symbol for local '%s'.",
                              statement->as.local_binding.name);
                hir_statement_free(hir_statement);
                return NULL;
            }

            info = type_checker_get_symbol_info(context->checker, symbol);
            if (!info) {
                hir_set_error(context,
                              statement->as.local_binding.name_span,
                              &symbol->declaration_span,
                              "Internal error: missing type info for local '%s'.",
                              statement->as.local_binding.name);
                hir_statement_free(hir_statement);
                return NULL;
            }

            hir_statement->as.local_binding.name = ast_copy_text(statement->as.local_binding.name);
            if (!hir_statement->as.local_binding.name) {
                hir_set_error(context,
                              statement->source_span,
                              NULL,
                              "Out of memory while lowering HIR locals.");
                hir_statement_free(hir_statement);
                return NULL;
            }
            hir_statement->as.local_binding.symbol = symbol;
            hir_statement->as.local_binding.type = info->type;
            hir_statement->as.local_binding.is_callable = info->is_callable;
            hir_statement->as.local_binding.is_final = symbol->is_final;
            hir_statement->as.local_binding.source_span = statement->as.local_binding.name_span;
            if (info->is_callable &&
                !lower_callable_signature(context,
                                         info,
                                         &hir_statement->as.local_binding.callable_signature)) {
                hir_statement_free(hir_statement);
                return NULL;
            }
            hir_statement->as.local_binding.initializer = lower_expression(
                context,
                statement->as.local_binding.initializer);
            if (!hir_statement->as.local_binding.initializer) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_RETURN:
        if (statement->as.return_expression) {
            hir_statement->as.return_expression = lower_expression(context,
                                                                   statement->as.return_expression);
            if (!hir_statement->as.return_expression) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_THROW:
        if (statement->as.throw_expression) {
            hir_statement->as.throw_expression = lower_expression(context,
                                                                  statement->as.throw_expression);
            if (!hir_statement->as.throw_expression) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_EXPRESSION:
        if (statement->as.expression) {
            hir_statement->as.expression = lower_expression(context, statement->as.expression);
            if (!hir_statement->as.expression) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_EXIT:
        break;
    }

    return hir_statement;
}

static HirBlock *lower_block(HirBuildContext *context, const AstBlock *block) {
    const Scope *scope;
    HirBlock *hir_block;
    size_t i;

    if (!block) {
        return NULL;
    }

    scope = symbol_table_find_scope(context->symbols, block, SCOPE_KIND_BLOCK);
    if (!scope) {
        hir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Internal error: missing scope for block during HIR lowering.");
        return NULL;
    }

    hir_block = hir_block_new();
    if (!hir_block) {
        hir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering HIR blocks.");
        return NULL;
    }

    for (i = 0; i < block->statement_count; i++) {
        HirStatement *statement = lower_statement(context, block->statements[i], scope);

        if (!statement) {
            hir_block_free(hir_block);
            return NULL;
        }
        if (!append_statement(hir_block, statement)) {
            hir_statement_free(statement);
            hir_block_free(hir_block);
            hir_set_error(context,
                          block->statements[i]->source_span,
                          NULL,
                          "Out of memory while lowering HIR statements.");
            return NULL;
        }
    }

    return hir_block;
}

static HirExpression *lower_expression(HirBuildContext *context,
                                       const AstExpression *expression) {
    const TypeCheckInfo *info;
    HirExpression *hir_expression;
    size_t i;

    if (!expression) {
        return NULL;
    }

    if (expression->kind == AST_EXPR_GROUPING) {
        return lower_expression(context, expression->as.grouping.inner);
    }

    info = type_checker_get_expression_info(context->checker, expression);
    if (!info) {
        hir_set_error(context,
                      expression->source_span,
                      NULL,
                      "Internal error: missing type info for expression during HIR lowering.");
        return NULL;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        hir_expression = hir_expression_new(expression->as.literal.kind == AST_LITERAL_TEMPLATE
                                                ? HIR_EXPR_TEMPLATE
                                                : HIR_EXPR_LITERAL);
        break;
    case AST_EXPR_IDENTIFIER:
        hir_expression = hir_expression_new(HIR_EXPR_SYMBOL);
        break;
    case AST_EXPR_LAMBDA:
        hir_expression = hir_expression_new(HIR_EXPR_LAMBDA);
        break;
    case AST_EXPR_ASSIGNMENT:
        hir_expression = hir_expression_new(HIR_EXPR_ASSIGNMENT);
        break;
    case AST_EXPR_TERNARY:
        hir_expression = hir_expression_new(HIR_EXPR_TERNARY);
        break;
    case AST_EXPR_BINARY:
        hir_expression = hir_expression_new(HIR_EXPR_BINARY);
        break;
    case AST_EXPR_UNARY:
        hir_expression = hir_expression_new(HIR_EXPR_UNARY);
        break;
    case AST_EXPR_CALL:
        hir_expression = hir_expression_new(HIR_EXPR_CALL);
        break;
    case AST_EXPR_INDEX:
        hir_expression = hir_expression_new(HIR_EXPR_INDEX);
        break;
    case AST_EXPR_MEMBER:
        hir_expression = hir_expression_new(HIR_EXPR_MEMBER);
        break;
    case AST_EXPR_CAST:
        hir_expression = hir_expression_new(HIR_EXPR_CAST);
        break;
    case AST_EXPR_ARRAY_LITERAL:
        hir_expression = hir_expression_new(HIR_EXPR_ARRAY_LITERAL);
        break;
    case AST_EXPR_GROUPING:
    default:
        hir_expression = NULL;
        break;
    }

    if (!hir_expression) {
        hir_set_error(context,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering HIR expressions.");
        return NULL;
    }

    hir_expression->type = info->type;
    hir_expression->is_callable = info->is_callable;
    hir_expression->source_span = expression->source_span;
    if (info->is_callable &&
        !lower_callable_signature(context, info, &hir_expression->callable_signature)) {
        hir_expression_free(hir_expression);
        return NULL;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                HirTemplatePart part;

                memset(&part, 0, sizeof(part));
                part.kind = expression->as.literal.as.template_parts.items[i].kind;
                if (part.kind == AST_TEMPLATE_PART_TEXT) {
                    part.as.text = ast_copy_text(
                        expression->as.literal.as.template_parts.items[i].as.text);
                    if (!part.as.text) {
                        hir_expression_free(hir_expression);
                        hir_set_error(context,
                                      expression->source_span,
                                      NULL,
                                      "Out of memory while lowering HIR templates.");
                        return NULL;
                    }
                } else {
                    part.as.expression = lower_expression(
                        context,
                        expression->as.literal.as.template_parts.items[i].as.expression);
                    if (!part.as.expression) {
                        hir_expression_free(hir_expression);
                        return NULL;
                    }
                }

                if (!append_template_part(&hir_expression->as.template_parts, part)) {
                    if (part.kind == AST_TEMPLATE_PART_TEXT) {
                        free(part.as.text);
                    } else {
                        hir_expression_free(part.as.expression);
                    }
                    hir_expression_free(hir_expression);
                    hir_set_error(context,
                                  expression->source_span,
                                  NULL,
                                  "Out of memory while lowering HIR templates.");
                    return NULL;
                }
            }
            return hir_expression;
        }

        hir_expression->as.literal.kind = expression->as.literal.kind;
        if (expression->as.literal.kind == AST_LITERAL_BOOL) {
            hir_expression->as.literal.as.bool_value = expression->as.literal.as.bool_value;
        } else if (expression->as.literal.kind != AST_LITERAL_NULL) {
            hir_expression->as.literal.as.text = ast_copy_text(expression->as.literal.as.text);
            if (!hir_expression->as.literal.as.text) {
                hir_expression_free(hir_expression);
                hir_set_error(context,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering HIR literals.");
                return NULL;
            }
        }
        return hir_expression;

    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *symbol = symbol_table_resolve_identifier(context->symbols, expression);

            if (!symbol) {
                hir_expression_free(hir_expression);
                hir_set_error(context,
                              expression->source_span,
                              NULL,
                              "Internal error: unresolved identifier '%s' during HIR lowering.",
                              expression->as.identifier ? expression->as.identifier : "<unknown>");
                return NULL;
            }

            hir_expression->as.symbol.symbol = symbol;
            hir_expression->as.symbol.name = ast_copy_text(expression->as.identifier);
            hir_expression->as.symbol.kind = symbol->kind;
            hir_expression->as.symbol.type = info->type;
            hir_expression->as.symbol.source_span = expression->source_span;
            if (!hir_expression->as.symbol.name) {
                hir_expression_free(hir_expression);
                hir_set_error(context,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering HIR symbols.");
                return NULL;
            }
        }
        return hir_expression;

    case AST_EXPR_LAMBDA:
        {
            const Scope *scope = symbol_table_find_scope(context->symbols,
                                                         expression,
                                                         SCOPE_KIND_LAMBDA);

            if (!scope) {
                hir_expression_free(hir_expression);
                hir_set_error(context,
                              expression->source_span,
                              NULL,
                              "Internal error: missing lambda scope during HIR lowering.");
                return NULL;
            }

            if (!lower_parameters(context,
                                  &hir_expression->as.lambda.parameters,
                                  &expression->as.lambda.parameters,
                                  scope)) {
                hir_expression_free(hir_expression);
                return NULL;
            }
            hir_expression->as.lambda.body = lower_body_to_block(context,
                                                                 &expression->as.lambda.body);
            if (!hir_expression->as.lambda.body) {
                hir_expression_free(hir_expression);
                return NULL;
            }
        }
        return hir_expression;

    case AST_EXPR_ASSIGNMENT:
        hir_expression->as.assignment.operator = expression->as.assignment.operator;
        hir_expression->as.assignment.target = lower_expression(context,
                                                                expression->as.assignment.target);
        hir_expression->as.assignment.value = lower_expression(context,
                                                               expression->as.assignment.value);
        break;

    case AST_EXPR_TERNARY:
        hir_expression->as.ternary.condition = lower_expression(context,
                                                                expression->as.ternary.condition);
        hir_expression->as.ternary.then_branch = lower_expression(context,
                                                                  expression->as.ternary.then_branch);
        hir_expression->as.ternary.else_branch = lower_expression(context,
                                                                  expression->as.ternary.else_branch);
        break;

    case AST_EXPR_BINARY:
        hir_expression->as.binary.operator = expression->as.binary.operator;
        hir_expression->as.binary.left = lower_expression(context, expression->as.binary.left);
        hir_expression->as.binary.right = lower_expression(context, expression->as.binary.right);
        break;

    case AST_EXPR_UNARY:
        hir_expression->as.unary.operator = expression->as.unary.operator;
        hir_expression->as.unary.operand = lower_expression(context,
                                                            expression->as.unary.operand);
        break;

    case AST_EXPR_CALL:
        hir_expression->as.call.callee = lower_expression(context, expression->as.call.callee);
        if (!hir_expression->as.call.callee) {
            hir_expression_free(hir_expression);
            return NULL;
        }
        for (i = 0; i < expression->as.call.arguments.count; i++) {
            HirExpression *argument = lower_expression(context,
                                                       expression->as.call.arguments.items[i]);

            if (!argument) {
                hir_expression_free(hir_expression);
                return NULL;
            }
            if (!append_argument(&hir_expression->as.call, argument)) {
                hir_expression_free(argument);
                hir_expression_free(hir_expression);
                hir_set_error(context,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering HIR calls.");
                return NULL;
            }
        }
        return hir_expression;

    case AST_EXPR_INDEX:
        hir_expression->as.index.target = lower_expression(context, expression->as.index.target);
        hir_expression->as.index.index = lower_expression(context, expression->as.index.index);
        break;

    case AST_EXPR_MEMBER:
        hir_expression->as.member.target = lower_expression(context, expression->as.member.target);
        hir_expression->as.member.member = ast_copy_text(expression->as.member.member);
        if (!hir_expression->as.member.member) {
            hir_expression_free(hir_expression);
            hir_set_error(context,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering HIR member access.");
            return NULL;
        }
        return hir_expression;

    case AST_EXPR_CAST:
        hir_expression->as.cast.target_type = info->type;
        hir_expression->as.cast.expression = lower_expression(context,
                                                              expression->as.cast.expression);
        break;

    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            HirExpression *element = lower_expression(context,
                                                      expression->as.array_literal.elements.items[i]);

            if (!element) {
                hir_expression_free(hir_expression);
                return NULL;
            }
            if (!append_array_element(&hir_expression->as.array_literal, element)) {
                hir_expression_free(element);
                hir_expression_free(hir_expression);
                hir_set_error(context,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering HIR array literals.");
                return NULL;
            }
        }
        return hir_expression;

    case AST_EXPR_GROUPING:
    default:
        hir_expression_free(hir_expression);
        return NULL;
    }

    if ((expression->kind == AST_EXPR_ASSIGNMENT &&
         (!hir_expression->as.assignment.target || !hir_expression->as.assignment.value)) ||
        (expression->kind == AST_EXPR_TERNARY &&
         (!hir_expression->as.ternary.condition ||
          !hir_expression->as.ternary.then_branch ||
          !hir_expression->as.ternary.else_branch)) ||
        (expression->kind == AST_EXPR_BINARY &&
         (!hir_expression->as.binary.left || !hir_expression->as.binary.right)) ||
        (expression->kind == AST_EXPR_UNARY && !hir_expression->as.unary.operand) ||
        (expression->kind == AST_EXPR_INDEX &&
         (!hir_expression->as.index.target || !hir_expression->as.index.index)) ||
        (expression->kind == AST_EXPR_CAST && !hir_expression->as.cast.expression)) {
        hir_expression_free(hir_expression);
        return NULL;
    }

    return hir_expression;
}

bool hir_build_program(HirProgram *program,
                       const AstProgram *ast_program,
                       const SymbolTable *symbols,
                       const TypeChecker *checker) {
    HirBuildContext context;
    size_t i;

    if (!program || !ast_program || !symbols || !checker) {
        return false;
    }

    hir_program_free(program);
    hir_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.ast_program = ast_program;
    context.symbols = symbols;
    context.checker = checker;

    if (type_checker_get_error(checker) != NULL) {
        hir_set_error(&context,
                      (AstSourceSpan){0},
                      NULL,
                      "Cannot lower program to HIR while the type checker reports errors.");
        return false;
    }

    if (ast_program->has_package) {
        const Symbol *package_symbol = symbol_table_get_package(symbols);

        if (!package_symbol) {
            hir_set_error(&context,
                          ast_program->package_name.tail_span,
                          NULL,
                          "Internal error: missing package symbol during HIR lowering.");
            return false;
        }

        program->has_package = true;
        program->package.symbol = package_symbol;
        program->package.name = ast_copy_text(ast_program->package_name.segments[
            ast_program->package_name.count - 1]);
        program->package.qualified_name = qualified_name_to_string(&ast_program->package_name);
        program->package.source_span = ast_program->package_name.tail_span;
        if (!program->package.name || !program->package.qualified_name) {
            hir_set_error(&context,
                          ast_program->package_name.tail_span,
                          NULL,
                          "Out of memory while lowering HIR package metadata.");
            return false;
        }
    }

    for (i = 0; i < ast_program->import_count; i++) {
        const Symbol *import_symbol = symbol_table_get_import(symbols, i);
        HirNamedSymbol import_entry;

        if (!import_symbol) {
            hir_set_error(&context,
                          ast_program->imports[i].tail_span,
                          NULL,
                          "Internal error: missing import symbol during HIR lowering.");
            return false;
        }

        memset(&import_entry, 0, sizeof(import_entry));
        import_entry.symbol = import_symbol;
        import_entry.name = ast_copy_text(ast_program->imports[i].segments[
            ast_program->imports[i].count - 1]);
        import_entry.qualified_name = qualified_name_to_string(&ast_program->imports[i]);
        import_entry.source_span = ast_program->imports[i].tail_span;
        if (!import_entry.name || !import_entry.qualified_name ||
            !append_named_symbol(&program->imports,
                                 &program->import_count,
                                 &program->import_capacity,
                                 import_entry)) {
            free_named_symbol(&import_entry);
            hir_set_error(&context,
                          ast_program->imports[i].tail_span,
                          NULL,
                          "Out of memory while lowering HIR imports.");
            return false;
        }
    }

    for (i = 0; i < ast_program->top_level_count; i++) {
        const AstTopLevelDecl *ast_decl = ast_program->top_level_decls[i];
        HirTopLevelDecl *hir_decl;

        if (ast_decl->kind == AST_TOP_LEVEL_BINDING) {
            const Scope *root_scope = symbol_table_root_scope(symbols);
            const Symbol *symbol;
            const TypeCheckInfo *info;

            symbol = scope_lookup_local(root_scope, ast_decl->as.binding_decl.name);
            if (!symbol) {
                hir_set_error(&context,
                              ast_decl->as.binding_decl.name_span,
                              NULL,
                              "Internal error: missing top-level symbol '%s' during HIR lowering.",
                              ast_decl->as.binding_decl.name);
                return false;
            }

            info = type_checker_get_symbol_info(checker, symbol);
            if (!info) {
                hir_set_error(&context,
                              ast_decl->as.binding_decl.name_span,
                              &symbol->declaration_span,
                              "Internal error: missing type info for top-level binding '%s'.",
                              ast_decl->as.binding_decl.name);
                return false;
            }

            hir_decl = hir_top_level_decl_new(HIR_TOP_LEVEL_BINDING);
            if (!hir_decl) {
                hir_set_error(&context,
                              ast_decl->as.binding_decl.name_span,
                              NULL,
                              "Out of memory while lowering HIR top-level bindings.");
                return false;
            }

            hir_decl->as.binding.name = ast_copy_text(ast_decl->as.binding_decl.name);
            hir_decl->as.binding.symbol = symbol;
            hir_decl->as.binding.type = info->type;
            hir_decl->as.binding.is_final = symbol->is_final;
            hir_decl->as.binding.is_callable = info->is_callable;
            hir_decl->as.binding.source_span = ast_decl->as.binding_decl.name_span;
            if (info->is_callable &&
                !lower_callable_signature(&context, info, &hir_decl->as.binding.callable_signature)) {
                free(hir_decl->as.binding.name);
                free(hir_decl);
                return false;
            }
            hir_decl->as.binding.initializer = lower_expression(&context,
                                                                ast_decl->as.binding_decl.initializer);
            if (!hir_decl->as.binding.name || !hir_decl->as.binding.initializer) {
                if (!program->has_error) {
                    hir_set_error(&context,
                                  ast_decl->as.binding_decl.name_span,
                                  NULL,
                                  "Out of memory while lowering HIR top-level bindings.");
                }
                free(hir_decl->as.binding.name);
                free_callable_signature(&hir_decl->as.binding.callable_signature);
                hir_expression_free(hir_decl->as.binding.initializer);
                free(hir_decl);
                return false;
            }
        } else {
            const Scope *start_scope = symbol_table_find_scope(symbols,
                                                               &ast_decl->as.start_decl,
                                                               SCOPE_KIND_START);

            hir_decl = hir_top_level_decl_new(HIR_TOP_LEVEL_START);
            if (!hir_decl) {
                hir_set_error(&context,
                              ast_decl->as.start_decl.start_span,
                              NULL,
                              "Out of memory while lowering HIR start declaration.");
                return false;
            }

            if (!start_scope) {
                free(hir_decl);
                hir_set_error(&context,
                              ast_decl->as.start_decl.start_span,
                              NULL,
                              "Internal error: missing start scope during HIR lowering.");
                return false;
            }

            hir_decl->as.start.source_span = ast_decl->as.start_decl.start_span;
            if (!lower_parameters(&context,
                                  &hir_decl->as.start.parameters,
                                  &ast_decl->as.start_decl.parameters,
                                  start_scope)) {
                free(hir_decl);
                return false;
            }
            hir_decl->as.start.body = lower_body_to_block(&context,
                                                          &ast_decl->as.start_decl.body);
            if (!hir_decl->as.start.body) {
                free_parameter_list(&hir_decl->as.start.parameters);
                free(hir_decl);
                return false;
            }
        }

        if (!append_top_level_decl(program, hir_decl)) {
            if (hir_decl->kind == HIR_TOP_LEVEL_BINDING) {
                free(hir_decl->as.binding.name);
                free_callable_signature(&hir_decl->as.binding.callable_signature);
                hir_expression_free(hir_decl->as.binding.initializer);
            } else {
                free_parameter_list(&hir_decl->as.start.parameters);
                hir_block_free(hir_decl->as.start.body);
            }
            free(hir_decl);
            hir_set_error(&context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while assembling the HIR program.");
            return false;
        }
    }

    return true;
}