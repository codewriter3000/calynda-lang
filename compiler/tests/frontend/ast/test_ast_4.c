#include "ast.h"

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
#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

AstExpression *make_identifier_expr(const char *name);
AstExpression *make_text_literal_expr(AstLiteralKind kind, const char *text);
AstExpression *make_binary_expr(AstBinaryOperator operator,
                                       AstExpression *left,
                                       AstExpression *right);
AstParameter make_parameter(AstPrimitiveType primitive, const char *name);


void test_template_literal(void) {
    AstExpression *template_expr = ast_expression_new(AST_EXPR_LITERAL);

    REQUIRE_TRUE(template_expr != NULL, "allocate template literal expression");
    template_expr->as.literal.kind = AST_LITERAL_TEMPLATE;

    REQUIRE_TRUE(ast_template_literal_append_text(&template_expr->as.literal,
                                                  "Hello "),
                 "append first template text part");
    REQUIRE_TRUE(ast_template_literal_append_expression(&template_expr->as.literal,
                                                        make_identifier_expr("name")),
                 "append template identifier expression");
    REQUIRE_TRUE(ast_template_literal_append_text(&template_expr->as.literal,
                                                  ", count="),
                 "append second template text part");
    REQUIRE_TRUE(ast_template_literal_append_expression(&template_expr->as.literal,
                                                        make_text_literal_expr(AST_LITERAL_INTEGER,
                                                                               "3")),
                 "append template integer expression");

    ASSERT_EQ_INT(AST_LITERAL_TEMPLATE, template_expr->as.literal.kind,
                  "template literal kind");
    ASSERT_EQ_INT(4, template_expr->as.literal.as.template_parts.count,
                  "template part count");
    ASSERT_EQ_INT(AST_TEMPLATE_PART_TEXT,
                  template_expr->as.literal.as.template_parts.items[0].kind,
                  "first template part kind");
    ASSERT_EQ_STR("Hello ",
                  template_expr->as.literal.as.template_parts.items[0].as.text,
                  "first template part text");
    ASSERT_EQ_INT(AST_TEMPLATE_PART_EXPRESSION,
                  template_expr->as.literal.as.template_parts.items[1].kind,
                  "second template part kind");
    ASSERT_EQ_INT(AST_EXPR_IDENTIFIER,
                  template_expr->as.literal.as.template_parts.items[1].as.expression->kind,
                  "template identifier expression kind");
    ASSERT_EQ_INT(AST_TEMPLATE_PART_TEXT,
                  template_expr->as.literal.as.template_parts.items[2].kind,
                  "third template part kind");
    ASSERT_EQ_STR(", count=",
                  template_expr->as.literal.as.template_parts.items[2].as.text,
                  "second template part text");
    ASSERT_EQ_INT(AST_EXPR_LITERAL,
                  template_expr->as.literal.as.template_parts.items[3].as.expression->kind,
                  "template literal expression kind");

    ast_expression_free(template_expr);
}

