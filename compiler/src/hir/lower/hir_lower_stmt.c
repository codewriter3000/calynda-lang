#include "hir_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static Symbol *hr_new_synthetic_local_symbol(const char *name,
                                             AstSourceSpan source_span) {
    Symbol *symbol = calloc(1, sizeof(*symbol));

    if (!symbol) {
        return NULL;
    }

    symbol->kind = SYMBOL_KIND_LOCAL;
    symbol->name = ast_copy_text(name);
    symbol->declaration_span = source_span;
    if (!symbol->name) {
        free(symbol);
        return NULL;
    }

    return symbol;
}

static HirExpression *hr_new_synthetic_symbol_expression(HirBuildContext *context,
                                                         const Symbol *symbol,
                                                         const char *name,
                                                         CheckedType type,
                                                         AstSourceSpan source_span) {
    HirExpression *expression = hr_expression_new(HIR_EXPR_SYMBOL);

    if (!expression) {
        hr_set_error(context,
                     source_span,
                     NULL,
                     "Out of memory while lowering HIR swap expression.");
        return NULL;
    }

    expression->type = type;
    expression->source_span = source_span;
    expression->as.symbol.symbol = symbol;
    expression->as.symbol.kind = SYMBOL_KIND_LOCAL;
    expression->as.symbol.type = type;
    expression->as.symbol.source_span = source_span;
    expression->as.symbol.name = ast_copy_text(name);
    if (!expression->as.symbol.name) {
        hir_expression_free(expression);
        hr_set_error(context,
                     source_span,
                     NULL,
                     "Out of memory while lowering HIR swap expression.");
        return NULL;
    }

    return expression;
}

static HirExpression *hr_new_swap_assignment_expression(HirBuildContext *context,
                                                        const AstExpression *target,
                                                        const AstExpression *value,
                                                        AstSourceSpan source_span) {
    HirExpression *expression = hr_expression_new(HIR_EXPR_ASSIGNMENT);
    const TypeCheckInfo *target_info;

    if (!expression) {
        hr_set_error(context,
                     source_span,
                     NULL,
                     "Out of memory while lowering HIR swap expression.");
        return NULL;
    }

    target_info = type_checker_get_expression_info(context->checker, target);
    if (!target_info) {
        hir_expression_free(expression);
        hr_set_error(context,
                     source_span,
                     NULL,
                     "Internal error: missing type info for swap target during HIR lowering.");
        return NULL;
    }

    expression->type = target_info->type;
    expression->source_span = source_span;
    expression->as.assignment.operator = AST_ASSIGN_OP_ASSIGN;
    expression->as.assignment.target = hr_lower_expression(context, target);
    expression->as.assignment.value = hr_lower_expression(context, value);
    if (!expression->as.assignment.target || !expression->as.assignment.value) {
        hir_expression_free(expression);
        return NULL;
    }

    return expression;
}

static bool hr_append_lowered_statement(HirBuildContext *context,
                                        HirBlock *block,
                                        HirStatement *statement,
                                        AstSourceSpan source_span) {
    if (hr_append_statement(block, statement)) {
        return true;
    }

    hir_statement_free(statement);
    hr_set_error(context,
                 source_span,
                 NULL,
                 "Out of memory while lowering HIR statements.");
    return false;
}

bool hr_lower_swap_statement(HirBuildContext *context,
                             HirBlock *block,
                             const AstStatement *statement) {
    const TypeCheckInfo *left_info;
    char temp_name_buffer[64];
    char *temp_name = NULL;
    Symbol *temp_symbol = NULL;
    HirStatement *temp_binding = NULL;
    HirStatement *left_assign = NULL;
    HirStatement *right_assign = NULL;

    if (!context || !block || !statement || statement->kind != AST_STMT_SWAP) {
        return false;
    }

    left_info = type_checker_get_expression_info(context->checker, statement->as.swap.left);
    if (!left_info) {
        hr_set_error(context,
                     statement->source_span,
                     NULL,
                     "Internal error: missing type info for swap statement during HIR lowering.");
        return false;
    }

    snprintf(temp_name_buffer,
             sizeof(temp_name_buffer),
             "__swap_tmp_%zu",
             context->synthetic_local_count++);
    temp_name = ast_copy_text(temp_name_buffer);
    temp_symbol = hr_new_synthetic_local_symbol(temp_name_buffer, statement->source_span);
    if (!temp_name || !temp_symbol) {
        free(temp_name);
        if (temp_symbol) {
            free(temp_symbol->name);
            free(temp_symbol);
        }
        hr_set_error(context,
                     statement->source_span,
                     NULL,
                     "Out of memory while lowering HIR swap statement.");
        return false;
    }

    temp_binding = hr_statement_new(HIR_STMT_LOCAL_BINDING);
    if (!temp_binding) {
        free(temp_name);
        free(temp_symbol->name);
        free(temp_symbol);
        hr_set_error(context,
                     statement->source_span,
                     NULL,
                     "Out of memory while lowering HIR swap statement.");
        return false;
    }
    temp_binding->source_span = statement->source_span;
    temp_binding->as.local_binding.name = temp_name;
    temp_binding->as.local_binding.symbol = temp_symbol;
    temp_binding->as.local_binding.type = left_info->type;
    temp_binding->as.local_binding.is_final = false;
    temp_binding->as.local_binding.owns_symbol = true;
    temp_binding->as.local_binding.source_span = statement->source_span;
    temp_binding->as.local_binding.initializer = hr_lower_expression(context,
                                                                     statement->as.swap.left);
    if (!temp_binding->as.local_binding.initializer) {
        hir_statement_free(temp_binding);
        return false;
    }

    left_assign = hr_statement_new(HIR_STMT_EXPRESSION);
    if (!left_assign) {
        hir_statement_free(temp_binding);
        hr_set_error(context,
                     statement->source_span,
                     NULL,
                     "Out of memory while lowering HIR swap statement.");
        return false;
    }
    left_assign->source_span = statement->source_span;
    left_assign->as.expression = hr_new_swap_assignment_expression(context,
                                                                   statement->as.swap.left,
                                                                   statement->as.swap.right,
                                                                   statement->source_span);
    if (!left_assign->as.expression) {
        hir_statement_free(left_assign);
        hir_statement_free(temp_binding);
        return false;
    }

    right_assign = hr_statement_new(HIR_STMT_EXPRESSION);
    if (!right_assign) {
        hir_statement_free(left_assign);
        hir_statement_free(temp_binding);
        hr_set_error(context,
                     statement->source_span,
                     NULL,
                     "Out of memory while lowering HIR swap statement.");
        return false;
    }
    right_assign->source_span = statement->source_span;
    right_assign->as.expression = hr_expression_new(HIR_EXPR_ASSIGNMENT);
    if (!right_assign->as.expression) {
        hir_statement_free(right_assign);
        hir_statement_free(left_assign);
        hir_statement_free(temp_binding);
        hr_set_error(context,
                     statement->source_span,
                     NULL,
                     "Out of memory while lowering HIR swap statement.");
        return false;
    }
    right_assign->as.expression->type = left_info->type;
    right_assign->as.expression->source_span = statement->source_span;
    right_assign->as.expression->as.assignment.operator = AST_ASSIGN_OP_ASSIGN;
    right_assign->as.expression->as.assignment.target =
        hr_lower_expression(context, statement->as.swap.right);
    right_assign->as.expression->as.assignment.value =
        hr_new_synthetic_symbol_expression(context,
                                           temp_symbol,
                                           temp_name_buffer,
                                           left_info->type,
                                           statement->source_span);
    if (!right_assign->as.expression->as.assignment.target ||
        !right_assign->as.expression->as.assignment.value) {
        hir_statement_free(right_assign);
        hir_statement_free(left_assign);
        hir_statement_free(temp_binding);
        return false;
    }

#include "hir_lower_stmt_p2.inc"
