#include "parser_internal.h"

AstExpression *parse_postfix_expression(Parser *parser) {
    AstExpression *expression = parse_primary_expression(parser);

    if (!expression) {
        return NULL;
    }

    for (;;) {
        if (parser_match(parser, TOK_LPAREN)) {
            AstExpression *call = ast_expression_new(AST_EXPR_CALL);

            if (!call) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            call->source_span = expression->source_span;
            call->as.call.callee = expression;
            if (!parser_check(parser, TOK_RPAREN)) {
                do {
                    AstExpression *argument = parse_expression_node(parser);

                    if (!argument) {
                        ast_expression_free(call);
                        return NULL;
                    }

                    if (!parser_add_expression(parser, &call->as.call.arguments,
                                               argument)) {
                        ast_expression_free(call);
                        return NULL;
                    }
                } while (parser_match(parser, TOK_COMMA));
            }

            if (!parser_consume(parser, TOK_RPAREN,
                                "Expected ')' after argument list.")) {
                ast_expression_free(call);
                return NULL;
            }

            expression = call;
            continue;
        }

        if (parser_match(parser, TOK_LBRACKET)) {
            AstExpression *index = ast_expression_new(AST_EXPR_INDEX);

            if (!index) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            index->source_span = expression->source_span;
            index->as.index.target = expression;
            index->as.index.index = parse_expression_node(parser);
            if (!index->as.index.index) {
                ast_expression_free(index);
                return NULL;
            }

            if (!parser_consume(parser, TOK_RBRACKET,
                                "Expected ']' after index expression.")) {
                ast_expression_free(index);
                return NULL;
            }

            expression = index;
            continue;
        }

        if (parser_match(parser, TOK_DOT)) {
            AstExpression *member = ast_expression_new(AST_EXPR_MEMBER);

            if (!member) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            member->source_span = expression->source_span;
            member->as.member.target = expression;
            member->as.member.member =
                parser_consume_identifier(parser, "Expected member name after '.'.");
            if (!member->as.member.member) {
                ast_expression_free(member);
                return NULL;
            }

            expression = member;
            continue;
        }

        if (parser_match(parser, TOK_PLUS_PLUS)) {
            AstExpression *post_inc = ast_expression_new(AST_EXPR_POST_INCREMENT);

            if (!post_inc) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            post_inc->source_span = expression->source_span;
            post_inc->as.post_increment.operand = expression;
            expression = post_inc;
            continue;
        }

        if (parser_match(parser, TOK_MINUS_MINUS)) {
            AstExpression *post_dec = ast_expression_new(AST_EXPR_POST_DECREMENT);

            if (!post_dec) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            post_dec->source_span = expression->source_span;
            post_dec->as.post_decrement.operand = expression;
            expression = post_dec;
            continue;
        }

        break;
    }

    return expression;
}

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
    case TOK_FREE: {
        AstMemoryOpKind op_kind;
        size_t expected_args;

        switch (token->type) {
        case TOK_MALLOC:  op_kind = AST_MEMORY_MALLOC;  expected_args = 1; break;
        case TOK_CALLOC:  op_kind = AST_MEMORY_CALLOC;  expected_args = 2; break;
        case TOK_REALLOC: op_kind = AST_MEMORY_REALLOC; expected_args = 2; break;
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
