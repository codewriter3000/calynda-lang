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
                    AstExpression *argument;

                    /* Allow `return expr` shorthand for |var block arguments */
                    if (parser_check(parser, TOK_RETURN)) {
                        const Token *ret_token = parser_current_token(parser);
                        AstExpression *nlr = ast_expression_new(AST_EXPR_NONLOCAL_RETURN);

                        if (!nlr) {
                            parser_set_oom_error(parser);
                            ast_expression_free(call);
                            return NULL;
                        }

                        nlr->source_span = parser_source_span(ret_token);
                        parser_advance(parser); /* consume 'return' */

                        if (!parser_check(parser, TOK_COMMA) &&
                            !parser_check(parser, TOK_RPAREN)) {
                            nlr->as.nonlocal_return_value = parse_expression_node(parser);
                            if (!nlr->as.nonlocal_return_value) {
                                ast_expression_free(nlr);
                                ast_expression_free(call);
                                return NULL;
                            }
                        }

                        argument = nlr;
                    } else {
                        argument = parse_expression_node(parser);
                    }

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

