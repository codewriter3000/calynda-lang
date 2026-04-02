#include "mir.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    MirProgram       *program;
    const HirProgram *hir_program;
    size_t            next_lambda_unit_index;
} MirBuildContext;

typedef struct {
    MirBuildContext *build;
    MirUnit         *unit;
    size_t           current_block_index;
    size_t           next_synthetic_local_index;
} MirUnitBuildContext;

typedef enum {
    MIR_LVALUE_LOCAL = 0,
    MIR_LVALUE_GLOBAL,
    MIR_LVALUE_INDEX,
    MIR_LVALUE_MEMBER
} MirLValueKind;

typedef struct {
    MirLValueKind kind;
    CheckedType   type;
    union {
        size_t local_index;
        char  *global_name;
        struct {
            MirValue target;
            MirValue index;
        } index;
        struct {
            MirValue target;
            char    *member;
        } member;
    } as;
} MirLValue;

typedef struct {
    const Symbol *symbol;
    CheckedType   type;
} MirCaptureSpec;

typedef struct {
    MirCaptureSpec *items;
    size_t          count;
    size_t          capacity;
} MirCaptureList;

typedef struct {
    const Symbol **items;
    size_t         count;
    size_t         capacity;
} MirBoundSymbolSet;

static const char *MIR_MODULE_INIT_NAME = "__mir$module_init";

static bool reserve_items(void **items, size_t *capacity,
                          size_t needed, size_t item_size);
static bool source_span_is_valid(AstSourceSpan span);
static CheckedType checked_type_void_value(void);
static void mir_set_error(MirBuildContext *context,
                          AstSourceSpan primary_span,
                          const AstSourceSpan *related_span,
                          const char *format,
                          ...);
static void mir_capture_list_free(MirCaptureList *captures);
static bool mir_capture_list_add(MirBuildContext *context,
                                 MirCaptureList *captures,
                                 const Symbol *symbol,
                                 CheckedType type,
                                 AstSourceSpan source_span);
static void mir_bound_symbol_set_free(MirBoundSymbolSet *set);
static bool mir_bound_symbol_set_contains(const MirBoundSymbolSet *set,
                                          const Symbol *symbol);
static bool mir_bound_symbol_set_add(MirBuildContext *context,
                                     MirBoundSymbolSet *set,
                                     const Symbol *symbol,
                                     AstSourceSpan source_span);
static bool mir_bound_symbol_set_clone(MirBuildContext *context,
                                       const MirBoundSymbolSet *source,
                                       MirBoundSymbolSet *clone,
                                       AstSourceSpan source_span);
static bool collect_lambda_local_symbols(MirBuildContext *context,
                                         const HirBlock *block,
                                         MirBoundSymbolSet *bound,
                                         AstSourceSpan source_span);
static bool collect_expression_captures(MirBuildContext *context,
                                        const HirExpression *expression,
                                        const MirBoundSymbolSet *bound,
                                        MirCaptureList *captures);
static bool collect_statement_captures(MirBuildContext *context,
                                       const HirStatement *statement,
                                       const MirBoundSymbolSet *bound,
                                       MirCaptureList *captures);
static bool collect_block_captures(MirBuildContext *context,
                                   const HirBlock *block,
                                   const MirBoundSymbolSet *bound,
                                   MirCaptureList *captures);
static bool analyze_lambda_captures(MirBuildContext *context,
                                    const HirLambdaExpression *lambda,
                                    AstSourceSpan source_span,
                                    MirCaptureList *captures);
static bool mir_value_clone(MirBuildContext *context,
                            const MirValue *source,
                            MirValue *clone);
static void mir_value_free(MirValue *value);
static void mir_template_part_free(MirTemplatePart *part);
static void mir_lvalue_free(MirLValue *lvalue);
static void mir_instruction_free(MirInstruction *instruction);
static void mir_basic_block_free(MirBasicBlock *block);
static void mir_unit_free(MirUnit *unit);
static MirValue mir_invalid_value(void);
static bool mir_value_from_literal(MirBuildContext *context,
                                   const HirLiteral *literal,
                                   CheckedType type,
                                   MirValue *value);
static bool mir_value_from_global(MirBuildContext *context,
                                  const char *name,
                                  CheckedType type,
                                  MirValue *value);
static bool append_unit(MirProgram *program, MirUnit unit);
static bool append_local(MirUnit *unit, MirLocal local);
static bool append_block(MirUnit *unit, MirBasicBlock block);
static bool append_instruction(MirBasicBlock *block, MirInstruction instruction);
static MirBasicBlock *mir_current_block(MirUnitBuildContext *context);
static bool mir_create_block(MirUnitBuildContext *context, size_t *block_index);
static size_t mir_find_local_index(const MirUnit *unit, const Symbol *symbol);
static bool append_call_global_instruction(MirUnitBuildContext *context,
                                           const char *global_name,
                                           CheckedType return_type,
                                           AstSourceSpan source_span);
static bool append_store_local_instruction(MirUnitBuildContext *context,
                                           size_t local_index,
                                           MirValue value,
                                           AstSourceSpan source_span);
static bool append_store_global_instruction(MirUnitBuildContext *context,
                                            const char *global_name,
                                            MirValue value,
                                            AstSourceSpan source_span);
static bool append_store_index_instruction(MirUnitBuildContext *context,
                                           const MirValue *target,
                                           const MirValue *index,
                                           MirValue value,
                                           AstSourceSpan source_span);
static bool append_store_member_instruction(MirUnitBuildContext *context,
                                            const MirValue *target,
                                            const char *member,
                                            MirValue value,
                                            AstSourceSpan source_span);
static bool append_synthetic_local(MirUnitBuildContext *context,
                                   const char *name_prefix,
                                   CheckedType type,
                                   AstSourceSpan source_span,
                                   size_t *local_index);
static void set_goto_terminator(MirUnitBuildContext *context, size_t target_block_index);
static bool map_compound_assignment(AstAssignmentOperator operator,
                                    AstBinaryOperator *binary_operator);
static bool lower_assignment_target(MirUnitBuildContext *context,
                                    const HirExpression *expression,
                                    MirLValue *lvalue);
static bool load_lvalue_value(MirUnitBuildContext *context,
                              const MirLValue *lvalue,
                              AstSourceSpan source_span,
                              MirValue *value);
static bool store_lvalue_value(MirUnitBuildContext *context,
                               const MirLValue *lvalue,
                               MirValue value,
                               AstSourceSpan source_span);
static bool build_lambda_unit(MirBuildContext *context,
                              const char *name,
                              const Symbol *symbol,
                              const HirLambdaExpression *lambda,
                              CheckedType return_type,
                              MirUnitKind kind,
                              const MirCaptureList *captures);
static bool lower_lambda_expression(MirUnitBuildContext *context,
                                    const HirExpression *expression,
                                    MirValue *value);
static bool lower_ternary_expression(MirUnitBuildContext *context,
                                     const HirExpression *expression,
                                     MirValue *value);
static bool lower_short_circuit_binary_expression(MirUnitBuildContext *context,
                                                  const HirExpression *expression,
                                                  MirValue *value);
static bool lower_assignment_expression(MirUnitBuildContext *context,
                                        const HirExpression *expression,
                                        MirValue *value);
static bool lower_template_part_value(MirUnitBuildContext *context,
                                      const HirExpression *expression,
                                      MirValue *value);
static bool lower_template_expression(MirUnitBuildContext *context,
                                      const HirExpression *expression,
                                      MirValue *value);
static bool lower_member_expression(MirUnitBuildContext *context,
                                    const HirExpression *expression,
                                    MirValue *value);
static bool lower_index_expression(MirUnitBuildContext *context,
                                   const HirExpression *expression,
                                   MirValue *value);
static bool lower_array_literal_expression(MirUnitBuildContext *context,
                                           const HirExpression *expression,
                                           MirValue *value);
static bool lower_expression(MirUnitBuildContext *context,
                             const HirExpression *expression,
                             MirValue *value);
static bool lower_statement(MirUnitBuildContext *context,
                            const HirStatement *statement);
static bool lower_block(MirUnitBuildContext *context, const HirBlock *block);
static bool lower_parameters(MirUnitBuildContext *context,
                             const HirParameterList *parameters);
static bool top_level_binding_uses_lambda_unit(const HirTopLevelDecl *decl);
static bool lower_module_init_unit(MirBuildContext *context,
                                   bool *created_module_init_unit);
static bool lower_lambda_unit(MirBuildContext *context,
                              const char *name,
                              const Symbol *symbol,
                              const HirLambdaExpression *lambda,
                              CheckedType return_type,
                              MirUnitKind kind);
static bool lower_start_unit(MirBuildContext *context,
                             const HirStartDecl *start_decl,
                             bool call_module_init);

void mir_program_init(MirProgram *program) {
    if (!program) {
        return;
    }

    memset(program, 0, sizeof(*program));
}

void mir_program_free(MirProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->unit_count; i++) {
        mir_unit_free(&program->units[i]);
    }
    free(program->units);
    memset(program, 0, sizeof(*program));
}

const MirBuildError *mir_get_error(const MirProgram *program) {
    if (!program || !program->has_error) {
        return NULL;
    }

    return &program->error;
}

bool mir_format_error(const MirBuildError *error,
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

static bool source_span_is_valid(AstSourceSpan span) {
    return span.start_line > 0 && span.start_column > 0;
}

static CheckedType checked_type_void_value(void) {
    CheckedType type;

    memset(&type, 0, sizeof(type));
    type.kind = CHECKED_TYPE_VOID;
    return type;
}

static void mir_set_error(MirBuildContext *context,
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

static void mir_capture_list_free(MirCaptureList *captures) {
    if (!captures) {
        return;
    }

    free(captures->items);
    memset(captures, 0, sizeof(*captures));
}

static bool mir_capture_list_add(MirBuildContext *context,
                                 MirCaptureList *captures,
                                 const Symbol *symbol,
                                 CheckedType type,
                                 AstSourceSpan source_span) {
    size_t i;

    if (!captures || !symbol) {
        return false;
    }

    for (i = 0; i < captures->count; i++) {
        if (captures->items[i].symbol == symbol) {
            return true;
        }
    }

    if (!reserve_items((void **)&captures->items,
                       &captures->capacity,
                       captures->count + 1,
                       sizeof(*captures->items))) {
        mir_set_error(context,
                      source_span,
                      NULL,
                      "Out of memory while collecting MIR closure captures.");
        return false;
    }

    captures->items[captures->count].symbol = symbol;
    captures->items[captures->count].type = type;
    captures->count++;
    return true;
}

static void mir_bound_symbol_set_free(MirBoundSymbolSet *set) {
    if (!set) {
        return;
    }

    free(set->items);
    memset(set, 0, sizeof(*set));
}

static bool mir_bound_symbol_set_contains(const MirBoundSymbolSet *set,
                                          const Symbol *symbol) {
    size_t i;

    if (!set || !symbol) {
        return false;
    }

    for (i = 0; i < set->count; i++) {
        if (set->items[i] == symbol) {
            return true;
        }
    }

    return false;
}

static bool mir_bound_symbol_set_add(MirBuildContext *context,
                                     MirBoundSymbolSet *set,
                                     const Symbol *symbol,
                                     AstSourceSpan source_span) {
    if (!set || !symbol) {
        return false;
    }

    if (mir_bound_symbol_set_contains(set, symbol)) {
        return true;
    }

    if (!reserve_items((void **)&set->items,
                       &set->capacity,
                       set->count + 1,
                       sizeof(*set->items))) {
        mir_set_error(context,
                      source_span,
                      NULL,
                      "Out of memory while collecting MIR lambda symbols.");
        return false;
    }

    set->items[set->count++] = symbol;
    return true;
}

static bool mir_bound_symbol_set_clone(MirBuildContext *context,
                                       const MirBoundSymbolSet *source,
                                       MirBoundSymbolSet *clone,
                                       AstSourceSpan source_span) {
    if (!clone) {
        return false;
    }

    memset(clone, 0, sizeof(*clone));
    if (!source || source->count == 0) {
        return true;
    }

    clone->items = malloc(source->count * sizeof(*clone->items));
    if (!clone->items) {
        mir_set_error(context,
                      source_span,
                      NULL,
                      "Out of memory while cloning MIR lambda symbols.");
        return false;
    }

    memcpy(clone->items, source->items, source->count * sizeof(*clone->items));
    clone->count = source->count;
    clone->capacity = source->count;
    return true;
}

static bool collect_lambda_local_symbols(MirBuildContext *context,
                                         const HirBlock *block,
                                         MirBoundSymbolSet *bound,
                                         AstSourceSpan source_span) {
    size_t i;

    if (!block || !bound) {
        return true;
    }

    for (i = 0; i < block->statement_count; i++) {
        const HirStatement *statement = block->statements[i];

        if (statement->kind == HIR_STMT_LOCAL_BINDING &&
            statement->as.local_binding.symbol &&
            !mir_bound_symbol_set_add(context,
                                      bound,
                                      statement->as.local_binding.symbol,
                                      source_span)) {
            return false;
        }
    }

    return true;
}

static bool collect_expression_captures(MirBuildContext *context,
                                        const HirExpression *expression,
                                        const MirBoundSymbolSet *bound,
                                        MirCaptureList *captures) {
    size_t i;

    if (!expression) {
        return true;
    }

    switch (expression->kind) {
    case HIR_EXPR_LITERAL:
        return true;
    case HIR_EXPR_TEMPLATE:
        for (i = 0; i < expression->as.template_parts.count; i++) {
            if (expression->as.template_parts.items[i].kind == AST_TEMPLATE_PART_EXPRESSION &&
                !collect_expression_captures(context,
                                             expression->as.template_parts.items[i].as.expression,
                                             bound,
                                             captures)) {
                return false;
            }
        }
        return true;
    case HIR_EXPR_SYMBOL:
        if ((expression->as.symbol.kind == SYMBOL_KIND_PARAMETER ||
             expression->as.symbol.kind == SYMBOL_KIND_LOCAL) &&
            !mir_bound_symbol_set_contains(bound, expression->as.symbol.symbol)) {
            return mir_capture_list_add(context,
                                        captures,
                                        expression->as.symbol.symbol,
                                        expression->as.symbol.type,
                                        expression->source_span);
        }
        return true;
    case HIR_EXPR_LAMBDA:
        {
            MirBoundSymbolSet nested_bound;

            if (!mir_bound_symbol_set_clone(context,
                                            bound,
                                            &nested_bound,
                                            expression->source_span)) {
                return false;
            }

            for (i = 0; i < expression->as.lambda.parameters.count; i++) {
                if (!mir_bound_symbol_set_add(context,
                                              &nested_bound,
                                              expression->as.lambda.parameters.items[i].symbol,
                                              expression->source_span)) {
                    mir_bound_symbol_set_free(&nested_bound);
                    return false;
                }
            }

            if (!collect_lambda_local_symbols(context,
                                              expression->as.lambda.body,
                                              &nested_bound,
                                              expression->source_span) ||
                !collect_block_captures(context,
                                        expression->as.lambda.body,
                                        &nested_bound,
                                        captures)) {
                mir_bound_symbol_set_free(&nested_bound);
                return false;
            }

            mir_bound_symbol_set_free(&nested_bound);
            return true;
        }
    case HIR_EXPR_ASSIGNMENT:
        return collect_expression_captures(context,
                                           expression->as.assignment.target,
                                           bound,
                                           captures) &&
               collect_expression_captures(context,
                                           expression->as.assignment.value,
                                           bound,
                                           captures);
    case HIR_EXPR_TERNARY:
        return collect_expression_captures(context,
                                           expression->as.ternary.condition,
                                           bound,
                                           captures) &&
               collect_expression_captures(context,
                                           expression->as.ternary.then_branch,
                                           bound,
                                           captures) &&
               collect_expression_captures(context,
                                           expression->as.ternary.else_branch,
                                           bound,
                                           captures);
    case HIR_EXPR_BINARY:
        return collect_expression_captures(context,
                                           expression->as.binary.left,
                                           bound,
                                           captures) &&
               collect_expression_captures(context,
                                           expression->as.binary.right,
                                           bound,
                                           captures);
    case HIR_EXPR_UNARY:
        return collect_expression_captures(context,
                                           expression->as.unary.operand,
                                           bound,
                                           captures);
    case HIR_EXPR_CALL:
        if (!collect_expression_captures(context,
                                         expression->as.call.callee,
                                         bound,
                                         captures)) {
            return false;
        }
        for (i = 0; i < expression->as.call.argument_count; i++) {
            if (!collect_expression_captures(context,
                                             expression->as.call.arguments[i],
                                             bound,
                                             captures)) {
                return false;
            }
        }
        return true;
    case HIR_EXPR_INDEX:
        return collect_expression_captures(context,
                                           expression->as.index.target,
                                           bound,
                                           captures) &&
               collect_expression_captures(context,
                                           expression->as.index.index,
                                           bound,
                                           captures);
    case HIR_EXPR_MEMBER:
        return collect_expression_captures(context,
                                           expression->as.member.target,
                                           bound,
                                           captures);
    case HIR_EXPR_CAST:
        return collect_expression_captures(context,
                                           expression->as.cast.expression,
                                           bound,
                                           captures);
    case HIR_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.element_count; i++) {
            if (!collect_expression_captures(context,
                                             expression->as.array_literal.elements[i],
                                             bound,
                                             captures)) {
                return false;
            }
        }
        return true;
    }

    return true;
}

static bool collect_statement_captures(MirBuildContext *context,
                                       const HirStatement *statement,
                                       const MirBoundSymbolSet *bound,
                                       MirCaptureList *captures) {
    if (!statement) {
        return true;
    }

    switch (statement->kind) {
    case HIR_STMT_LOCAL_BINDING:
        return collect_expression_captures(context,
                                           statement->as.local_binding.initializer,
                                           bound,
                                           captures);
    case HIR_STMT_RETURN:
        return collect_expression_captures(context,
                                           statement->as.return_expression,
                                           bound,
                                           captures);
    case HIR_STMT_THROW:
        return collect_expression_captures(context,
                                           statement->as.throw_expression,
                                           bound,
                                           captures);
    case HIR_STMT_EXPRESSION:
        return collect_expression_captures(context,
                                           statement->as.expression,
                                           bound,
                                           captures);
    case HIR_STMT_EXIT:
        return true;
    }

    return true;
}

static bool collect_block_captures(MirBuildContext *context,
                                   const HirBlock *block,
                                   const MirBoundSymbolSet *bound,
                                   MirCaptureList *captures) {
    size_t i;

    if (!block) {
        return true;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (!collect_statement_captures(context,
                                        block->statements[i],
                                        bound,
                                        captures)) {
            return false;
        }
    }

    return true;
}

static bool analyze_lambda_captures(MirBuildContext *context,
                                    const HirLambdaExpression *lambda,
                                    AstSourceSpan source_span,
                                    MirCaptureList *captures) {
    MirBoundSymbolSet bound;
    size_t i;

    memset(&bound, 0, sizeof(bound));
    memset(captures, 0, sizeof(*captures));

    for (i = 0; i < lambda->parameters.count; i++) {
        if (!mir_bound_symbol_set_add(context,
                                      &bound,
                                      lambda->parameters.items[i].symbol,
                                      source_span)) {
            mir_bound_symbol_set_free(&bound);
            mir_capture_list_free(captures);
            return false;
        }
    }

    if (!collect_lambda_local_symbols(context, lambda->body, &bound, source_span) ||
        !collect_block_captures(context, lambda->body, &bound, captures)) {
        mir_bound_symbol_set_free(&bound);
        mir_capture_list_free(captures);
        return false;
    }

    mir_bound_symbol_set_free(&bound);
    return true;
}

static MirValue mir_invalid_value(void) {
    MirValue value;

    memset(&value, 0, sizeof(value));
    value.kind = MIR_VALUE_INVALID;
    return value;
}

static bool mir_value_clone(MirBuildContext *context,
                            const MirValue *source,
                            MirValue *clone) {
    if (!source || !clone) {
        return false;
    }

    *clone = mir_invalid_value();
    clone->kind = source->kind;
    clone->type = source->type;
    switch (source->kind) {
    case MIR_VALUE_INVALID:
        return true;
    case MIR_VALUE_TEMP:
        clone->as.temp_index = source->as.temp_index;
        return true;
    case MIR_VALUE_LOCAL:
        clone->as.local_index = source->as.local_index;
        return true;
    case MIR_VALUE_GLOBAL:
        clone->as.global_name = ast_copy_text(source->as.global_name);
        if (!clone->as.global_name) {
            mir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while cloning MIR globals.");
            return false;
        }
        return true;
    case MIR_VALUE_LITERAL:
        clone->as.literal.kind = source->as.literal.kind;
        if (source->as.literal.kind == AST_LITERAL_BOOL) {
            clone->as.literal.bool_value = source->as.literal.bool_value;
            return true;
        }
        if (source->as.literal.kind == AST_LITERAL_NULL) {
            return true;
        }
        clone->as.literal.text = ast_copy_text(source->as.literal.text);
        if (!clone->as.literal.text) {
            mir_set_error(context,
                          (AstSourceSpan){0},
                          NULL,
                          "Out of memory while cloning MIR literals.");
            return false;
        }
        return true;
    }

    return false;
}

static void mir_value_free(MirValue *value) {
    if (!value) {
        return;
    }

    if (value->kind == MIR_VALUE_GLOBAL) {
        free(value->as.global_name);
    } else if (value->kind == MIR_VALUE_LITERAL &&
               value->as.literal.kind != AST_LITERAL_BOOL &&
               value->as.literal.kind != AST_LITERAL_NULL) {
        free(value->as.literal.text);
    }

    memset(value, 0, sizeof(*value));
}

static void mir_template_part_free(MirTemplatePart *part) {
    if (!part) {
        return;
    }

    if (part->kind == MIR_TEMPLATE_PART_TEXT) {
        free(part->as.text);
    } else {
        mir_value_free(&part->as.value);
    }

    memset(part, 0, sizeof(*part));
}

static void mir_lvalue_free(MirLValue *lvalue) {
    if (!lvalue) {
        return;
    }

    switch (lvalue->kind) {
    case MIR_LVALUE_GLOBAL:
        free(lvalue->as.global_name);
        break;
    case MIR_LVALUE_INDEX:
        mir_value_free(&lvalue->as.index.target);
        mir_value_free(&lvalue->as.index.index);
        break;
    case MIR_LVALUE_MEMBER:
        mir_value_free(&lvalue->as.member.target);
        free(lvalue->as.member.member);
        break;
    case MIR_LVALUE_LOCAL:
        break;
    }

    memset(lvalue, 0, sizeof(*lvalue));
}

static void mir_instruction_free(MirInstruction *instruction) {
    size_t i;

    if (!instruction) {
        return;
    }

    switch (instruction->kind) {
    case MIR_INSTR_BINARY:
        mir_value_free(&instruction->as.binary.left);
        mir_value_free(&instruction->as.binary.right);
        break;
    case MIR_INSTR_UNARY:
        mir_value_free(&instruction->as.unary.operand);
        break;
    case MIR_INSTR_CLOSURE:
        free(instruction->as.closure.unit_name);
        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            mir_value_free(&instruction->as.closure.captures[i]);
        }
        free(instruction->as.closure.captures);
        break;
    case MIR_INSTR_CALL:
        mir_value_free(&instruction->as.call.callee);
        for (i = 0; i < instruction->as.call.argument_count; i++) {
            mir_value_free(&instruction->as.call.arguments[i]);
        }
        free(instruction->as.call.arguments);
        break;
    case MIR_INSTR_CAST:
        mir_value_free(&instruction->as.cast.operand);
        break;
    case MIR_INSTR_MEMBER:
        mir_value_free(&instruction->as.member.target);
        free(instruction->as.member.member);
        break;
    case MIR_INSTR_INDEX_LOAD:
        mir_value_free(&instruction->as.index_load.target);
        mir_value_free(&instruction->as.index_load.index);
        break;
    case MIR_INSTR_ARRAY_LITERAL:
        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            mir_value_free(&instruction->as.array_literal.elements[i]);
        }
        free(instruction->as.array_literal.elements);
        break;
    case MIR_INSTR_TEMPLATE:
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            mir_template_part_free(&instruction->as.template_literal.parts[i]);
        }
        free(instruction->as.template_literal.parts);
        break;
    case MIR_INSTR_STORE_LOCAL:
        mir_value_free(&instruction->as.store_local.value);
        break;
    case MIR_INSTR_STORE_GLOBAL:
        free(instruction->as.store_global.global_name);
        mir_value_free(&instruction->as.store_global.value);
        break;
    case MIR_INSTR_STORE_INDEX:
        mir_value_free(&instruction->as.store_index.target);
        mir_value_free(&instruction->as.store_index.index);
        mir_value_free(&instruction->as.store_index.value);
        break;
    case MIR_INSTR_STORE_MEMBER:
        mir_value_free(&instruction->as.store_member.target);
        free(instruction->as.store_member.member);
        mir_value_free(&instruction->as.store_member.value);
        break;
    }

    memset(instruction, 0, sizeof(*instruction));
}

static void mir_basic_block_free(MirBasicBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    free(block->label);
    for (i = 0; i < block->instruction_count; i++) {
        mir_instruction_free(&block->instructions[i]);
    }
    free(block->instructions);
    switch (block->terminator.kind) {
    case MIR_TERM_RETURN:
        if (block->terminator.as.return_term.has_value) {
            mir_value_free(&block->terminator.as.return_term.value);
        }
        break;
    case MIR_TERM_BRANCH:
        mir_value_free(&block->terminator.as.branch_term.condition);
        break;
    case MIR_TERM_THROW:
        mir_value_free(&block->terminator.as.throw_term.value);
        break;
    case MIR_TERM_NONE:
    case MIR_TERM_GOTO:
        break;
    }
    memset(block, 0, sizeof(*block));
}

static void mir_unit_free(MirUnit *unit) {
    size_t i;

    if (!unit) {
        return;
    }

    free(unit->name);
    for (i = 0; i < unit->local_count; i++) {
        free(unit->locals[i].name);
    }
    free(unit->locals);
    for (i = 0; i < unit->block_count; i++) {
        mir_basic_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    memset(unit, 0, sizeof(*unit));
}

static bool mir_value_from_literal(MirBuildContext *context,
                                   const HirLiteral *literal,
                                   CheckedType type,
                                   MirValue *value) {
    if (!literal || !value) {
        return false;
    }

    memset(value, 0, sizeof(*value));
    value->kind = MIR_VALUE_LITERAL;
    value->type = type;
    value->as.literal.kind = literal->kind;
    if (literal->kind == AST_LITERAL_BOOL) {
        value->as.literal.bool_value = literal->as.bool_value;
        return true;
    }
    if (literal->kind == AST_LITERAL_NULL) {
        return true;
    }

    value->as.literal.text = ast_copy_text(literal->as.text);
    if (!value->as.literal.text) {
        mir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR literals.");
        return false;
    }
    return true;
}

static bool mir_value_from_global(MirBuildContext *context,
                                  const char *name,
                                  CheckedType type,
                                  MirValue *value) {
    if (!name || !value) {
        return false;
    }

    memset(value, 0, sizeof(*value));
    value->kind = MIR_VALUE_GLOBAL;
    value->type = type;
    value->as.global_name = ast_copy_text(name);
    if (!value->as.global_name) {
        mir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR globals.");
        return false;
    }
    return true;
}

static bool append_unit(MirProgram *program, MirUnit unit) {
    if (!reserve_items((void **)&program->units,
                       &program->unit_capacity,
                       program->unit_count + 1,
                       sizeof(*program->units))) {
        return false;
    }

    program->units[program->unit_count++] = unit;
    return true;
}

static bool append_local(MirUnit *unit, MirLocal local) {
    if (!reserve_items((void **)&unit->locals,
                       &unit->local_capacity,
                       unit->local_count + 1,
                       sizeof(*unit->locals))) {
        return false;
    }

    unit->locals[unit->local_count++] = local;
    return true;
}

static bool append_block(MirUnit *unit, MirBasicBlock block) {
    if (!reserve_items((void **)&unit->blocks,
                       &unit->block_capacity,
                       unit->block_count + 1,
                       sizeof(*unit->blocks))) {
        return false;
    }

    unit->blocks[unit->block_count++] = block;
    return true;
}

static bool append_instruction(MirBasicBlock *block, MirInstruction instruction) {
    if (!reserve_items((void **)&block->instructions,
                       &block->instruction_capacity,
                       block->instruction_count + 1,
                       sizeof(*block->instructions))) {
        return false;
    }

    block->instructions[block->instruction_count++] = instruction;
    return true;
}

static MirBasicBlock *mir_current_block(MirUnitBuildContext *context) {
    if (!context || !context->unit || context->current_block_index >= context->unit->block_count) {
        return NULL;
    }

    return &context->unit->blocks[context->current_block_index];
}

static bool mir_create_block(MirUnitBuildContext *context, size_t *block_index) {
    MirBasicBlock block;
    char label[32];
    int written;
    size_t index;

    if (!context || !context->unit || !block_index) {
        return false;
    }

    index = context->unit->block_count;
    written = snprintf(label, sizeof(label), "bb%zu", index);
    if (written < 0 || (size_t)written >= sizeof(label)) {
        mir_set_error(context->build,
                      (AstSourceSpan){0},
                      NULL,
                      "Failed to label MIR block.");
        return false;
    }

    memset(&block, 0, sizeof(block));
    block.label = ast_copy_text(label);
    if (!block.label) {
        mir_set_error(context->build,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR blocks.");
        return NULL;
    }

    if (!append_block(context->unit, block)) {
        free(block.label);
        mir_set_error(context->build,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR blocks.");
        return NULL;
    }

    *block_index = index;
    return true;
}

static size_t mir_find_local_index(const MirUnit *unit, const Symbol *symbol) {
    size_t i;

    if (!unit || !symbol) {
        return (size_t)-1;
    }

    for (i = 0; i < unit->local_count; i++) {
        if (unit->locals[i].symbol == symbol) {
            return i;
        }
    }

    return (size_t)-1;
}

static bool append_call_global_instruction(MirUnitBuildContext *context,
                                           const char *global_name,
                                           CheckedType return_type,
                                           AstSourceSpan source_span) {
    MirInstruction instruction;
    MirBasicBlock *block;

    block = mir_current_block(context);
    if (!block) {
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering call.");
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;
    if (!mir_value_from_global(context->build,
                               global_name,
                               return_type,
                               &instruction.as.call.callee)) {
        return false;
    }
    if (!append_instruction(block, instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR calls.");
        return false;
    }

    return true;
}

static bool append_store_local_instruction(MirUnitBuildContext *context,
                                           size_t local_index,
                                           MirValue value,
                                           AstSourceSpan source_span) {
    MirInstruction instruction;
    MirBasicBlock *block;

    block = mir_current_block(context);
    if (!block) {
        mir_value_free(&value);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering store.");
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_STORE_LOCAL;
    instruction.as.store_local.local_index = local_index;
    instruction.as.store_local.value = value;
    if (!append_instruction(block, instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR stores.");
        return false;
    }

    return true;
}

static bool append_store_global_instruction(MirUnitBuildContext *context,
                                            const char *global_name,
                                            MirValue value,
                                            AstSourceSpan source_span) {
    MirInstruction instruction;
    MirBasicBlock *block;

    block = mir_current_block(context);
    if (!block) {
        mir_value_free(&value);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering global store.");
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_STORE_GLOBAL;
    instruction.as.store_global.global_name = ast_copy_text(global_name);
    instruction.as.store_global.value = value;
    if (!instruction.as.store_global.global_name) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR global stores.");
        return false;
    }
    if (!append_instruction(block, instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR global stores.");
        return false;
    }

    return true;
}

static bool append_store_index_instruction(MirUnitBuildContext *context,
                                           const MirValue *target,
                                           const MirValue *index,
                                           MirValue value,
                                           AstSourceSpan source_span) {
    MirInstruction instruction;
    MirBasicBlock *block;

    block = mir_current_block(context);
    if (!block) {
        mir_value_free(&value);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering index store.");
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_STORE_INDEX;
    if (!mir_value_clone(context->build, target, &instruction.as.store_index.target) ||
        !mir_value_clone(context->build, index, &instruction.as.store_index.index)) {
        instruction.as.store_index.value = value;
        mir_instruction_free(&instruction);
        return false;
    }
    instruction.as.store_index.value = value;
    if (!append_instruction(block, instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR index stores.");
        return false;
    }

    return true;
}

static bool append_store_member_instruction(MirUnitBuildContext *context,
                                            const MirValue *target,
                                            const char *member,
                                            MirValue value,
                                            AstSourceSpan source_span) {
    MirInstruction instruction;
    MirBasicBlock *block;

    block = mir_current_block(context);
    if (!block) {
        mir_value_free(&value);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering member store.");
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_STORE_MEMBER;
    if (!mir_value_clone(context->build, target, &instruction.as.store_member.target)) {
        instruction.as.store_member.value = value;
        mir_instruction_free(&instruction);
        return false;
    }
    instruction.as.store_member.member = ast_copy_text(member);
    instruction.as.store_member.value = value;
    if (!instruction.as.store_member.member) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR member stores.");
        return false;
    }
    if (!append_instruction(block, instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR member stores.");
        return false;
    }

    return true;
}

static bool append_synthetic_local(MirUnitBuildContext *context,
                                   const char *name_prefix,
                                   CheckedType type,
                                   AstSourceSpan source_span,
                                   size_t *local_index) {
    MirLocal local;
    char name[48];
    int written;

    if (!context || !context->unit || !local_index || !name_prefix) {
        return false;
    }

    written = snprintf(name,
                       sizeof(name),
                       "__mir_%s%zu",
                       name_prefix,
                       context->next_synthetic_local_index++);
    if (written < 0 || (size_t)written >= sizeof(name)) {
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Failed to name MIR synthetic local.");
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.kind = MIR_LOCAL_SYNTHETIC;
    local.name = ast_copy_text(name);
    local.type = type;
    local.index = context->unit->local_count;
    if (!local.name || !append_local(context->unit, local)) {
        free(local.name);
        mir_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR synthetic locals.");
        return false;
    }

    *local_index = local.index;
    return true;
}

static void set_goto_terminator(MirUnitBuildContext *context, size_t target_block_index) {
    MirBasicBlock *block;

    block = mir_current_block(context);
    if (!block || block->terminator.kind != MIR_TERM_NONE) {
        return;
    }

    block->terminator.kind = MIR_TERM_GOTO;
    block->terminator.as.goto_term.target_block = target_block_index;
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

static bool lower_assignment_target(MirUnitBuildContext *context,
                                    const HirExpression *expression,
                                    MirLValue *lvalue) {
    if (!context || !expression || !lvalue) {
        return false;
    }

    memset(lvalue, 0, sizeof(*lvalue));
    lvalue->type = expression->type;
    switch (expression->kind) {
    case HIR_EXPR_SYMBOL:
        if (expression->as.symbol.kind == SYMBOL_KIND_PARAMETER ||
            expression->as.symbol.kind == SYMBOL_KIND_LOCAL) {
            size_t local_index = mir_find_local_index(context->unit,
                                                      expression->as.symbol.symbol);

            if (local_index == (size_t)-1) {
                mir_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Internal error: missing MIR assignment local for symbol '%s'.",
                              expression->as.symbol.name);
                return false;
            }

            lvalue->kind = MIR_LVALUE_LOCAL;
            lvalue->as.local_index = local_index;
            return true;
        }

        lvalue->kind = MIR_LVALUE_GLOBAL;
        lvalue->as.global_name = ast_copy_text(expression->as.symbol.name);
        if (!lvalue->as.global_name) {
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR assignment targets.");
            return false;
        }
        return true;

    case HIR_EXPR_INDEX:
        lvalue->kind = MIR_LVALUE_INDEX;
        if (!lower_expression(context,
                              expression->as.index.target,
                              &lvalue->as.index.target) ||
            !lower_expression(context,
                              expression->as.index.index,
                              &lvalue->as.index.index)) {
            mir_lvalue_free(lvalue);
            return false;
        }
        return true;

    case HIR_EXPR_MEMBER:
        lvalue->kind = MIR_LVALUE_MEMBER;
        if (!lower_expression(context,
                              expression->as.member.target,
                              &lvalue->as.member.target)) {
            mir_lvalue_free(lvalue);
            return false;
        }
        lvalue->as.member.member = ast_copy_text(expression->as.member.member);
        if (!lvalue->as.member.member) {
            mir_lvalue_free(lvalue);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR member targets.");
            return false;
        }
        return true;

    default:
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: unsupported MIR assignment target kind %d.",
                      expression->kind);
        return false;
    }
}

static bool load_lvalue_value(MirUnitBuildContext *context,
                              const MirLValue *lvalue,
                              AstSourceSpan source_span,
                              MirValue *value) {
    MirInstruction instruction;

    if (!context || !lvalue || !value) {
        return false;
    }

    *value = mir_invalid_value();
    switch (lvalue->kind) {
    case MIR_LVALUE_LOCAL:
        value->kind = MIR_VALUE_LOCAL;
        value->type = lvalue->type;
        value->as.local_index = lvalue->as.local_index;
        return true;

    case MIR_LVALUE_GLOBAL:
        return mir_value_from_global(context->build,
                                     lvalue->as.global_name,
                                     lvalue->type,
                                     value);

    case MIR_LVALUE_INDEX:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_INDEX_LOAD;
        instruction.as.index_load.dest_temp = context->unit->next_temp_index++;
        if (!mir_value_clone(context->build,
                             &lvalue->as.index.target,
                             &instruction.as.index_load.target) ||
            !mir_value_clone(context->build,
                             &lvalue->as.index.index,
                             &instruction.as.index_load.index)) {
            mir_instruction_free(&instruction);
            return false;
        }
        if (!mir_current_block(context) ||
            !append_instruction(mir_current_block(context), instruction)) {
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          source_span,
                          NULL,
                          "Out of memory while lowering MIR index loads.");
            return false;
        }
        value->kind = MIR_VALUE_TEMP;
        value->type = lvalue->type;
        value->as.temp_index = instruction.as.index_load.dest_temp;
        return true;

    case MIR_LVALUE_MEMBER:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_MEMBER;
        instruction.as.member.dest_temp = context->unit->next_temp_index++;
        if (!mir_value_clone(context->build,
                             &lvalue->as.member.target,
                             &instruction.as.member.target)) {
            mir_instruction_free(&instruction);
            return false;
        }
        instruction.as.member.member = ast_copy_text(lvalue->as.member.member);
        if (!instruction.as.member.member) {
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          source_span,
                          NULL,
                          "Out of memory while lowering MIR member loads.");
            return false;
        }
        if (!mir_current_block(context) ||
            !append_instruction(mir_current_block(context), instruction)) {
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          source_span,
                          NULL,
                          "Out of memory while lowering MIR member loads.");
            return false;
        }
        value->kind = MIR_VALUE_TEMP;
        value->type = lvalue->type;
        value->as.temp_index = instruction.as.member.dest_temp;
        return true;
    }

    return false;
}

static bool store_lvalue_value(MirUnitBuildContext *context,
                               const MirLValue *lvalue,
                               MirValue value,
                               AstSourceSpan source_span) {
    if (!context || !lvalue) {
        mir_value_free(&value);
        return false;
    }

    switch (lvalue->kind) {
    case MIR_LVALUE_LOCAL:
        return append_store_local_instruction(context,
                                              lvalue->as.local_index,
                                              value,
                                              source_span);
    case MIR_LVALUE_GLOBAL:
        return append_store_global_instruction(context,
                                               lvalue->as.global_name,
                                               value,
                                               source_span);
    case MIR_LVALUE_INDEX:
        return append_store_index_instruction(context,
                                              &lvalue->as.index.target,
                                              &lvalue->as.index.index,
                                              value,
                                              source_span);
    case MIR_LVALUE_MEMBER:
        return append_store_member_instruction(context,
                                               &lvalue->as.member.target,
                                               lvalue->as.member.member,
                                               value,
                                               source_span);
    }

    mir_value_free(&value);
    return false;
}

static bool lower_ternary_expression(MirUnitBuildContext *context,
                                     const HirExpression *expression,
                                     MirValue *value) {
    MirBasicBlock *origin_block;
    MirValue condition_value;
    MirValue branch_value;
    size_t result_local_index;
    size_t then_block_index;
    size_t else_block_index;
    size_t merge_block_index;

    if (!append_synthetic_local(context,
                                "ternary",
                                expression->type,
                                expression->source_span,
                                &result_local_index)) {
        return false;
    }

    if (!lower_expression(context, expression->as.ternary.condition, &condition_value)) {
        return false;
    }

    if (!mir_create_block(context, &then_block_index) ||
        !mir_create_block(context, &else_block_index) ||
        !mir_create_block(context, &merge_block_index)) {
        mir_value_free(&condition_value);
        return false;
    }

    origin_block = mir_current_block(context);
    if (!origin_block) {
        mir_value_free(&condition_value);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: missing current MIR block for ternary branch.");
        return false;
    }

    origin_block->terminator.kind = MIR_TERM_BRANCH;
    origin_block->terminator.as.branch_term.condition = condition_value;
    origin_block->terminator.as.branch_term.true_block = then_block_index;
    origin_block->terminator.as.branch_term.false_block = else_block_index;

    context->current_block_index = then_block_index;
    if (!lower_expression(context, expression->as.ternary.then_branch, &branch_value)) {
        return false;
    }
    if (branch_value.kind == MIR_VALUE_INVALID) {
        mir_set_error(context->build,
                      expression->as.ternary.then_branch->source_span,
                      NULL,
                      "Internal error: ternary true branch did not produce a MIR value.");
        return false;
    }
    if (!append_store_local_instruction(context,
                                        result_local_index,
                                        branch_value,
                                        expression->as.ternary.then_branch->source_span)) {
        return false;
    }
    set_goto_terminator(context, merge_block_index);

    context->current_block_index = else_block_index;
    if (!lower_expression(context, expression->as.ternary.else_branch, &branch_value)) {
        return false;
    }
    if (branch_value.kind == MIR_VALUE_INVALID) {
        mir_set_error(context->build,
                      expression->as.ternary.else_branch->source_span,
                      NULL,
                      "Internal error: ternary false branch did not produce a MIR value.");
        return false;
    }
    if (!append_store_local_instruction(context,
                                        result_local_index,
                                        branch_value,
                                        expression->as.ternary.else_branch->source_span)) {
        return false;
    }
    set_goto_terminator(context, merge_block_index);

    context->current_block_index = merge_block_index;
    value->kind = MIR_VALUE_LOCAL;
    value->type = expression->type;
    value->as.local_index = result_local_index;
    return true;
}

static bool lower_short_circuit_binary_expression(MirUnitBuildContext *context,
                                                  const HirExpression *expression,
                                                  MirValue *value) {
    AstBinaryOperator operator;
    MirBasicBlock *origin_block;
    MirValue left_value;
    MirValue right_value;
    MirValue short_circuit_value;
    size_t result_local_index;
    size_t true_block_index;
    size_t false_block_index;
    size_t rhs_block_index;
    size_t short_circuit_block_index;
    size_t merge_block_index;
    bool short_circuit_bool_value;

    operator = expression->as.binary.operator;
    short_circuit_bool_value = (operator == AST_BINARY_OP_LOGICAL_OR);

    if (!append_synthetic_local(context,
                                "logical",
                                expression->type,
                                expression->source_span,
                                &result_local_index)) {
        return false;
    }

    if (!lower_expression(context, expression->as.binary.left, &left_value)) {
        return false;
    }

    if (!mir_create_block(context, &true_block_index) ||
        !mir_create_block(context, &false_block_index) ||
        !mir_create_block(context, &merge_block_index)) {
        mir_value_free(&left_value);
        return false;
    }

    rhs_block_index = (operator == AST_BINARY_OP_LOGICAL_AND)
                          ? true_block_index
                          : false_block_index;
    short_circuit_block_index = (operator == AST_BINARY_OP_LOGICAL_AND)
                                    ? false_block_index
                                    : true_block_index;

    origin_block = mir_current_block(context);
    if (!origin_block) {
        mir_value_free(&left_value);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: missing current MIR block for short-circuit branch.");
        return false;
    }

    origin_block->terminator.kind = MIR_TERM_BRANCH;
    origin_block->terminator.as.branch_term.condition = left_value;
    origin_block->terminator.as.branch_term.true_block = true_block_index;
    origin_block->terminator.as.branch_term.false_block = false_block_index;

    context->current_block_index = short_circuit_block_index;
    short_circuit_value = mir_invalid_value();
    short_circuit_value.kind = MIR_VALUE_LITERAL;
    short_circuit_value.type = expression->type;
    short_circuit_value.as.literal.kind = AST_LITERAL_BOOL;
    short_circuit_value.as.literal.bool_value = short_circuit_bool_value;
    if (!append_store_local_instruction(context,
                                        result_local_index,
                                        short_circuit_value,
                                        expression->source_span)) {
        return false;
    }
    set_goto_terminator(context, merge_block_index);

    context->current_block_index = rhs_block_index;
    if (!lower_expression(context, expression->as.binary.right, &right_value)) {
        return false;
    }
    if (right_value.kind == MIR_VALUE_INVALID) {
        mir_set_error(context->build,
                      expression->as.binary.right->source_span,
                      NULL,
                      "Internal error: logical right operand did not produce a MIR value.");
        return false;
    }
    if (!append_store_local_instruction(context,
                                        result_local_index,
                                        right_value,
                                        expression->as.binary.right->source_span)) {
        return false;
    }
    set_goto_terminator(context, merge_block_index);

    context->current_block_index = merge_block_index;
    value->kind = MIR_VALUE_LOCAL;
    value->type = expression->type;
    value->as.local_index = result_local_index;
    return true;
}

static bool lower_assignment_expression(MirUnitBuildContext *context,
                                        const HirExpression *expression,
                                        MirValue *value) {
    MirInstruction instruction;
    MirLValue lvalue;
    MirValue current_value;
    MirValue rhs_value;
    MirValue stored_value;
    AstBinaryOperator binary_operator;

    memset(&lvalue, 0, sizeof(lvalue));
    current_value = mir_invalid_value();
    rhs_value = mir_invalid_value();
    stored_value = mir_invalid_value();
    *value = mir_invalid_value();

    if (!lower_assignment_target(context, expression->as.assignment.target, &lvalue)) {
        return false;
    }

    if (expression->as.assignment.operator == AST_ASSIGN_OP_ASSIGN) {
        if (!lower_expression(context, expression->as.assignment.value, value)) {
            mir_lvalue_free(&lvalue);
            return false;
        }
    } else {
        if (!map_compound_assignment(expression->as.assignment.operator, &binary_operator) ||
            !load_lvalue_value(context,
                               &lvalue,
                               expression->as.assignment.target->source_span,
                               &current_value) ||
            !lower_expression(context, expression->as.assignment.value, &rhs_value)) {
            mir_lvalue_free(&lvalue);
            mir_value_free(&current_value);
            mir_value_free(&rhs_value);
            return false;
        }
        if (!mir_current_block(context)) {
            mir_lvalue_free(&lvalue);
            mir_value_free(&current_value);
            mir_value_free(&rhs_value);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Internal error: missing current MIR block after lowering assignment operands.");
            return false;
        }

        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_BINARY;
        instruction.as.binary.dest_temp = context->unit->next_temp_index++;
        instruction.as.binary.operator = binary_operator;
        instruction.as.binary.left = current_value;
        instruction.as.binary.right = rhs_value;
        if (!append_instruction(mir_current_block(context), instruction)) {
            mir_instruction_free(&instruction);
            mir_lvalue_free(&lvalue);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR assignment expressions.");
            return false;
        }

        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.binary.dest_temp;
    }

    if (!mir_value_clone(context->build, value, &stored_value) ||
        !store_lvalue_value(context,
                            &lvalue,
                            stored_value,
                            expression->source_span)) {
        mir_lvalue_free(&lvalue);
        mir_value_free(value);
        return false;
    }

    mir_lvalue_free(&lvalue);
    return true;
}

static bool lower_template_part_value(MirUnitBuildContext *context,
                                      const HirExpression *expression,
                                      MirValue *value) {
    MirInstruction instruction;

    if (!context || !expression || !value) {
        return false;
    }

    if (!expression->is_callable || expression->callable_signature.parameter_count != 0) {
        return lower_expression(context, expression, value);
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;
    if (!lower_expression(context, expression, &instruction.as.call.callee)) {
        return false;
    }
    instruction.as.call.has_result = (expression->type.kind != CHECKED_TYPE_VOID);
    if (instruction.as.call.has_result) {
        instruction.as.call.dest_temp = context->unit->next_temp_index++;
    }

    if (!mir_current_block(context)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: missing current MIR block after lowering template auto-call.");
        return false;
    }
    if (!append_instruction(mir_current_block(context), instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR template auto-calls.");
        return false;
    }

    if (instruction.as.call.has_result) {
        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.call.dest_temp;
    } else {
        *value = mir_invalid_value();
        value->type = expression->type;
    }
    return true;
}

static bool lower_template_expression(MirUnitBuildContext *context,
                                      const HirExpression *expression,
                                      MirValue *value) {
    MirInstruction instruction;
    size_t i;

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_TEMPLATE;
    instruction.as.template_literal.dest_temp = context->unit->next_temp_index++;
    if (expression->as.template_parts.count > 0) {
        instruction.as.template_literal.parts = calloc(expression->as.template_parts.count,
                                                       sizeof(*instruction.as.template_literal.parts));
        if (!instruction.as.template_literal.parts) {
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR templates.");
            return false;
        }
    }
    instruction.as.template_literal.part_count = expression->as.template_parts.count;
    for (i = 0; i < expression->as.template_parts.count; i++) {
        instruction.as.template_literal.parts[i].kind =
            expression->as.template_parts.items[i].kind == AST_TEMPLATE_PART_TEXT
                ? MIR_TEMPLATE_PART_TEXT
                : MIR_TEMPLATE_PART_VALUE;
        if (instruction.as.template_literal.parts[i].kind == MIR_TEMPLATE_PART_TEXT) {
            instruction.as.template_literal.parts[i].as.text =
                ast_copy_text(expression->as.template_parts.items[i].as.text);
            if (!instruction.as.template_literal.parts[i].as.text) {
                mir_instruction_free(&instruction);
                mir_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering MIR templates.");
                return false;
            }
        } else if (!lower_template_part_value(context,
                                              expression->as.template_parts.items[i].as.expression,
                                              &instruction.as.template_literal.parts[i].as.value)) {
            mir_instruction_free(&instruction);
            return false;
        }
    }

    if (!mir_current_block(context) ||
        !append_instruction(mir_current_block(context), instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR templates.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.template_literal.dest_temp;
    return true;
}

static bool lower_member_expression(MirUnitBuildContext *context,
                                    const HirExpression *expression,
                                    MirValue *value) {
    MirInstruction instruction;

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_MEMBER;
    instruction.as.member.dest_temp = context->unit->next_temp_index++;
    if (!lower_expression(context, expression->as.member.target, &instruction.as.member.target)) {
        return false;
    }
    instruction.as.member.member = ast_copy_text(expression->as.member.member);
    if (!instruction.as.member.member) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR member expressions.");
        return false;
    }
    if (!mir_current_block(context) ||
        !append_instruction(mir_current_block(context), instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR member expressions.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.member.dest_temp;
    return true;
}

static bool lower_index_expression(MirUnitBuildContext *context,
                                   const HirExpression *expression,
                                   MirValue *value) {
    MirInstruction instruction;

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_INDEX_LOAD;
    instruction.as.index_load.dest_temp = context->unit->next_temp_index++;
    if (!lower_expression(context, expression->as.index.target, &instruction.as.index_load.target) ||
        !lower_expression(context, expression->as.index.index, &instruction.as.index_load.index)) {
        mir_instruction_free(&instruction);
        return false;
    }
    if (!mir_current_block(context) ||
        !append_instruction(mir_current_block(context), instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR index expressions.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.index_load.dest_temp;
    return true;
}

static bool lower_array_literal_expression(MirUnitBuildContext *context,
                                           const HirExpression *expression,
                                           MirValue *value) {
    MirInstruction instruction;
    size_t i;

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_ARRAY_LITERAL;
    instruction.as.array_literal.dest_temp = context->unit->next_temp_index++;
    if (expression->as.array_literal.element_count > 0) {
        instruction.as.array_literal.elements = calloc(expression->as.array_literal.element_count,
                                                       sizeof(*instruction.as.array_literal.elements));
        if (!instruction.as.array_literal.elements) {
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR array literals.");
            return false;
        }
    }
    instruction.as.array_literal.element_count = expression->as.array_literal.element_count;
    for (i = 0; i < expression->as.array_literal.element_count; i++) {
        if (!lower_expression(context,
                              expression->as.array_literal.elements[i],
                              &instruction.as.array_literal.elements[i])) {
            mir_instruction_free(&instruction);
            return false;
        }
    }
    if (!mir_current_block(context) ||
        !append_instruction(mir_current_block(context), instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR array literals.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.array_literal.dest_temp;
    return true;
}

static bool lower_expression(MirUnitBuildContext *context,
                             const HirExpression *expression,
                             MirValue *value) {
    MirInstruction instruction;
    MirValue left;
    MirValue right;
    size_t i;

    if (!context || !expression || !value) {
        return false;
    }

    *value = mir_invalid_value();
    if (!mir_current_block(context)) {
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering expression.");
        return false;
    }

    switch (expression->kind) {
    case HIR_EXPR_LITERAL:
        return mir_value_from_literal(context->build,
                                      &expression->as.literal,
                                      expression->type,
                                      value);

    case HIR_EXPR_TEMPLATE:
        return lower_template_expression(context, expression, value);

    case HIR_EXPR_SYMBOL:
        if (expression->as.symbol.kind == SYMBOL_KIND_PARAMETER ||
            expression->as.symbol.kind == SYMBOL_KIND_LOCAL) {
            size_t local_index = mir_find_local_index(context->unit,
                                                      expression->as.symbol.symbol);

            if (local_index == (size_t)-1) {
                mir_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Internal error: missing MIR local for symbol '%s'.",
                              expression->as.symbol.name);
                return false;
            }

            value->kind = MIR_VALUE_LOCAL;
            value->type = expression->type;
            value->as.local_index = local_index;
            return true;
        }

        return mir_value_from_global(context->build,
                                     expression->as.symbol.name,
                                     expression->type,
                                     value);

    case HIR_EXPR_BINARY:
        if (expression->as.binary.operator == AST_BINARY_OP_LOGICAL_AND ||
            expression->as.binary.operator == AST_BINARY_OP_LOGICAL_OR) {
            return lower_short_circuit_binary_expression(context, expression, value);
        }

        if (!lower_expression(context, expression->as.binary.left, &left) ||
            !lower_expression(context, expression->as.binary.right, &right)) {
            return false;
        }

        if (!mir_current_block(context)) {
            mir_value_free(&left);
            mir_value_free(&right);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Internal error: missing current MIR block after lowering binary operands.");
            return false;
        }

        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_BINARY;
        instruction.as.binary.dest_temp = context->unit->next_temp_index++;
        instruction.as.binary.operator = expression->as.binary.operator;
        instruction.as.binary.left = left;
        instruction.as.binary.right = right;
        if (!append_instruction(mir_current_block(context), instruction)) {
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR instructions.");
            return false;
        }

        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.binary.dest_temp;
        return true;

    case HIR_EXPR_UNARY:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_UNARY;
        instruction.as.unary.dest_temp = context->unit->next_temp_index++;
        instruction.as.unary.operator = expression->as.unary.operator;
        if (!lower_expression(context, expression->as.unary.operand, &instruction.as.unary.operand)) {
            return false;
        }
        if (!mir_current_block(context) ||
            !append_instruction(mir_current_block(context), instruction)) {
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR unary expressions.");
            return false;
        }
        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.unary.dest_temp;
        return true;

    case HIR_EXPR_CALL:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_CALL;
        if (!lower_expression(context, expression->as.call.callee, &instruction.as.call.callee)) {
            return false;
        }
        instruction.as.call.has_result = (expression->type.kind != CHECKED_TYPE_VOID);
        if (instruction.as.call.has_result) {
            instruction.as.call.dest_temp = context->unit->next_temp_index++;
        }
        if (expression->as.call.argument_count > 0) {
            instruction.as.call.arguments = calloc(expression->as.call.argument_count,
                                                   sizeof(*instruction.as.call.arguments));
            if (!instruction.as.call.arguments) {
                mir_instruction_free(&instruction);
                mir_set_error(context->build,
                              expression->source_span,
                              NULL,
                              "Out of memory while lowering MIR calls.");
                return false;
            }
        }
        instruction.as.call.argument_count = expression->as.call.argument_count;
        for (i = 0; i < expression->as.call.argument_count; i++) {
            if (!lower_expression(context,
                                  expression->as.call.arguments[i],
                                  &instruction.as.call.arguments[i])) {
                mir_instruction_free(&instruction);
                return false;
            }
        }
        if (!mir_current_block(context)) {
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Internal error: missing current MIR block after lowering call arguments.");
            return false;
        }
        if (!append_instruction(mir_current_block(context), instruction)) {
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR calls.");
            return false;
        }
        if (instruction.as.call.has_result) {
            value->kind = MIR_VALUE_TEMP;
            value->type = expression->type;
            value->as.temp_index = instruction.as.call.dest_temp;
        } else {
            value->kind = MIR_VALUE_INVALID;
            value->type = expression->type;
        }
        return true;

    case HIR_EXPR_CAST:
        memset(&instruction, 0, sizeof(instruction));
        instruction.kind = MIR_INSTR_CAST;
        instruction.as.cast.dest_temp = context->unit->next_temp_index++;
        instruction.as.cast.target_type = expression->as.cast.target_type;
        if (!lower_expression(context, expression->as.cast.expression, &instruction.as.cast.operand)) {
            return false;
        }
        if (!mir_current_block(context)) {
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Internal error: missing current MIR block after lowering cast operand.");
            return false;
        }
        if (!append_instruction(mir_current_block(context), instruction)) {
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR casts.");
            return false;
        }
        value->kind = MIR_VALUE_TEMP;
        value->type = expression->type;
        value->as.temp_index = instruction.as.cast.dest_temp;
        return true;

    case HIR_EXPR_LAMBDA:
        return lower_lambda_expression(context, expression, value);
    case HIR_EXPR_ASSIGNMENT:
        return lower_assignment_expression(context, expression, value);
    case HIR_EXPR_TERNARY:
        return lower_ternary_expression(context, expression, value);
    case HIR_EXPR_INDEX:
        return lower_index_expression(context, expression, value);
    case HIR_EXPR_MEMBER:
        return lower_member_expression(context, expression, value);
    case HIR_EXPR_ARRAY_LITERAL:
        return lower_array_literal_expression(context, expression, value);
    }

    return false;
}

static bool lower_statement(MirUnitBuildContext *context,
                            const HirStatement *statement) {
    MirBasicBlock *block;
    MirValue value;
    MirLocal local;

    if (!context || !statement) {
        return false;
    }

    block = mir_current_block(context);
    if (!block) {
        mir_set_error(context->build,
                      statement->source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering statement.");
        return false;
    }

    switch (statement->kind) {
    case HIR_STMT_LOCAL_BINDING:
        memset(&local, 0, sizeof(local));
        local.kind = MIR_LOCAL_LOCAL;
        local.name = ast_copy_text(statement->as.local_binding.name);
        local.symbol = statement->as.local_binding.symbol;
        local.type = statement->as.local_binding.type;
        local.is_final = statement->as.local_binding.is_final;
        local.index = context->unit->local_count;
        if (!local.name || !append_local(context->unit, local)) {
            free(local.name);
            mir_set_error(context->build,
                          statement->source_span,
                          NULL,
                          "Out of memory while lowering MIR locals.");
            return false;
        }
        if (!lower_expression(context, statement->as.local_binding.initializer, &value)) {
            return false;
        }
        return append_store_local_instruction(context,
                                              local.index,
                                              value,
                                              statement->source_span);

    case HIR_STMT_RETURN:
        memset(&block->terminator, 0, sizeof(block->terminator));
        block->terminator.kind = MIR_TERM_RETURN;
        if (statement->as.return_expression) {
            if (!lower_expression(context, statement->as.return_expression, &value)) {
                return false;
            }
            block = mir_current_block(context);
            if (!block) {
                mir_value_free(&value);
                mir_set_error(context->build,
                              statement->source_span,
                              NULL,
                              "Internal error: missing current MIR return block.");
                return false;
            }
            memset(&block->terminator, 0, sizeof(block->terminator));
            block->terminator.kind = MIR_TERM_RETURN;
            block->terminator.as.return_term.has_value = true;
            block->terminator.as.return_term.value = value;
        }
        return true;

    case HIR_STMT_EXPRESSION:
        if (!statement->as.expression) {
            return true;
        }
        if (!lower_expression(context, statement->as.expression, &value)) {
            return false;
        }
        mir_value_free(&value);
        return true;

    case HIR_STMT_THROW:
        if (!lower_expression(context, statement->as.throw_expression, &value)) {
            return false;
        }
        block = mir_current_block(context);
        if (!block) {
            mir_value_free(&value);
            mir_set_error(context->build,
                          statement->source_span,
                          NULL,
                          "Internal error: missing current MIR throw block.");
            return false;
        }
        memset(&block->terminator, 0, sizeof(block->terminator));
        block->terminator.kind = MIR_TERM_THROW;
        block->terminator.as.throw_term.value = value;
        return true;

    case HIR_STMT_EXIT:
        mir_set_error(context->build,
                      statement->source_span,
                      NULL,
                      "Internal error: exit should already be normalized before MIR lowering.");
        return false;
    }

    return false;
}

static bool lower_block(MirUnitBuildContext *context, const HirBlock *block) {
    MirBasicBlock *current_block;
    size_t i;

    if (!context || !block) {
        return false;
    }

    for (i = 0; i < block->statement_count; i++) {
        current_block = mir_current_block(context);
        if (!current_block) {
            mir_set_error(context->build,
                          (AstSourceSpan){0},
                          NULL,
                          "Internal error: missing current MIR block while lowering block.");
            return false;
        }
        if (current_block->terminator.kind != MIR_TERM_NONE) {
            break;
        }
        if (!lower_statement(context, block->statements[i])) {
            return false;
        }
    }

    current_block = mir_current_block(context);
    if (!current_block) {
        mir_set_error(context->build,
                      (AstSourceSpan){0},
                      NULL,
                      "Internal error: missing current MIR block at block exit.");
        return false;
    }

    if (current_block->terminator.kind == MIR_TERM_NONE) {
        current_block->terminator.kind = MIR_TERM_RETURN;
        current_block->terminator.as.return_term.has_value = false;
    }
    return true;
}

static bool lower_parameters(MirUnitBuildContext *context,
                             const HirParameterList *parameters) {
    size_t i;

    if (!context || !parameters) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        MirLocal local;

        memset(&local, 0, sizeof(local));
        local.kind = MIR_LOCAL_PARAMETER;
        local.name = ast_copy_text(parameters->items[i].name);
        local.symbol = parameters->items[i].symbol;
        local.type = parameters->items[i].type;
        local.index = context->unit->local_count;
        if (!local.name || !append_local(context->unit, local)) {
            free(local.name);
            mir_set_error(context->build,
                          parameters->items[i].source_span,
                          NULL,
                          "Out of memory while lowering MIR parameters.");
            return false;
        }
        context->unit->parameter_count++;
    }

    return true;
}

static bool top_level_binding_uses_lambda_unit(const HirTopLevelDecl *decl) {
    return decl &&
           decl->kind == HIR_TOP_LEVEL_BINDING &&
           decl->as.binding.is_callable &&
           decl->as.binding.initializer &&
           decl->as.binding.initializer->kind == HIR_EXPR_LAMBDA;
}

static bool lower_module_init_unit(MirBuildContext *context,
                                   bool *created_module_init_unit) {
    MirUnit unit;
    MirUnitBuildContext unit_context;
    MirBasicBlock *block;
    size_t i;
    bool has_initializers = false;

    if (!context) {
        return false;
    }

    if (created_module_init_unit) {
        *created_module_init_unit = false;
    }

    for (i = 0; i < context->hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = context->hir_program->top_level_decls[i];

        if (decl->kind == HIR_TOP_LEVEL_BINDING &&
            !top_level_binding_uses_lambda_unit(decl)) {
            has_initializers = true;
            break;
        }
    }

    if (!has_initializers) {
        return true;
    }

    memset(&unit, 0, sizeof(unit));
    memset(&unit_context, 0, sizeof(unit_context));
    unit.kind = MIR_UNIT_INIT;
    unit.name = ast_copy_text(MIR_MODULE_INIT_NAME);
    unit.return_type = checked_type_void_value();
    if (!unit.name) {
        mir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR module init unit.");
        return false;
    }

    unit_context.build = context;
    unit_context.unit = &unit;
    if (!mir_create_block(&unit_context, &unit_context.current_block_index)) {
        mir_unit_free(&unit);
        return false;
    }

    for (i = 0; i < context->hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = context->hir_program->top_level_decls[i];
        MirValue value;

        if (decl->kind != HIR_TOP_LEVEL_BINDING ||
            top_level_binding_uses_lambda_unit(decl)) {
            continue;
        }

        if (!lower_expression(&unit_context, decl->as.binding.initializer, &value) ||
            !append_store_global_instruction(&unit_context,
                                             decl->as.binding.name,
                                             value,
                                             decl->as.binding.source_span)) {
            mir_unit_free(&unit);
            return false;
        }
    }

    block = mir_current_block(&unit_context);
    if (!block) {
        mir_unit_free(&unit);
        mir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Internal error: missing MIR block at module init exit.");
        return false;
    }
    if (block->terminator.kind == MIR_TERM_NONE) {
        block->terminator.kind = MIR_TERM_RETURN;
        block->terminator.as.return_term.has_value = false;
    }

    if (!append_unit(context->program, unit)) {
        mir_unit_free(&unit);
        mir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while assembling MIR module init unit.");
        return false;
    }

    if (created_module_init_unit) {
        *created_module_init_unit = true;
    }
    return true;
}

static bool build_lambda_unit(MirBuildContext *context,
                              const char *name,
                              const Symbol *symbol,
                              const HirLambdaExpression *lambda,
                              CheckedType return_type,
                              MirUnitKind kind,
                              const MirCaptureList *captures) {
    MirUnit unit;
    MirUnitBuildContext unit_context;
    size_t i;

    memset(&unit, 0, sizeof(unit));
    memset(&unit_context, 0, sizeof(unit_context));
    unit.kind = kind;
    unit.name = ast_copy_text(name);
    unit.symbol = symbol;
    unit.return_type = return_type;
    if (!unit.name) {
        mir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR units.");
        return false;
    }

    if (captures) {
        for (i = 0; i < captures->count; i++) {
            MirLocal local;
            const char *capture_name = captures->items[i].symbol->name
                                           ? captures->items[i].symbol->name
                                           : "<capture>";

            memset(&local, 0, sizeof(local));
            local.kind = MIR_LOCAL_CAPTURE;
            local.name = ast_copy_text(capture_name);
            local.symbol = captures->items[i].symbol;
            local.type = captures->items[i].type;
            local.is_final = captures->items[i].symbol->is_final;
            local.index = unit.local_count;
            if (!local.name || !append_local(&unit, local)) {
                free(local.name);
                mir_unit_free(&unit);
                mir_set_error(context,
                              captures->items[i].symbol->declaration_span,
                              NULL,
                              "Out of memory while lowering MIR closure captures.");
                return false;
            }
        }
    }

    unit_context.build = context;
    unit_context.unit = &unit;
    if (!mir_create_block(&unit_context, &unit_context.current_block_index) ||
        !lower_parameters(&unit_context, &lambda->parameters) ||
        !lower_block(&unit_context, lambda->body)) {
        mir_unit_free(&unit);
        return false;
    }

    if (!append_unit(context->program, unit)) {
        mir_unit_free(&unit);
        mir_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while assembling MIR units.");
        return false;
    }

    return true;
}

static bool lower_lambda_unit(MirBuildContext *context,
                              const char *name,
                              const Symbol *symbol,
                              const HirLambdaExpression *lambda,
                              CheckedType return_type,
                              MirUnitKind kind) {
    MirCaptureList empty_captures;

    memset(&empty_captures, 0, sizeof(empty_captures));
    return build_lambda_unit(context,
                             name,
                             symbol,
                             lambda,
                             return_type,
                             kind,
                             &empty_captures);
}

static bool lower_lambda_expression(MirUnitBuildContext *context,
                                    const HirExpression *expression,
                                    MirValue *value) {
    MirCaptureList captures;
    MirInstruction instruction;
    char unit_name[128];
    const char *parent_name;
    int written;
    size_t i;

    memset(&captures, 0, sizeof(captures));
    if (!context || !expression || !value) {
        return false;
    }

    if (!analyze_lambda_captures(context->build,
                                 &expression->as.lambda,
                                 expression->source_span,
                                 &captures)) {
        return false;
    }

    parent_name = (context->unit && context->unit->name) ? context->unit->name : "lambda";
    written = snprintf(unit_name,
                       sizeof(unit_name),
                       "%s$lambda%zu",
                       parent_name,
                       context->build->next_lambda_unit_index++);
    if (written < 0 || (size_t)written >= sizeof(unit_name)) {
        mir_capture_list_free(&captures);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Failed to name MIR lambda unit.");
        return false;
    }

    if (!build_lambda_unit(context->build,
                           unit_name,
                           NULL,
                           &expression->as.lambda,
                           expression->callable_signature.return_type,
                           MIR_UNIT_LAMBDA,
                           &captures)) {
        mir_capture_list_free(&captures);
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CLOSURE;
    instruction.as.closure.dest_temp = context->unit->next_temp_index++;
    instruction.as.closure.unit_name = ast_copy_text(unit_name);
    if (!instruction.as.closure.unit_name) {
        mir_capture_list_free(&captures);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR closures.");
        return false;
    }

    if (captures.count > 0) {
        instruction.as.closure.captures = calloc(captures.count,
                                                 sizeof(*instruction.as.closure.captures));
        if (!instruction.as.closure.captures) {
            mir_capture_list_free(&captures);
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR closures.");
            return false;
        }
    }
    instruction.as.closure.capture_count = captures.count;

    for (i = 0; i < captures.count; i++) {
        size_t local_index = mir_find_local_index(context->unit, captures.items[i].symbol);

        if (local_index == (size_t)-1) {
            mir_capture_list_free(&captures);
            mir_instruction_free(&instruction);
            mir_set_error(context->build,
                          expression->source_span,
                          &captures.items[i].symbol->declaration_span,
                          "Internal error: missing MIR capture for symbol '%s'.",
                          captures.items[i].symbol->name ? captures.items[i].symbol->name : "<anonymous>");
            return false;
        }

        instruction.as.closure.captures[i].kind = MIR_VALUE_LOCAL;
        instruction.as.closure.captures[i].type = captures.items[i].type;
        instruction.as.closure.captures[i].as.local_index = local_index;
    }

    mir_capture_list_free(&captures);

    if (!mir_current_block(context) ||
        !append_instruction(mir_current_block(context), instruction)) {
        mir_instruction_free(&instruction);
        mir_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR closures.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.closure.dest_temp;
    return true;
}

static bool lower_start_unit(MirBuildContext *context,
                             const HirStartDecl *start_decl,
                             bool call_module_init) {
    MirUnit unit;
    MirUnitBuildContext unit_context;

    if (!context || !start_decl) {
        return false;
    }

    memset(&unit, 0, sizeof(unit));
    memset(&unit_context, 0, sizeof(unit_context));
    unit.kind = MIR_UNIT_START;
    unit.name = ast_copy_text("start");
    unit.return_type = (CheckedType){CHECKED_TYPE_VALUE, AST_PRIMITIVE_INT32, 0};
    if (!unit.name) {
        mir_set_error(context,
                      start_decl->source_span,
                      NULL,
                      "Out of memory while lowering MIR start unit.");
        return false;
    }

    unit_context.build = context;
    unit_context.unit = &unit;
    if (!mir_create_block(&unit_context, &unit_context.current_block_index) ||
        !lower_parameters(&unit_context, &start_decl->parameters)) {
        mir_unit_free(&unit);
        return false;
    }

    if (call_module_init &&
        !append_call_global_instruction(&unit_context,
                                        MIR_MODULE_INIT_NAME,
                                        checked_type_void_value(),
                                        start_decl->source_span)) {
        mir_unit_free(&unit);
        return false;
    }

    if (!lower_block(&unit_context, start_decl->body)) {
        mir_unit_free(&unit);
        return false;
    }

    if (!append_unit(context->program, unit)) {
        mir_unit_free(&unit);
        mir_set_error(context,
                      start_decl->source_span,
                      NULL,
                      "Out of memory while assembling MIR start unit.");
        return false;
    }

    return true;
}

bool mir_build_program(MirProgram *program, const HirProgram *hir_program) {
    MirBuildContext context;
    const HirStartDecl *start_decl = NULL;
    size_t i;
    bool created_module_init_unit;

    if (!program || !hir_program) {
        return false;
    }

    mir_program_free(program);
    mir_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.hir_program = hir_program;

    if (hir_get_error(hir_program) != NULL) {
        mir_set_error(&context,
                      (AstSourceSpan){0},
                      NULL,
                      "Cannot lower program to MIR while the HIR reports errors.");
        return false;
    }

    for (i = 0; i < hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = hir_program->top_level_decls[i];

        if (decl->kind == HIR_TOP_LEVEL_START) {
            start_decl = &decl->as.start;
            continue;
        }

        if (top_level_binding_uses_lambda_unit(decl)) {
            if (!lower_lambda_unit(&context,
                                   decl->as.binding.name,
                                   decl->as.binding.symbol,
                                   &decl->as.binding.initializer->as.lambda,
                                   decl->as.binding.callable_signature.return_type,
                                   MIR_UNIT_BINDING)) {
                return false;
            }
            continue;
        }
    }

    if (!lower_module_init_unit(&context, &created_module_init_unit)) {
        return false;
    }

    if (start_decl &&
        !lower_start_unit(&context, start_decl, created_module_init_unit)) {
        return false;
    }

    return true;
}