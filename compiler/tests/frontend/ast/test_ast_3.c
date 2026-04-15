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


void test_expression_forms(void) {
    AstExpression *lambda_expr;
    AstParameter parameter;
    AstExpression *assignment_expr;
    AstExpression *then_branch;
    AstExpression *member_expr;
    AstExpression *call_expr;

    lambda_expr = ast_expression_new(AST_EXPR_LAMBDA);
    REQUIRE_TRUE(lambda_expr != NULL, "allocate lambda expression");
    parameter = make_parameter(AST_PRIMITIVE_INT32, "value");
    REQUIRE_TRUE(parameter.name != NULL, "copy lambda parameter name");
    REQUIRE_TRUE(ast_parameter_list_append(&lambda_expr->as.lambda.parameters, &parameter),
                 "append lambda parameter");
    lambda_expr->as.lambda.body.kind = AST_LAMBDA_BODY_EXPRESSION;
    lambda_expr->as.lambda.body.as.expression =
        make_binary_expr(AST_BINARY_OP_ADD,
                         make_identifier_expr("value"),
                         make_text_literal_expr(AST_LITERAL_INTEGER, "1"));
    REQUIRE_TRUE(lambda_expr->as.lambda.body.as.expression != NULL,
                 "build lambda body expression");

    ASSERT_EQ_INT(AST_EXPR_LAMBDA, lambda_expr->kind, "lambda expression kind");
    ASSERT_EQ_INT(1, lambda_expr->as.lambda.parameters.count,
                  "lambda parameter count");
    ASSERT_EQ_INT(AST_EXPR_BINARY,
                  lambda_expr->as.lambda.body.as.expression->kind,
                  "lambda body expression kind");
    ASSERT_EQ_INT(AST_BINARY_OP_ADD,
                  lambda_expr->as.lambda.body.as.expression->as.binary.operator,
                  "lambda binary operator");

    assignment_expr = ast_expression_new(AST_EXPR_ASSIGNMENT);
    REQUIRE_TRUE(assignment_expr != NULL, "allocate assignment expression");
    assignment_expr->as.assignment.operator = AST_ASSIGN_OP_ADD;
    assignment_expr->as.assignment.target = make_identifier_expr("result");
    REQUIRE_TRUE(assignment_expr->as.assignment.target != NULL,
                 "create assignment target");
    assignment_expr->as.assignment.value = ast_expression_new(AST_EXPR_TERNARY);
    REQUIRE_TRUE(assignment_expr->as.assignment.value != NULL,
                 "allocate ternary expression");
    assignment_expr->as.assignment.value->as.ternary.condition =
        make_identifier_expr("ready");
    REQUIRE_TRUE(assignment_expr->as.assignment.value->as.ternary.condition != NULL,
                 "create ternary condition");

    then_branch = ast_expression_new(AST_EXPR_CAST);
    REQUIRE_TRUE(then_branch != NULL, "allocate cast expression");
    then_branch->as.cast.target_type = AST_PRIMITIVE_INT32;

    member_expr = ast_expression_new(AST_EXPR_MEMBER);
    REQUIRE_TRUE(member_expr != NULL, "allocate member expression");
    member_expr->as.member.target = ast_expression_new(AST_EXPR_INDEX);
    REQUIRE_TRUE(member_expr->as.member.target != NULL,
                 "allocate index expression");
    member_expr->as.member.target->as.index.target = make_identifier_expr("items");
    REQUIRE_TRUE(member_expr->as.member.target->as.index.target != NULL,
                 "create index target");
    member_expr->as.member.target->as.index.index =
        make_text_literal_expr(AST_LITERAL_INTEGER, "0");
    REQUIRE_TRUE(member_expr->as.member.target->as.index.index != NULL,
                 "create index operand");
    member_expr->as.member.member = ast_copy_text("count");
    REQUIRE_TRUE(member_expr->as.member.member != NULL, "copy member name");
    then_branch->as.cast.expression = member_expr;
    assignment_expr->as.assignment.value->as.ternary.then_branch = then_branch;

    assignment_expr->as.assignment.value->as.ternary.else_branch =
        ast_expression_new(AST_EXPR_ARRAY_LITERAL);
    REQUIRE_TRUE(assignment_expr->as.assignment.value->as.ternary.else_branch != NULL,
                 "allocate array literal expression");
    REQUIRE_TRUE(ast_expression_list_append(
                     &assignment_expr->as.assignment.value->as.ternary.else_branch
                          ->as.array_literal.elements,
                     make_text_literal_expr(AST_LITERAL_INTEGER, "1")),
                 "append first array element");
    REQUIRE_TRUE(ast_expression_list_append(
                     &assignment_expr->as.assignment.value->as.ternary.else_branch
                          ->as.array_literal.elements,
                     make_text_literal_expr(AST_LITERAL_INTEGER, "2")),
                 "append second array element");

    ASSERT_EQ_INT(AST_EXPR_ASSIGNMENT, assignment_expr->kind,
                  "assignment expression kind");
    ASSERT_EQ_INT(AST_ASSIGN_OP_ADD, assignment_expr->as.assignment.operator,
                  "assignment operator");
    ASSERT_EQ_INT(AST_EXPR_TERNARY, assignment_expr->as.assignment.value->kind,
                  "assignment value kind");
    ASSERT_EQ_INT(AST_EXPR_CAST,
                  assignment_expr->as.assignment.value->as.ternary.then_branch->kind,
                  "ternary then-branch kind");
    ASSERT_EQ_INT(AST_EXPR_MEMBER,
                  assignment_expr->as.assignment.value->as.ternary.then_branch
                      ->as.cast.expression->kind,
                  "cast operand kind");
    ASSERT_EQ_INT(AST_EXPR_INDEX,
                  assignment_expr->as.assignment.value->as.ternary.then_branch
                      ->as.cast.expression->as.member.target->kind,
                  "member target kind");
    ASSERT_EQ_INT(AST_EXPR_ARRAY_LITERAL,
                  assignment_expr->as.assignment.value->as.ternary.else_branch->kind,
                  "ternary else-branch kind");
    ASSERT_EQ_INT(2,
                  assignment_expr->as.assignment.value->as.ternary.else_branch
                      ->as.array_literal.elements.count,
                  "array literal element count");

    call_expr = ast_expression_new(AST_EXPR_CALL);
    REQUIRE_TRUE(call_expr != NULL, "allocate call expression");
    call_expr->as.call.callee = make_identifier_expr("emit");
    REQUIRE_TRUE(call_expr->as.call.callee != NULL, "create emit callee");
    call_expr->as.call.arguments.items = NULL;
    call_expr->as.call.arguments.count = 0;
    call_expr->as.call.arguments.capacity = 0;
    call_expr->as.call.arguments.count = 0;
    call_expr->as.call.arguments.capacity = 0;
    call_expr->as.call.arguments.items = NULL;
    call_expr->as.call.arguments.count = 0;

    {
        AstExpression *grouping_expr = ast_expression_new(AST_EXPR_GROUPING);
        AstExpression *unary_expr = ast_expression_new(AST_EXPR_UNARY);

        REQUIRE_TRUE(grouping_expr != NULL, "allocate grouping expression");
        REQUIRE_TRUE(unary_expr != NULL, "allocate unary expression");
        unary_expr->as.unary.operator = AST_UNARY_OP_NEGATE;
        unary_expr->as.unary.operand = make_identifier_expr("delta");
        REQUIRE_TRUE(unary_expr->as.unary.operand != NULL,
                     "create unary operand");
        grouping_expr->as.grouping.inner = unary_expr;
        REQUIRE_TRUE(ast_expression_list_append(&call_expr->as.call.arguments,
                                                grouping_expr),
                     "append grouped unary argument");
    }

    ASSERT_EQ_INT(AST_EXPR_CALL, call_expr->kind, "call expression kind");
    ASSERT_EQ_INT(1, call_expr->as.call.arguments.count, "call argument count");
    ASSERT_EQ_INT(AST_EXPR_GROUPING, call_expr->as.call.arguments.items[0]->kind,
                  "call argument grouping kind");
    ASSERT_EQ_INT(AST_EXPR_UNARY,
                  call_expr->as.call.arguments.items[0]->as.grouping.inner->kind,
                  "grouping inner kind");
    ASSERT_EQ_INT(AST_UNARY_OP_NEGATE,
                  call_expr->as.call.arguments.items[0]->as.grouping.inner->as.unary.operator,
                  "unary operator kind");

    ast_expression_free(lambda_expr);
    ast_expression_free(assignment_expr);
    ast_expression_free(call_expr);
}

