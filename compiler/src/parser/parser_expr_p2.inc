    /* Prefix increment / decrement */
    if (parser_check(parser, TOK_PLUS_PLUS) || parser_check(parser, TOK_MINUS_MINUS)) {
        AstUnaryOperator op = parser_check(parser, TOK_PLUS_PLUS)
                                  ? AST_UNARY_OP_PRE_INCREMENT
                                  : AST_UNARY_OP_PRE_DECREMENT;
        const Token *operator_token = parser_current_token(parser);
        AstExpression *expression = ast_expression_new(AST_EXPR_UNARY);
        AstExpression *operand;

        parser_advance(parser);

        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }

        operand = parse_unary_expression(parser);
        if (!operand) {
            ast_expression_free(expression);
            return NULL;
        }

        expression->source_span = parser_source_span(operator_token);
        expression->as.unary.operator = op;
        expression->as.unary.operand = operand;
        return expression;
    }

    for (i = 0; i < sizeof(unary_operators) / sizeof(unary_operators[0]); i++) {
        if (parser_match(parser, unary_operators[i].token_type)) {
            AstExpression *expression = ast_expression_new(AST_EXPR_UNARY);
            AstExpression *operand;
            const Token *operator_token = parser_previous_token(parser);

            if (!expression) {
                parser_set_oom_error(parser);
                return NULL;
            }

            operand = parse_unary_expression(parser);
            if (!operand) {
                ast_expression_free(expression);
                return NULL;
            }

            expression->source_span = parser_source_span(operator_token);
            expression->as.unary.operator = unary_operators[i].operator;
            expression->as.unary.operand = operand;
            return expression;
        }
    }

    if (parser_match(parser, TOK_SPAWN)) {
        AstExpression *expression = ast_expression_new(AST_EXPR_SPAWN);
        const Token *spawn_token = parser_previous_token(parser);

        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }

        expression->source_span = parser_source_span(spawn_token);
        if (looks_like_lambda_expression(parser)) {
            expression->as.spawn.callable = parse_lambda_expression(parser);
        } else {
            expression->as.spawn.callable = parse_unary_expression(parser);
        }
        if (!expression->as.spawn.callable) {
            ast_expression_free(expression);
            return NULL;
        }
        return expression;
    }

    return parse_postfix_expression(parser);
}
