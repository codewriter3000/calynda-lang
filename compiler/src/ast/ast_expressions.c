#include "ast_internal.h"

void ast_block_free(AstBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    for (i = 0; i < block->statement_count; i++) {
        ast_statement_free(block->statements[i]);
    }

    free(block->statements);
    free(block);
}

bool ast_block_append_statement(AstBlock *block, AstStatement *statement) {
    if (!block || !statement) {
        return false;
    }

    if (!ast_reserve_items((void **)&block->statements, &block->statement_capacity,
                           block->statement_count + 1, sizeof(*block->statements))) {
        return false;
    }

    block->statements[block->statement_count++] = statement;
    return true;
}

void ast_expression_list_free_internal(AstExpressionList *list) {
    size_t i;

    if (!list) {
        return;
    }

    for (i = 0; i < list->count; i++) {
        ast_expression_free(list->items[i]);
    }

    free(list->items);
    memset(list, 0, sizeof(*list));
}

bool ast_expression_list_append(AstExpressionList *list, AstExpression *expression) {
    if (!list || !expression) {
        return false;
    }

    if (!ast_reserve_items((void **)&list->items, &list->capacity,
                           list->count + 1, sizeof(*list->items))) {
        return false;
    }

    list->items[list->count++] = expression;
    return true;
}

bool ast_template_literal_append_text(AstLiteral *literal, const char *text) {
    AstTemplatePart part;

    if (!literal || literal->kind != AST_LITERAL_TEMPLATE || !text) {
        return false;
    }

    memset(&part, 0, sizeof(part));
    part.kind = AST_TEMPLATE_PART_TEXT;
    part.as.text = ast_copy_text(text);
    if (!part.as.text) {
        return false;
    }

    if (!ast_reserve_items((void **)&literal->as.template_parts.items,
                           &literal->as.template_parts.capacity,
                           literal->as.template_parts.count + 1,
                           sizeof(*literal->as.template_parts.items))) {
        free(part.as.text);
        return false;
    }

    literal->as.template_parts.items[literal->as.template_parts.count++] = part;
    return true;
}

bool ast_template_literal_append_expression(AstLiteral *literal,
                                            AstExpression *expression) {
    AstTemplatePart part;

    if (!literal || literal->kind != AST_LITERAL_TEMPLATE || !expression) {
        return false;
    }

    memset(&part, 0, sizeof(part));
    part.kind = AST_TEMPLATE_PART_EXPRESSION;
    part.as.expression = expression;

    if (!ast_reserve_items((void **)&literal->as.template_parts.items,
                           &literal->as.template_parts.capacity,
                           literal->as.template_parts.count + 1,
                           sizeof(*literal->as.template_parts.items))) {
        return false;
    }

    literal->as.template_parts.items[literal->as.template_parts.count++] = part;
    return true;
}

void ast_literal_free_internal(AstLiteral *literal) {
    size_t i;

    if (!literal) {
        return;
    }

    switch (literal->kind) {
    case AST_LITERAL_INTEGER:
    case AST_LITERAL_FLOAT:
    case AST_LITERAL_CHAR:
    case AST_LITERAL_STRING:
        free(literal->as.text);
        break;
    case AST_LITERAL_TEMPLATE:
        for (i = 0; i < literal->as.template_parts.count; i++) {
            AstTemplatePart *part = &literal->as.template_parts.items[i];
            if (part->kind == AST_TEMPLATE_PART_TEXT) {
                free(part->as.text);
            } else {
                ast_expression_free(part->as.expression);
            }
        }
        free(literal->as.template_parts.items);
        break;
    case AST_LITERAL_BOOL:
    case AST_LITERAL_NULL:
        break;
    }

    memset(literal, 0, sizeof(*literal));
    literal->kind = AST_LITERAL_NULL;
}

void ast_lambda_body_init_internal(AstLambdaBody *body) {
    if (!body) {
        return;
    }
    memset(body, 0, sizeof(*body));
    body->kind = AST_LAMBDA_BODY_EXPRESSION;
}

void ast_lambda_body_free_internal(AstLambdaBody *body) {
    if (!body) {
        return;
    }

    if (body->kind == AST_LAMBDA_BODY_BLOCK) {
        ast_block_free(body->as.block);
    } else {
        ast_expression_free(body->as.expression);
    }

    ast_lambda_body_init_internal(body);
}

AstExpression *ast_expression_new(AstExpressionKind kind) {
    AstExpression *expression = calloc(1, sizeof(*expression));

    if (!expression) {
        return NULL;
    }

    expression->kind = kind;
    if (kind == AST_EXPR_LITERAL) {
        expression->as.literal.kind = AST_LITERAL_NULL;
    } else if (kind == AST_EXPR_LAMBDA) {
        ast_parameter_list_init(&expression->as.lambda.parameters);
        ast_lambda_body_init_internal(&expression->as.lambda.body);
    }

    return expression;
}

void ast_expression_free(AstExpression *expression) {
    if (!expression) {
        return;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        ast_literal_free_internal(&expression->as.literal);
        break;
    case AST_EXPR_IDENTIFIER:
        free(expression->as.identifier);
        break;
    case AST_EXPR_LAMBDA:
        ast_parameter_list_free(&expression->as.lambda.parameters);
        ast_lambda_body_free_internal(&expression->as.lambda.body);
        break;
    case AST_EXPR_ASSIGNMENT:
        ast_expression_free(expression->as.assignment.target);
        ast_expression_free(expression->as.assignment.value);
        break;
    case AST_EXPR_TERNARY:
        ast_expression_free(expression->as.ternary.condition);
        ast_expression_free(expression->as.ternary.then_branch);
        ast_expression_free(expression->as.ternary.else_branch);
        break;
    case AST_EXPR_BINARY:
        ast_expression_free(expression->as.binary.left);
        ast_expression_free(expression->as.binary.right);
        break;
    case AST_EXPR_UNARY:
        ast_expression_free(expression->as.unary.operand);
        break;
    case AST_EXPR_CALL:
        ast_expression_free(expression->as.call.callee);
        ast_expression_list_free_internal(&expression->as.call.arguments);
        break;
    case AST_EXPR_INDEX:
        ast_expression_free(expression->as.index.target);
        ast_expression_free(expression->as.index.index);
        break;
    case AST_EXPR_MEMBER:
        ast_expression_free(expression->as.member.target);
        free(expression->as.member.member);
        break;
    case AST_EXPR_CAST:
        ast_expression_free(expression->as.cast.expression);
        break;
    case AST_EXPR_ARRAY_LITERAL:
        ast_expression_list_free_internal(&expression->as.array_literal.elements);
        break;
    case AST_EXPR_GROUPING:
        ast_expression_free(expression->as.grouping.inner);
        break;
    case AST_EXPR_DISCARD:
        break;
    case AST_EXPR_POST_INCREMENT:
        ast_expression_free(expression->as.post_increment.operand);
        break;
    case AST_EXPR_POST_DECREMENT:
        ast_expression_free(expression->as.post_decrement.operand);
        break;
    case AST_EXPR_MEMORY_OP:
        ast_expression_list_free_internal(&expression->as.memory_op.arguments);
        break;
    }

    free(expression);
}
