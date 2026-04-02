#include "../src/parser.h"

#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

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

static void test_parse_program_structure(void) {
    const char *source =
        "package com.example;\n"
        "import io.stdlib;\n"
        "\n"
        "public final int32 add = (int32 a, int32 b) -> a + b;\n"
        "\n"
        "start(string[] args) -> {\n"
        "    var total = add(1, 2 * 3);\n"
        "    return total;\n"
        "};\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse full program");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for full program");
    ASSERT_TRUE(program.has_package, "program has package declaration");
    ASSERT_EQ_INT(2, program.package_name.count, "package segment count");
    ASSERT_EQ_STR("example", program.package_name.segments[1], "package tail segment");
    ASSERT_EQ_INT(1, program.import_count, "import count");
    ASSERT_EQ_INT(2, program.top_level_count, "top-level declaration count");
    ASSERT_EQ_INT(AST_TOP_LEVEL_BINDING, program.top_level_decls[0]->kind,
                  "binding declaration kind");
    ASSERT_EQ_INT(2, program.top_level_decls[0]->as.binding_decl.modifier_count,
                  "binding modifier count");
    ASSERT_EQ_STR("add", program.top_level_decls[0]->as.binding_decl.name,
                  "binding name");
    ASSERT_EQ_INT(AST_EXPR_LAMBDA,
                  program.top_level_decls[0]->as.binding_decl.initializer->kind,
                  "binding initializer kind");
    ASSERT_EQ_INT(2,
                  program.top_level_decls[0]->as.binding_decl.initializer
                      ->as.lambda.parameters.count,
                  "lambda parameter count");
    ASSERT_EQ_INT(AST_LAMBDA_BODY_EXPRESSION,
                  program.top_level_decls[0]->as.binding_decl.initializer
                      ->as.lambda.body.kind,
                  "lambda body kind");
    ASSERT_EQ_INT(AST_EXPR_BINARY,
                  program.top_level_decls[0]->as.binding_decl.initializer
                      ->as.lambda.body.as.expression->kind,
                  "lambda body expression kind");
    ASSERT_EQ_INT(AST_TOP_LEVEL_START, program.top_level_decls[1]->kind,
                  "start declaration kind");
    ASSERT_EQ_INT(1,
                  program.top_level_decls[1]->as.start_decl.parameters.count,
                  "start parameter count");
    ASSERT_EQ_INT(AST_PRIMITIVE_STRING,
                  program.top_level_decls[1]->as.start_decl.parameters.items[0]
                      .type.primitive,
                  "start parameter primitive type");
    ASSERT_EQ_INT(1,
                  program.top_level_decls[1]->as.start_decl.parameters.items[0]
                      .type.dimension_count,
                  "start parameter array dimensions");
    ASSERT_EQ_INT(AST_LAMBDA_BODY_BLOCK,
                  program.top_level_decls[1]->as.start_decl.body.kind,
                  "start body kind");
    ASSERT_EQ_INT(2,
                  program.top_level_decls[1]->as.start_decl.body.as.block
                      ->statement_count,
                  "start block statement count");
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING,
                  program.top_level_decls[1]->as.start_decl.body.as.block
                      ->statements[0]->kind,
                  "first start statement kind");
    ASSERT_EQ_STR("total",
                  program.top_level_decls[1]->as.start_decl.body.as.block
                      ->statements[0]->as.local_binding.name,
                  "local binding name");
    ASSERT_EQ_INT(AST_EXPR_CALL,
                  program.top_level_decls[1]->as.start_decl.body.as.block
                      ->statements[0]->as.local_binding.initializer->kind,
                  "local binding initializer kind");
    ASSERT_EQ_INT(2,
                  program.top_level_decls[1]->as.start_decl.body.as.block
                      ->statements[0]->as.local_binding.initializer->as.call.arguments.count,
                  "call argument count");
    ASSERT_EQ_INT(AST_EXPR_BINARY,
                  program.top_level_decls[1]->as.start_decl.body.as.block
                      ->statements[0]->as.local_binding.initializer->as.call.arguments.items[1]
                      ->kind,
                  "second call argument precedence kind");
    ASSERT_EQ_INT(AST_STMT_RETURN,
                  program.top_level_decls[1]->as.start_decl.body.as.block
                      ->statements[1]->kind,
                  "return statement kind");
    ASSERT_EQ_INT(AST_EXPR_IDENTIFIER,
                  program.top_level_decls[1]->as.start_decl.body.as.block
                      ->statements[1]->as.return_expression->kind,
                  "return expression kind");
    ASSERT_EQ_STR("total",
                  program.top_level_decls[1]->as.start_decl.body.as.block
                      ->statements[1]->as.return_expression->as.identifier,
                  "return identifier name");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_expression_precedence(void) {
    Parser parser;
    AstExpression *expression;

    parser_init(&parser, "foo(1, 2)[0].bar + int32(-value)");
    expression = parser_parse_expression(&parser);

    REQUIRE_TRUE(expression != NULL, "parse precedence expression");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no error for precedence expression");
    ASSERT_EQ_INT(AST_EXPR_BINARY, expression->kind, "root expression kind");
    ASSERT_EQ_INT(AST_BINARY_OP_ADD, expression->as.binary.operator,
                  "root binary operator");
    ASSERT_EQ_INT(AST_EXPR_MEMBER, expression->as.binary.left->kind,
                  "left operand member access");
    ASSERT_EQ_STR("bar", expression->as.binary.left->as.member.member,
                  "member access name");
    ASSERT_EQ_INT(AST_EXPR_INDEX,
                  expression->as.binary.left->as.member.target->kind,
                  "member target is index expression");
    ASSERT_EQ_INT(AST_EXPR_CALL,
                  expression->as.binary.left->as.member.target->as.index.target->kind,
                  "index target is call expression");
    ASSERT_EQ_INT(2,
                  expression->as.binary.left->as.member.target->as.index.target
                      ->as.call.arguments.count,
                  "call argument count");
    ASSERT_EQ_INT(AST_EXPR_CAST, expression->as.binary.right->kind,
                  "right operand cast expression");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, expression->as.binary.right->as.cast.target_type,
                  "cast target type");
    ASSERT_EQ_INT(AST_EXPR_UNARY,
                  expression->as.binary.right->as.cast.expression->kind,
                  "cast operand unary expression");
    ASSERT_EQ_INT(AST_UNARY_OP_NEGATE,
                  expression->as.binary.right->as.cast.expression->as.unary.operator,
                  "unary operator kind");
    ASSERT_EQ_STR("value",
                  expression->as.binary.right->as.cast.expression->as.unary.operand
                      ->as.identifier,
                  "unary operand identifier");

    ast_expression_free(expression);
    parser_free(&parser);
}

static void test_parse_assignment_and_ternary(void) {
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

static void test_parse_template_literal(void) {
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

static void test_parse_errors(void) {
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

int main(void) {
    printf("Running parser tests...\n\n");

    RUN_TEST(test_parse_program_structure);
    RUN_TEST(test_parse_expression_precedence);
    RUN_TEST(test_parse_assignment_and_ternary);
    RUN_TEST(test_parse_template_literal);
    RUN_TEST(test_parse_errors);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}