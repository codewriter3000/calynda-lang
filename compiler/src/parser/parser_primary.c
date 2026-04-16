#include "parser_internal.h"

AstExpression *parse_primary_expression(Parser *parser) {
    const Token *token = parser_current_token(parser);
    AstExpression *expression;

    switch (token->type) {
    case TOK_INT_LIT:
    case TOK_FLOAT_LIT:
    case TOK_CHAR_LIT:
    case TOK_STRING_LIT:
        expression = ast_expression_new(AST_EXPR_LITERAL);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        expression->as.literal.kind =
            (token->type == TOK_INT_LIT) ? AST_LITERAL_INTEGER :
            (token->type == TOK_FLOAT_LIT) ? AST_LITERAL_FLOAT :
            (token->type == TOK_CHAR_LIT) ? AST_LITERAL_CHAR : AST_LITERAL_STRING;
        expression->as.literal.as.text = parser_copy_token_text(token);
        if (!expression->as.literal.as.text) {
            parser_set_oom_error(parser);
            ast_expression_free(expression);
            return NULL;
        }
        parser_advance(parser);
        return expression;

    case TOK_TRUE:
    case TOK_FALSE:
        expression = ast_expression_new(AST_EXPR_LITERAL);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        expression->as.literal.kind = AST_LITERAL_BOOL;
        expression->as.literal.as.bool_value = (token->type == TOK_TRUE);
        parser_advance(parser);
        return expression;

    case TOK_NULL:
        expression = ast_expression_new(AST_EXPR_LITERAL);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        expression->as.literal.kind = AST_LITERAL_NULL;
        parser_advance(parser);
        return expression;

    case TOK_TEMPLATE_FULL:
    case TOK_TEMPLATE_START:
        return parse_template_literal_expression(parser);

    case TOK_IDENTIFIER:
        expression = ast_expression_new(AST_EXPR_IDENTIFIER);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        expression->as.identifier = parser_copy_token_text(token);
        if (!expression->as.identifier) {
            parser_set_oom_error(parser);
            ast_expression_free(expression);
            return NULL;
        }
        parser_advance(parser);
        return expression;

    case TOK_LPAREN:
        expression = ast_expression_new(AST_EXPR_GROUPING);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        parser_advance(parser);
        expression->as.grouping.inner = parse_expression_node(parser);
        if (!expression->as.grouping.inner) {
            ast_expression_free(expression);
            return NULL;
        }
        if (!parser_consume(parser, TOK_RPAREN,
                            "Expected ')' after grouped expression.")) {
            ast_expression_free(expression);
            return NULL;
        }
        return expression;

    case TOK_UNDERSCORE:
        expression = ast_expression_new(AST_EXPR_DISCARD);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        parser_advance(parser);
        return expression;

    case TOK_LBRACKET:
        return parse_array_literal_expression(parser);

    case TOK_MALLOC:
    case TOK_CALLOC:
    case TOK_REALLOC:
    case TOK_FREE:
    case TOK_DEREF:
    case TOK_ADDR:
    case TOK_OFFSET:
    case TOK_STORE:
    case TOK_CLEANUP:
    case TOK_STACKALLOC: {
        AstMemoryOpKind op_kind;
        size_t expected_args;

        switch (token->type) {
        case TOK_MALLOC:  op_kind = AST_MEMORY_MALLOC;  expected_args = 1; break;
        case TOK_CALLOC:  op_kind = AST_MEMORY_CALLOC;  expected_args = 2; break;
        case TOK_REALLOC: op_kind = AST_MEMORY_REALLOC; expected_args = 2; break;
        case TOK_DEREF:   op_kind = AST_MEMORY_DEREF;   expected_args = 1; break;
        case TOK_ADDR:    op_kind = AST_MEMORY_ADDR;    expected_args = 1; break;
        case TOK_OFFSET:  op_kind = AST_MEMORY_OFFSET;  expected_args = 2; break;
        case TOK_STORE:   op_kind = AST_MEMORY_STORE;   expected_args = 2; break;
        case TOK_CLEANUP: op_kind = AST_MEMORY_CLEANUP; expected_args = 2; break;
        case TOK_STACKALLOC: op_kind = AST_MEMORY_STACKALLOC; expected_args = 1; break;
        default:          op_kind = AST_MEMORY_FREE;    expected_args = 1; break;
        }

        expression = ast_expression_new(AST_EXPR_MEMORY_OP);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        expression->as.memory_op.kind = op_kind;
        parser_advance(parser);

        if (!parser_consume(parser, TOK_LPAREN, "Expected '(' after memory operation.")) {
            ast_expression_free(expression);
            return NULL;
        }

        {
            AstExpression *arg = parse_expression_node(parser);
            if (!arg) {
                ast_expression_free(expression);
                return NULL;
            }
            if (!parser_add_expression(parser, &expression->as.memory_op.arguments, arg)) {
                ast_expression_free(expression);
                return NULL;
            }
        }

        if (expected_args == 2) {
            AstExpression *arg;
            if (!parser_consume(parser, TOK_COMMA,
                                "Expected ',' between memory operation arguments.")) {
                ast_expression_free(expression);
                return NULL;
            }
            arg = parse_expression_node(parser);
            if (!arg) {
                ast_expression_free(expression);
                return NULL;
            }
            if (!parser_add_expression(parser, &expression->as.memory_op.arguments, arg)) {
                ast_expression_free(expression);
                return NULL;
            }
        }

        if (!parser_consume(parser, TOK_RPAREN, "Expected ')' after memory operation arguments.")) {
            ast_expression_free(expression);
            return NULL;
        }

        return expression;
    }

    default:
        if (is_primitive_type_token(token->type) &&
            parser_token_at(parser, parser->current + 1)->type == TOK_LPAREN) {
            return parse_cast_expression(parser);
        }

        parser_set_error(parser, *token, "Expected expression.");
        return NULL;
    }
}
