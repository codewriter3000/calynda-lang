#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

HirExpression *hr_lower_expression(HirBuildContext *context,
                                   const AstExpression *expression) {
    const TypeCheckInfo *info;
    HirExpression *hir_expression;

    if (!expression) {
        return NULL;
    }

    if (expression->kind == AST_EXPR_GROUPING) {
        return hr_lower_expression(context, expression->as.grouping.inner);
    }

    info = type_checker_get_expression_info(context->checker, expression);
    if (!info) {
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Internal error: missing type info for expression during HIR lowering.");
        return NULL;
    }

    if (expression->kind == AST_EXPR_MEMORY_OP) {
        return hr_lower_memory_expression(context, expression, info);
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        hir_expression = hr_expression_new(expression->as.literal.kind == AST_LITERAL_TEMPLATE
                                               ? HIR_EXPR_TEMPLATE
                                               : HIR_EXPR_LITERAL);
        break;
    case AST_EXPR_IDENTIFIER:
        hir_expression = hr_expression_new(HIR_EXPR_SYMBOL);
        break;
    case AST_EXPR_LAMBDA:
        hir_expression = hr_expression_new(HIR_EXPR_LAMBDA);
        break;
    case AST_EXPR_ASSIGNMENT:
        hir_expression = hr_expression_new(HIR_EXPR_ASSIGNMENT);
        break;
    case AST_EXPR_TERNARY:
        hir_expression = hr_expression_new(HIR_EXPR_TERNARY);
        break;
    case AST_EXPR_BINARY:
        hir_expression = hr_expression_new(HIR_EXPR_BINARY);
        break;
    case AST_EXPR_UNARY:
        hir_expression = hr_expression_new(HIR_EXPR_UNARY);
        break;
    case AST_EXPR_CALL:
        hir_expression = hr_expression_new(HIR_EXPR_CALL);
        break;
    case AST_EXPR_INDEX:
        hir_expression = hr_expression_new(HIR_EXPR_INDEX);
        break;
    case AST_EXPR_MEMBER:
        hir_expression = hr_expression_new(HIR_EXPR_MEMBER);
        break;
    case AST_EXPR_CAST:
        hir_expression = hr_expression_new(HIR_EXPR_CAST);
        break;
    case AST_EXPR_ARRAY_LITERAL:
        hir_expression = hr_expression_new(HIR_EXPR_ARRAY_LITERAL);
        break;
    case AST_EXPR_DISCARD:
        hir_expression = hr_expression_new(HIR_EXPR_DISCARD);
        break;
    case AST_EXPR_POST_INCREMENT:
        hir_expression = hr_expression_new(HIR_EXPR_POST_INCREMENT);
        break;
    case AST_EXPR_POST_DECREMENT:
        hir_expression = hr_expression_new(HIR_EXPR_POST_DECREMENT);
        break;
    case AST_EXPR_GROUPING:
    default:
        hir_expression = NULL;
        break;
    }

    if (!hir_expression) {
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering HIR expressions.");
        return NULL;
    }

    hir_expression->type = info->type;
    hir_expression->is_callable = info->is_callable;
    hir_expression->source_span = expression->source_span;
    if (info->is_callable &&
        !hr_lower_callable_signature(context, info, &hir_expression->callable_signature)) {
        hir_expression_free(hir_expression);
        return NULL;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
    case AST_EXPR_IDENTIFIER:
    case AST_EXPR_LAMBDA:
    case AST_EXPR_CALL:
    case AST_EXPR_MEMBER:
    case AST_EXPR_ARRAY_LITERAL:
        return hr_lower_expr_complex(context, expression, hir_expression, info);

    case AST_EXPR_ASSIGNMENT:
        hir_expression->as.assignment.operator = expression->as.assignment.operator;
        hir_expression->as.assignment.target = hr_lower_expression(context,
                                                                   expression->as.assignment.target);
        hir_expression->as.assignment.value = hr_lower_expression(context,
                                                                  expression->as.assignment.value);
        break;

    case AST_EXPR_TERNARY:
        hir_expression->as.ternary.condition = hr_lower_expression(context,
                                                                   expression->as.ternary.condition);
        hir_expression->as.ternary.then_branch = hr_lower_expression(context,
                                                                     expression->as.ternary.then_branch);
        hir_expression->as.ternary.else_branch = hr_lower_expression(context,
                                                                     expression->as.ternary.else_branch);
        break;

    case AST_EXPR_BINARY:
        hir_expression->as.binary.operator = expression->as.binary.operator;
        hir_expression->as.binary.left = hr_lower_expression(context, expression->as.binary.left);
        hir_expression->as.binary.right = hr_lower_expression(context, expression->as.binary.right);
        break;

    case AST_EXPR_UNARY:
        hir_expression->as.unary.operator = expression->as.unary.operator;
        hir_expression->as.unary.operand = hr_lower_expression(context,
                                                               expression->as.unary.operand);
        break;

    case AST_EXPR_INDEX:
        hir_expression->as.index.target = hr_lower_expression(context, expression->as.index.target);
        hir_expression->as.index.index = hr_lower_expression(context, expression->as.index.index);
        break;

    case AST_EXPR_CAST:
        hir_expression->as.cast.target_type = info->type;
        hir_expression->as.cast.expression = hr_lower_expression(context,
                                                                 expression->as.cast.expression);
        break;

    case AST_EXPR_DISCARD:
        break;

    case AST_EXPR_POST_INCREMENT:
        hir_expression->as.post_increment.operand =
            hr_lower_expression(context, expression->as.post_increment.operand);
        break;

    case AST_EXPR_POST_DECREMENT:
        hir_expression->as.post_decrement.operand =
            hr_lower_expression(context, expression->as.post_decrement.operand);
        break;

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
        (expression->kind == AST_EXPR_CAST && !hir_expression->as.cast.expression) ||
        (expression->kind == AST_EXPR_POST_INCREMENT &&
         !hir_expression->as.post_increment.operand) ||
        (expression->kind == AST_EXPR_POST_DECREMENT &&
         !hir_expression->as.post_decrement.operand)) {
        hir_expression_free(hir_expression);
        return NULL;
    }

    return hir_expression;
}
