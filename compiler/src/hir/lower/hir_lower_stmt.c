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

    if (!hr_append_lowered_statement(context, block, temp_binding, statement->source_span) ||
        !hr_append_lowered_statement(context, block, left_assign, statement->source_span) ||
        !hr_append_lowered_statement(context, block, right_assign, statement->source_span)) {
        return false;
    }

    return true;
}

HirStatement *hr_lower_statement(HirBuildContext *context,
                                 const AstStatement *statement,
                                 const Scope *scope) {
    HirStatement *hir_statement;
    HirStatementKind hir_kind;

    if (!statement) {
        return NULL;
    }

    hir_kind = (statement->kind == AST_STMT_EXIT) ? HIR_STMT_RETURN
                                                  : (HirStatementKind)statement->kind;
    hir_statement = hr_statement_new(hir_kind);
    if (!hir_statement) {
        hr_set_error(context,
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
                hr_set_error(context,
                             statement->as.local_binding.name_span,
                             NULL,
                             "Internal error: missing symbol for local '%s'.",
                             statement->as.local_binding.name);
                hir_statement_free(hir_statement);
                return NULL;
            }

            info = type_checker_get_symbol_info(context->checker, symbol);
            if (!info) {
                hr_set_error(context,
                             statement->as.local_binding.name_span,
                             &symbol->declaration_span,
                             "Internal error: missing type info for local '%s'.",
                             statement->as.local_binding.name);
                hir_statement_free(hir_statement);
                return NULL;
            }

            hir_statement->as.local_binding.name = ast_copy_text(statement->as.local_binding.name);
            if (!hir_statement->as.local_binding.name) {
                hr_set_error(context,
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
                !hr_lower_callable_signature(context,
                                             info,
                                             &hir_statement->as.local_binding.callable_signature)) {
                hir_statement_free(hir_statement);
                return NULL;
            }
            hir_statement->as.local_binding.initializer = hr_lower_expression(
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
            hir_statement->as.return_expression = hr_lower_expression(context,
                                                                      statement->as.return_expression);
            if (!hir_statement->as.return_expression) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_THROW:
        if (statement->as.throw_expression) {
            hir_statement->as.throw_expression = hr_lower_expression(context,
                                                                     statement->as.throw_expression);
            if (!hir_statement->as.throw_expression) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_EXPRESSION:
        if (statement->as.expression) {
            hir_statement->as.expression = hr_lower_expression(context, statement->as.expression);
            if (!hir_statement->as.expression) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_EXIT:
        break;
    case AST_STMT_MANUAL:
        hir_statement->as.manual.is_checked = statement->as.manual.is_checked;
        if (statement->as.manual.body) {
            hir_statement->as.manual.body = hr_lower_block(context,
                                                           statement->as.manual.body);
            if (!hir_statement->as.manual.body) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_SWAP:
        hr_set_error(context,
                     statement->source_span,
                     NULL,
                     "Internal error: swap statements should be expanded before direct HIR lowering.");
        hir_statement_free(hir_statement);
        return NULL;
    }

    return hir_statement;
}
