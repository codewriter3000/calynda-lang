#include "hir_internal.h"
#include <stdlib.h>
#include <string.h>

void hr_free_named_symbol(HirNamedSymbol *symbol) {
    if (!symbol) {
        return;
    }

    free(symbol->name);
    free(symbol->qualified_name);
    memset(symbol, 0, sizeof(*symbol));
}
void hr_free_callable_signature(HirCallableSignature *signature) {
    if (!signature) {
        return;
    }

    free(signature->parameter_types);
    memset(signature, 0, sizeof(*signature));
}

void hr_free_parameter_list(HirParameterList *list) {
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

void hr_free_template_part_list(HirTemplatePartList *list) {
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

void hr_free_call_expression(HirCallExpression *call) {
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

void hr_free_array_literal_expression(HirArrayLiteralExpression *array_literal) {
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
        if (statement->as.local_binding.owns_symbol && statement->as.local_binding.symbol) {
            free(statement->as.local_binding.symbol->qualified_name);
            free(statement->as.local_binding.symbol->name);
            free((Symbol *)statement->as.local_binding.symbol);
        }
        if (statement->as.local_binding.is_callable) {
            hr_free_callable_signature(&statement->as.local_binding.callable_signature);
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
    case HIR_STMT_MANUAL:
        hir_block_free(statement->as.manual.body);
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
        hr_free_template_part_list(&expression->as.template_parts);
        break;
    case HIR_EXPR_SYMBOL:
        free(expression->as.symbol.name);
        break;
    case HIR_EXPR_LAMBDA:
        hr_free_parameter_list(&expression->as.lambda.parameters);
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
        hr_free_call_expression(&expression->as.call);
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
        hr_free_array_literal_expression(&expression->as.array_literal);
        break;
    case HIR_EXPR_DISCARD:
        break;
    case HIR_EXPR_POST_INCREMENT:
        hir_expression_free(expression->as.post_increment.operand);
        break;
    case HIR_EXPR_POST_DECREMENT:
        hir_expression_free(expression->as.post_decrement.operand);
        break;
    case HIR_EXPR_MEMORY_OP:
        {
            size_t mem_i;
            for (mem_i = 0; mem_i < expression->as.memory_op.argument_count; mem_i++) {
                hir_expression_free(expression->as.memory_op.arguments[mem_i]);
            }
            free(expression->as.memory_op.arguments);
        }
        break;
    }

    if (expression->is_callable) {
        hr_free_callable_signature(&expression->callable_signature);
    }

    free(expression);
}
