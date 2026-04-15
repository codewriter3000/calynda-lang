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


void test_parse_program_structure(void) {
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


void test_parse_expression_precedence(void) {
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

