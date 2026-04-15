#include "parser.h"

#include <stdio.h>
#include <string.h>

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

#define ASSERT_EQ_INT(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %d, got %d\n",      \
                __FILE__, __LINE__, (msg), (int)(expected), (int)(actual)); \
    }                                                                       \
} while (0)
#define ASSERT_TRUE(condition, msg) do {                                    \
    tests_run++;                                                            \
    if (condition) {                                                        \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n",                            \
                __FILE__, __LINE__, (msg));                                 \
    }                                                                       \
} while (0)
#define REQUIRE_TRUE(condition, msg) do {                                   \
    tests_run++;                                                            \
    if (condition) {                                                        \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n",                            \
                __FILE__, __LINE__, (msg));                                 \
        return;                                                             \
    }                                                                       \
} while (0)
#define ASSERT_EQ_STR(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((actual) != NULL && strcmp((expected), (actual)) == 0) {            \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected \"%s\", got \"%s\"\n", \
                __FILE__, __LINE__, (msg), (expected),                      \
                (actual) ? (actual) : "(null)");                           \
    }                                                                       \
} while (0)
#define ASSERT_CONTAINS(needle, haystack, msg) do {                         \
    tests_run++;                                                            \
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {       \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\" in \"%s\"\n", \
                __FILE__, __LINE__, (msg), (needle),                        \
                (haystack) ? (haystack) : "(null)");                       \
    }                                                                       \
} while (0)
#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)


void test_parse_assignment_and_ternary(void) {
    Parser parser;
    AstExpression *expression;

    parser_init(&parser, "result = a = b ? c : d");
    expression = parser_parse_expression(&parser);

    REQUIRE_TRUE(expression != NULL, "parse assignment and ternary expression");
    ASSERT_EQ_INT(AST_EXPR_ASSIGNMENT, expression->kind, "outer assignment kind");
    ASSERT_EQ_STR("result", expression->as.assignment.target->as.identifier,
                  "outer assignment target");
    ASSERT_EQ_INT(AST_EXPR_ASSIGNMENT, expression->as.assignment.value->kind,
                  "inner assignment kind");
    ASSERT_EQ_STR("a", expression->as.assignment.value->as.assignment.target->as.identifier,
                  "inner assignment target");
    ASSERT_EQ_INT(AST_EXPR_TERNARY,
                  expression->as.assignment.value->as.assignment.value->kind,
                  "inner assignment value ternary kind");
    ASSERT_EQ_STR("b",
                  expression->as.assignment.value->as.assignment.value
                      ->as.ternary.condition->as.identifier,
                  "ternary condition identifier");
    ASSERT_EQ_STR("c",
                  expression->as.assignment.value->as.assignment.value
                      ->as.ternary.then_branch->as.identifier,
                  "ternary then branch identifier");
    ASSERT_EQ_STR("d",
                  expression->as.assignment.value->as.assignment.value
                      ->as.ternary.else_branch->as.identifier,
                  "ternary else branch identifier");

    ast_expression_free(expression);
    parser_free(&parser);
}


void test_parse_template_literal(void) {
    Parser parser;
    AstExpression *expression;

    parser_init(&parser, "`prefix ${(() -> { return 1; })()} suffix`");
    expression = parser_parse_expression(&parser);

    REQUIRE_TRUE(expression != NULL, "parse template literal expression");
    ASSERT_EQ_INT(AST_EXPR_LITERAL, expression->kind, "template expression kind");
    ASSERT_EQ_INT(AST_LITERAL_TEMPLATE, expression->as.literal.kind,
                  "template literal kind");
    ASSERT_EQ_INT(3, expression->as.literal.as.template_parts.count,
                  "template part count");
    ASSERT_EQ_STR("prefix ",
                  expression->as.literal.as.template_parts.items[0].as.text,
                  "template prefix text");
    ASSERT_EQ_INT(AST_TEMPLATE_PART_EXPRESSION,
                  expression->as.literal.as.template_parts.items[1].kind,
                  "interpolation part kind");
    ASSERT_EQ_INT(AST_EXPR_CALL,
                  expression->as.literal.as.template_parts.items[1].as.expression->kind,
                  "interpolation expression kind");
    ASSERT_EQ_INT(AST_EXPR_GROUPING,
                  expression->as.literal.as.template_parts.items[1].as.expression
                      ->as.call.callee->kind,
                  "interpolation callee grouping kind");
    ASSERT_EQ_INT(AST_EXPR_LAMBDA,
                  expression->as.literal.as.template_parts.items[1].as.expression
                      ->as.call.callee->as.grouping.inner->kind,
                  "grouped expression lambda kind");
    ASSERT_EQ_INT(AST_LAMBDA_BODY_BLOCK,
                  expression->as.literal.as.template_parts.items[1].as.expression
                      ->as.call.callee->as.grouping.inner->as.lambda.body.kind,
                  "lambda block body kind");
    ASSERT_EQ_STR(" suffix",
                  expression->as.literal.as.template_parts.items[2].as.text,
                  "template suffix text");

    ast_expression_free(expression);
    parser_free(&parser);
}


void test_parse_errors(void) {
    const ParserError *error;
    Parser parser;
    AstProgram program;

    parser_init(&parser, "package com.example\nstart(string[] args) -> 0;\n");
    ASSERT_TRUE(!parser_parse_program(&parser, &program),
                "missing package semicolon produces parse error");
    error = parser_get_error(&parser);
    REQUIRE_TRUE(error != NULL, "parse error is available");
    ASSERT_EQ_INT(TOK_START, error->token.type, "error token type for missing semicolon");
    ASSERT_EQ_INT(2, error->token.line, "error line for missing semicolon");
    ASSERT_EQ_INT(1, error->token.column, "error column for missing semicolon");
    ASSERT_CONTAINS("package declaration", error->message,
                    "error message for missing package semicolon");
    parser_free(&parser);

    parser_init(&parser, "var broken = \"unterminated");
    ASSERT_TRUE(!parser_parse_program(&parser, &program),
                "lexer error surfaces through parser");
    error = parser_get_error(&parser);
    REQUIRE_TRUE(error != NULL, "lexer error is available");
    ASSERT_EQ_INT(TOK_ERROR, error->token.type, "lexer error token type");
    ASSERT_CONTAINS("Unterminated string literal", error->message,
                    "lexer error message text");
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2 feature: prefix and postfix ++/--                               */
/* ------------------------------------------------------------------ */
void test_parse_prefix_increment_decrement(void) {
    Parser parser;
    AstExpression *expression;

    parser_init(&parser, "++x");
    expression = parser_parse_expression(&parser);
    REQUIRE_TRUE(expression != NULL, "parse prefix increment");
    ASSERT_EQ_INT(AST_EXPR_UNARY, expression->kind, "prefix ++ is unary expression");
    ASSERT_EQ_INT(AST_UNARY_OP_PRE_INCREMENT, expression->as.unary.operator,
                  "prefix ++ operator");
    ASSERT_EQ_STR("x", expression->as.unary.operand->as.identifier,
                  "prefix ++ operand");
    ast_expression_free(expression);
    parser_free(&parser);

    parser_init(&parser, "--y");
    expression = parser_parse_expression(&parser);
    REQUIRE_TRUE(expression != NULL, "parse prefix decrement");
    ASSERT_EQ_INT(AST_EXPR_UNARY, expression->kind, "prefix -- is unary expression");
    ASSERT_EQ_INT(AST_UNARY_OP_PRE_DECREMENT, expression->as.unary.operator,
                  "prefix -- operator");
    ASSERT_EQ_STR("y", expression->as.unary.operand->as.identifier,
                  "prefix -- operand");
    ast_expression_free(expression);
    parser_free(&parser);
}


void test_parse_postfix_increment_decrement(void) {
    Parser parser;
    AstExpression *expression;

    parser_init(&parser, "x++");
    expression = parser_parse_expression(&parser);
    REQUIRE_TRUE(expression != NULL, "parse postfix increment");
    ASSERT_EQ_INT(AST_EXPR_POST_INCREMENT, expression->kind,
                  "postfix ++ expression kind");
    ASSERT_EQ_STR("x", expression->as.post_increment.operand->as.identifier,
                  "postfix ++ operand");
    ast_expression_free(expression);
    parser_free(&parser);

    parser_init(&parser, "y--");
    expression = parser_parse_expression(&parser);
    REQUIRE_TRUE(expression != NULL, "parse postfix decrement");
    ASSERT_EQ_INT(AST_EXPR_POST_DECREMENT, expression->kind,
                  "postfix -- expression kind");
    ASSERT_EQ_STR("y", expression->as.post_decrement.operand->as.identifier,
                  "postfix -- operand");
    ast_expression_free(expression);
    parser_free(&parser);
}

