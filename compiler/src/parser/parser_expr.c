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

static bool parse_manual_lambda_body(Parser *parser,
                                     AstLambdaBody *body,
                                     AstSourceSpan manual_span) {
    AstBlock *manual_body;
    AstBlock *wrapper_block;
    AstStatement *manual_statement;

    if (!parser_check(parser, TOK_LBRACE)) {
        parser_set_error(parser,
                         *parser_current_token(parser),
                         "Manual lambda shorthand requires a block body.");
        return false;
    }

    manual_body = parse_block(parser);
    if (!manual_body) {
        return false;
    }

    manual_statement = ast_statement_new(AST_STMT_MANUAL);
    if (!manual_statement) {
        parser_set_oom_error(parser);
        ast_block_free(manual_body);
        return false;
    }
    manual_statement->source_span = manual_span;
    manual_statement->as.manual.body = manual_body;

    wrapper_block = ast_block_new();
    if (!wrapper_block) {
        parser_set_oom_error(parser);
        ast_statement_free(manual_statement);
        return false;
    }

    if (!parser_add_statement(parser, wrapper_block, manual_statement)) {
        ast_block_free(wrapper_block);
        return false;
    }

    body->kind = AST_LAMBDA_BODY_BLOCK;
    body->as.block = wrapper_block;
    return true;
}

AstExpression *parse_expression_node(Parser *parser) {
    if (looks_like_lambda_expression(parser)) {
        return parse_lambda_expression(parser);
    }
    return parse_assignment_expression(parser);
}

AstExpression *parse_lambda_expression(Parser *parser) {
    AstExpression *expression = ast_expression_new(AST_EXPR_LAMBDA);
    const Token *start_token = parser_current_token(parser);
    const Token *manual_token = NULL;

    if (!expression) {
        parser_set_oom_error(parser);
        return NULL;
    }

    expression->source_span = parser_source_span(start_token);

    /* shorthand: identifier -> body  ≡  (var identifier) -> body */
    if (parser_check(parser, TOK_IDENTIFIER)) {
        AstParameter shorthand_param;
        const Token *name_token = parser_current_token(parser);

        memset(&shorthand_param, 0, sizeof(shorthand_param));
        shorthand_param.is_untyped = true;
        shorthand_param.name_span  = parser_source_span(name_token);
        shorthand_param.name = parser_consume_identifier(
            parser, "Expected parameter name in shorthand lambda.");
        if (!shorthand_param.name) {
            ast_expression_free(expression);
            return NULL;
        }

        if (!parser_consume(parser, TOK_ARROW,
                            "Expected '->' in shorthand lambda.")) {
            free(shorthand_param.name);
            ast_expression_free(expression);
            return NULL;
        }

        if (!parser_add_parameter(parser, &expression->as.lambda.parameters,
                                  &shorthand_param)) {
            ast_expression_free(expression);
            return NULL;
        }

        if (!parser_parse_lambda_body(parser, &expression->as.lambda.body)) {
            ast_expression_free(expression);
            return NULL;
        }

        return expression;
    }

    if (parser_match(parser, TOK_MANUAL)) {
        manual_token = parser_previous_token(parser);
    }

    if (!parser_consume(parser, TOK_LPAREN, "Expected '(' to start lambda parameters.") ||
        !parser_parse_parameter_list(parser, &expression->as.lambda.parameters, true) ||
        !parser_consume(parser, TOK_RPAREN, "Expected ')' after lambda parameters.") ||
        !parser_consume(parser, TOK_ARROW, "Expected '->' after lambda parameters.")) {
        ast_expression_free(expression);
        return NULL;
    }

    if (manual_token) {
        if (!parse_manual_lambda_body(parser,
                                      &expression->as.lambda.body,
                                      parser_source_span(manual_token))) {
            ast_expression_free(expression);
            return NULL;
        }
    } else if (!parser_parse_lambda_body(parser, &expression->as.lambda.body)) {
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

#include "parser_expr_p2.inc"
