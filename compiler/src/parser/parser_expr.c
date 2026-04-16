#include "parser_internal.h"

static const AssignmentOperatorMapping assignment_operators[] = {
    {TOK_ASSIGN, AST_ASSIGN_OP_ASSIGN},
    {TOK_PLUS_ASSIGN, AST_ASSIGN_OP_ADD},
    {TOK_MINUS_ASSIGN, AST_ASSIGN_OP_SUBTRACT},
    {TOK_STAR_ASSIGN, AST_ASSIGN_OP_MULTIPLY},
    {TOK_SLASH_ASSIGN, AST_ASSIGN_OP_DIVIDE},
    {TOK_PERCENT_ASSIGN, AST_ASSIGN_OP_MODULO},
    {TOK_AMP_ASSIGN, AST_ASSIGN_OP_BIT_AND},
    {TOK_PIPE_ASSIGN, AST_ASSIGN_OP_BIT_OR},
    {TOK_CARET_ASSIGN, AST_ASSIGN_OP_BIT_XOR},
    {TOK_LSHIFT_ASSIGN, AST_ASSIGN_OP_SHIFT_LEFT},
    {TOK_RSHIFT_ASSIGN, AST_ASSIGN_OP_SHIFT_RIGHT}
};

static const UnaryOperatorMapping unary_operators[] = {
    {TOK_BANG, AST_UNARY_OP_LOGICAL_NOT},
    {TOK_TILDE, AST_UNARY_OP_BITWISE_NOT},
    {TOK_MINUS, AST_UNARY_OP_NEGATE},
    {TOK_PLUS, AST_UNARY_OP_PLUS}
};

AstExpression *parse_expression_node(Parser *parser) {
    if (looks_like_lambda_expression(parser)) {
        return parse_lambda_expression(parser);
    }
    return parse_assignment_expression(parser);
}

AstExpression *parse_lambda_expression(Parser *parser) {
    AstExpression *expression = ast_expression_new(AST_EXPR_LAMBDA);

    if (!expression) {
        parser_set_oom_error(parser);
        return NULL;
    }

    expression->source_span = parser_source_span(parser_current_token(parser));

    if (!parser_consume(parser, TOK_LPAREN, "Expected '(' to start lambda parameters.") ||
        !parser_parse_parameter_list(parser, &expression->as.lambda.parameters, true) ||
        !parser_consume(parser, TOK_RPAREN, "Expected ')' after lambda parameters.") ||
        !parser_consume(parser, TOK_ARROW, "Expected '->' after lambda parameters.") ||
        !parser_parse_lambda_body(parser, &expression->as.lambda.body)) {
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

AstExpression *parse_assignment_expression(Parser *parser) {
    AstExpression *expression = parse_ternary_expression(parser);
    size_t i;

    if (!expression) {
        return NULL;
    }

    for (i = 0; i < sizeof(assignment_operators) / sizeof(assignment_operators[0]); i++) {
        if (parser_check(parser, assignment_operators[i].token_type)) {
            AstExpression *assignment = ast_expression_new(AST_EXPR_ASSIGNMENT);
            AstExpression *value;

            if (!assignment) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            parser_advance(parser);
            value = parse_assignment_expression(parser);
            if (!value) {
                ast_expression_free(assignment);
                ast_expression_free(expression);
                return NULL;
            }

            assignment->source_span = expression->source_span;
            assignment->as.assignment.operator = assignment_operators[i].operator;
            assignment->as.assignment.target = expression;
            assignment->as.assignment.value = value;
            return assignment;
        }
    }

    return expression;
}

AstExpression *parse_ternary_expression(Parser *parser) {
    AstExpression *condition = parse_logical_or_expression(parser);

    if (!condition) {
        return NULL;
    }

    if (parser_match(parser, TOK_QUESTION)) {
        AstExpression *expression = ast_expression_new(AST_EXPR_TERNARY);
        AstExpression *then_branch;
        AstExpression *else_branch;

        if (!expression) {
            parser_set_oom_error(parser);
            ast_expression_free(condition);
            return NULL;
        }

        then_branch = parse_expression_node(parser);
        if (!then_branch) {
            ast_expression_free(condition);
            ast_expression_free(expression);
            return NULL;
        }

        if (!parser_consume(parser, TOK_COLON,
                            "Expected ':' after ternary true branch.")) {
            ast_expression_free(condition);
            ast_expression_free(then_branch);
            ast_expression_free(expression);
            return NULL;
        }

        else_branch = parse_ternary_expression(parser);
        if (!else_branch) {
            ast_expression_free(condition);
            ast_expression_free(then_branch);
            ast_expression_free(expression);
            return NULL;
        }

        expression->source_span = condition->source_span;
        expression->as.ternary.condition = condition;
        expression->as.ternary.then_branch = then_branch;
        expression->as.ternary.else_branch = else_branch;
        return expression;
    }

    return condition;
}

AstExpression *parse_unary_expression(Parser *parser) {
    size_t i;

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
