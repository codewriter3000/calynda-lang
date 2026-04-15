#include "ast.h"

#include <stdio.h>
#include <string.h>

int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

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

AstExpression *make_identifier_expr(const char *name) {
    AstExpression *expression = ast_expression_new(AST_EXPR_IDENTIFIER);

    if (!expression) {
        return NULL;
    }

    expression->as.identifier = ast_copy_text(name);
    if (!expression->as.identifier) {
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

AstExpression *make_text_literal_expr(AstLiteralKind kind, const char *text) {
    AstExpression *expression = ast_expression_new(AST_EXPR_LITERAL);

    if (!expression) {
        return NULL;
    }

    expression->as.literal.kind = kind;
    expression->as.literal.as.text = ast_copy_text(text);
    if (!expression->as.literal.as.text) {
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

AstExpression *make_binary_expr(AstBinaryOperator operator,
                                       AstExpression *left,
                                       AstExpression *right) {
    AstExpression *expression = ast_expression_new(AST_EXPR_BINARY);

    if (!expression) {
        ast_expression_free(left);
        ast_expression_free(right);
        return NULL;
    }

    expression->as.binary.operator = operator;
    expression->as.binary.left = left;
    expression->as.binary.right = right;
    return expression;
}

AstParameter make_parameter(AstPrimitiveType primitive, const char *name) {
    AstParameter parameter;

    memset(&parameter, 0, sizeof(parameter));
    ast_type_init_primitive(&parameter.type, primitive);
    parameter.name = ast_copy_text(name);
    return parameter;
}

void test_program_structure(void);
void test_expression_forms(void);
void test_template_literal(void);


int main(void) {
    printf("Running AST tests...\n\n");

    RUN_TEST(test_program_structure);
    RUN_TEST(test_expression_forms);
    RUN_TEST(test_template_literal);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
