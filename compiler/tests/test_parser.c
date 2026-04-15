#include "parser.h"

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

/* ------------------------------------------------------------------ */
/* V2 feature: prefix and postfix ++/--                               */
/* ------------------------------------------------------------------ */
static void test_parse_prefix_increment_decrement(void) {
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

static void test_parse_postfix_increment_decrement(void) {
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

/* ------------------------------------------------------------------ */
/* V2 feature: discard expression _                                   */
/* ------------------------------------------------------------------ */
static void test_parse_discard_expression(void) {
    Parser parser;
    AstExpression *expression;

    parser_init(&parser, "_ = foo()");
    expression = parser_parse_expression(&parser);
    REQUIRE_TRUE(expression != NULL, "parse discard assignment");
    ASSERT_EQ_INT(AST_EXPR_ASSIGNMENT, expression->kind,
                  "discard assignment kind");
    ASSERT_EQ_INT(AST_EXPR_DISCARD, expression->as.assignment.target->kind,
                  "discard target kind");
    ASSERT_EQ_INT(AST_EXPR_CALL, expression->as.assignment.value->kind,
                  "discard value kind");
    ast_expression_free(expression);
    parser_free(&parser);
}

/* ------------------------------------------------------------------ */
/* V2 feature: varargs parameter (Type... name)                       */
/* ------------------------------------------------------------------ */
static void test_parse_varargs_parameter(void) {
    const char *source = "int32 sum = (int32... nums) -> 0;\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse varargs binding");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for varargs");
    ASSERT_EQ_INT(1, program.top_level_count, "one top-level decl");
    ASSERT_EQ_INT(AST_EXPR_LAMBDA,
                  program.top_level_decls[0]->as.binding_decl.initializer->kind,
                  "initializer is lambda");
    ASSERT_EQ_INT(1,
                  program.top_level_decls[0]->as.binding_decl.initializer
                      ->as.lambda.parameters.count,
                  "lambda parameter count");
    ASSERT_TRUE(program.top_level_decls[0]->as.binding_decl.initializer
                    ->as.lambda.parameters.items[0].is_varargs,
                "parameter is varargs");
    ASSERT_EQ_STR("nums",
                  program.top_level_decls[0]->as.binding_decl.initializer
                      ->as.lambda.parameters.items[0].name,
                  "varargs parameter name");
    ast_program_free(&program);
    parser_free(&parser);
}

/* ------------------------------------------------------------------ */
/* V2 feature: export / static / internal modifiers                   */
/* ------------------------------------------------------------------ */
static void test_parse_v2_modifiers(void) {
    const char *source =
        "export int32 x = 5;\n"
        "static final int32 y = 10;\n"
        "internal int32 z = 15;\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse v2 modifiers");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for v2 modifiers");
    ASSERT_EQ_INT(3, program.top_level_count, "three top-level decls");

    ASSERT_EQ_INT(1, program.top_level_decls[0]->as.binding_decl.modifier_count,
                  "export decl modifier count");
    ASSERT_EQ_INT(AST_MODIFIER_EXPORT,
                  program.top_level_decls[0]->as.binding_decl.modifiers[0],
                  "export modifier");

    ASSERT_EQ_INT(2, program.top_level_decls[1]->as.binding_decl.modifier_count,
                  "static final decl modifier count");
    ASSERT_EQ_INT(AST_MODIFIER_STATIC,
                  program.top_level_decls[1]->as.binding_decl.modifiers[0],
                  "static modifier");
    ASSERT_EQ_INT(AST_MODIFIER_FINAL,
                  program.top_level_decls[1]->as.binding_decl.modifiers[1],
                  "final modifier on static decl");

    ASSERT_EQ_INT(1, program.top_level_decls[2]->as.binding_decl.modifier_count,
                  "internal decl modifier count");
    ASSERT_EQ_INT(AST_MODIFIER_INTERNAL,
                  program.top_level_decls[2]->as.binding_decl.modifiers[0],
                  "internal modifier");

    ast_program_free(&program);
    parser_free(&parser);
}

/* ------------------------------------------------------------------ */
/* V2 feature: import alias, wildcard, selective                      */
/* ------------------------------------------------------------------ */
static void test_parse_import_alias(void) {
    const char *source = "import io.stdlib as std;\nint32 x = 0;\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse import alias");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for import alias");
    ASSERT_EQ_INT(1, program.import_count, "one import");
    ASSERT_EQ_INT(AST_IMPORT_ALIAS, program.imports[0].kind, "import alias kind");
    ASSERT_EQ_STR("std", program.imports[0].alias, "import alias name");
    ASSERT_EQ_INT(2, program.imports[0].module_name.count, "import module segments");
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_import_wildcard(void) {
    const char *source = "import io.stdlib.*;\nint32 x = 0;\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse import wildcard");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for import wildcard");
    ASSERT_EQ_INT(1, program.import_count, "one import");
    ASSERT_EQ_INT(AST_IMPORT_WILDCARD, program.imports[0].kind, "import wildcard kind");
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_import_selective(void) {
    const char *source = "import io.stdlib.{print, readLine};\nint32 x = 0;\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse import selective");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for import selective");
    ASSERT_EQ_INT(1, program.import_count, "one import");
    ASSERT_EQ_INT(AST_IMPORT_SELECTIVE, program.imports[0].kind,
                  "import selective kind");
    ASSERT_EQ_INT(2, program.imports[0].selected_count, "selective import count");
    ASSERT_EQ_STR("print", program.imports[0].selected_names[0],
                  "first selective import name");
    ASSERT_EQ_STR("readLine", program.imports[0].selected_names[1],
                  "second selective import name");
    ast_program_free(&program);
    parser_free(&parser);
}

/* ------------------------------------------------------------------ */
/* V2 feature: Java-style primitive aliases                           */
/* ------------------------------------------------------------------ */
static void test_parse_java_primitive_aliases(void) {
    const char *source =
        "int x = 5;\n"
        "double pi = 3.14;\n"
        "byte b = 0;\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse java aliases");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for java aliases");
    ASSERT_EQ_INT(3, program.top_level_count, "three top-level decls");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT,
                  program.top_level_decls[0]->as.binding_decl.declared_type.primitive,
                  "int alias primitive type");
    ASSERT_EQ_INT(AST_PRIMITIVE_DOUBLE,
                  program.top_level_decls[1]->as.binding_decl.declared_type.primitive,
                  "double alias primitive type");
    ASSERT_EQ_INT(AST_PRIMITIVE_BYTE,
                  program.top_level_decls[2]->as.binding_decl.declared_type.primitive,
                  "byte alias primitive type");
    ast_program_free(&program);
    parser_free(&parser);
}

/* ------------------------------------------------------------------ */
/* V2 feature: tagged union declaration                               */
/* ------------------------------------------------------------------ */
static void test_parse_union_declaration(void) {
    const char *source = "union Option<T> { Some(int32), None };\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse union decl");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for union");
    ASSERT_EQ_INT(1, program.top_level_count, "one top-level decl");
    ASSERT_EQ_INT(AST_TOP_LEVEL_UNION, program.top_level_decls[0]->kind,
                  "union kind");
    ASSERT_EQ_STR("Option", program.top_level_decls[0]->as.union_decl.name,
                  "union name");
    ASSERT_EQ_INT(1, program.top_level_decls[0]->as.union_decl.generic_param_count,
                  "union generic param count");
    ASSERT_EQ_INT(2, program.top_level_decls[0]->as.union_decl.variant_count,
                  "union variant count");
    ASSERT_EQ_STR("Some",
                  program.top_level_decls[0]->as.union_decl.variants[0].name,
                  "first variant name");
    ASSERT_TRUE(program.top_level_decls[0]->as.union_decl.variants[0].payload_type != NULL,
                "first variant has payload");
    ASSERT_EQ_STR("None",
                  program.top_level_decls[0]->as.union_decl.variants[1].name,
                  "second variant name");
    ASSERT_TRUE(program.top_level_decls[0]->as.union_decl.variants[1].payload_type == NULL,
                "second variant has no payload");
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_union_with_modifiers(void) {
    const char *source =
        "export union Result<T, E> {\n"
        "    Ok(int32),\n"
        "    Err(string)\n"
        "};\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse export union");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for export union");
    ASSERT_EQ_INT(1, program.top_level_count, "one top-level decl");
    ASSERT_EQ_INT(AST_TOP_LEVEL_UNION, program.top_level_decls[0]->kind,
                  "union kind");
    ASSERT_EQ_STR("Result", program.top_level_decls[0]->as.union_decl.name,
                  "union name");
    ASSERT_EQ_INT(1, program.top_level_decls[0]->as.union_decl.modifier_count,
                  "union modifier count");
    ASSERT_EQ_INT(AST_MODIFIER_EXPORT,
                  program.top_level_decls[0]->as.union_decl.modifiers[0],
                  "union export modifier");
    ASSERT_EQ_INT(2, program.top_level_decls[0]->as.union_decl.generic_param_count,
                  "union generic param count");
    ASSERT_EQ_INT(2, program.top_level_decls[0]->as.union_decl.variant_count,
                  "union variant count");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_internal_local_binding(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    internal void doBreak = () -> { return; };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstStartDecl *start_decl;
    const AstStatement *stmt;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse internal local binding");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for internal local");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    REQUIRE_TRUE(start_decl->body.kind == AST_LAMBDA_BODY_BLOCK,
                 "start has block body");
    REQUIRE_TRUE(start_decl->body.as.block->statement_count >= 1,
                 "start block has statements");

    stmt = start_decl->body.as.block->statements[0];
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, stmt->kind, "first stmt is local binding");
    ASSERT_TRUE(stmt->as.local_binding.is_internal, "local binding is_internal");
    ASSERT_EQ_STR("doBreak", stmt->as.local_binding.name, "local binding name");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_named_type_in_binding(void) {
    const char *source =
        "union Option<T> { Some(T), None };\n"
        "Option<int32> val = 42;\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    const AstBindingDecl *binding;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse named type binding");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for named type");

    ASSERT_EQ_INT(3, (int)program.top_level_count, "three top-level decls");

    binding = &program.top_level_decls[1]->as.binding_decl;
    ASSERT_EQ_INT(AST_TYPE_NAMED, binding->declared_type.kind, "binding has named type");
    ASSERT_EQ_STR("Option", binding->declared_type.name, "named type is Option");
    ASSERT_EQ_INT(1, (int)binding->declared_type.generic_args.count, "one generic arg");
    ASSERT_EQ_INT(AST_GENERIC_ARG_TYPE,
                  binding->declared_type.generic_args.items[0].kind,
                  "generic arg is a type");
    ASSERT_EQ_INT(AST_TYPE_PRIMITIVE,
                  binding->declared_type.generic_args.items[0].type->kind,
                  "generic arg type is primitive");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_arr_wildcard_type(void) {
    const char *source =
        "arr<?> record = [1, \"hello\"];\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    const AstBindingDecl *binding;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse arr wildcard");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for arr<?>") ;

    binding = &program.top_level_decls[0]->as.binding_decl;
    ASSERT_EQ_INT(AST_TYPE_ARR, binding->declared_type.kind, "binding has arr type");
    ASSERT_EQ_INT(1, (int)binding->declared_type.generic_args.count, "one generic arg");
    ASSERT_EQ_INT(AST_GENERIC_ARG_WILDCARD,
                  binding->declared_type.generic_args.items[0].kind,
                  "generic arg is wildcard");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_nested_generic_rshift(void) {
    const char *source =
        "union Box<T> { Val(T), Empty };\n"
        "union Option<T> { Some(T), None };\n"
        "Option<Box<int32>> nested = 42;\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    const AstBindingDecl *binding;
    const AstGenericArg *outer_arg;
    const AstGenericArg *inner_arg;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse nested>>generic");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for nested>>generic");

    binding = &program.top_level_decls[2]->as.binding_decl;
    ASSERT_EQ_INT(AST_TYPE_NAMED, binding->declared_type.kind, "outer named type");
    ASSERT_EQ_STR("Option", binding->declared_type.name, "outer is Option");
    ASSERT_EQ_INT(1, (int)binding->declared_type.generic_args.count, "one outer generic arg");

    outer_arg = &binding->declared_type.generic_args.items[0];
    ASSERT_EQ_INT(AST_GENERIC_ARG_TYPE, outer_arg->kind, "outer arg is type");
    ASSERT_EQ_INT(AST_TYPE_NAMED, outer_arg->type->kind, "inner is named type");
    ASSERT_EQ_STR("Box", outer_arg->type->name, "inner named type is Box");
    ASSERT_EQ_INT(1, (int)outer_arg->type->generic_args.count, "one inner generic arg");

    inner_arg = &outer_arg->type->generic_args.items[0];
    ASSERT_EQ_INT(AST_GENERIC_ARG_TYPE, inner_arg->kind, "inner arg is type");
    ASSERT_EQ_INT(AST_TYPE_PRIMITIVE, inner_arg->type->kind, "innermost is primitive");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_named_type_in_parameter(void) {
    const char *source =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<string> x = args;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstStatement *stmt;
    const AstStartDecl *start_decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse named type in local");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error");

    start_decl = &program.top_level_decls[1]->as.start_decl;
    stmt = start_decl->body.as.block->statements[0];
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, stmt->kind, "local binding stmt");
    ASSERT_EQ_INT(AST_TYPE_NAMED, stmt->as.local_binding.declared_type.kind,
                  "local has named type");
    ASSERT_EQ_STR("Option", stmt->as.local_binding.declared_type.name,
                  "local named type is Option");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_asm_decl(void) {
    const char *source =
        "int32 my_add = asm(int32 a, int32 b) -> {\n"
        "    mov eax, edi\n"
        "    add eax, esi\n"
        "    ret\n"
        "};\n"
        "start(string[] args) -> my_add(1, 2);\n";
    Parser parser;
    AstProgram program;
    const AstTopLevelDecl *decl;
    const AstAsmDecl *asm_decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse asm decl");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for asm decl");
    ASSERT_EQ_INT(2, (int)program.top_level_count, "two top-level decls (asm + start)");

    decl = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_ASM, (int)decl->kind, "first decl is asm");
    asm_decl = &decl->as.asm_decl;
    ASSERT_EQ_STR("my_add", asm_decl->name, "asm decl name is my_add");
    ASSERT_EQ_INT(AST_TYPE_PRIMITIVE, (int)asm_decl->return_type.kind, "asm return type is primitive");
    ASSERT_EQ_INT(2, (int)asm_decl->parameters.count, "asm has 2 parameters");
    ASSERT_EQ_STR("a", asm_decl->parameters.items[0].name, "asm param 1 name");
    ASSERT_EQ_STR("b", asm_decl->parameters.items[1].name, "asm param 2 name");
    ASSERT_TRUE(asm_decl->body != NULL, "asm body is not null");
    ASSERT_TRUE(asm_decl->body_length > 0, "asm body has non-zero length");
    ASSERT_TRUE(strstr(asm_decl->body, "mov eax, edi") != NULL, "asm body contains mov instruction");
    ASSERT_TRUE(strstr(asm_decl->body, "add eax, esi") != NULL, "asm body contains add instruction");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_asm_decl_with_modifiers(void) {
    const char *source =
        "export int32 my_fn = asm() -> {\n"
        "    xor eax, eax\n"
        "    ret\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstTopLevelDecl *decl;
    const AstAsmDecl *asm_decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse asm decl with modifiers");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for asm decl with modifiers");
    ASSERT_EQ_INT(1, (int)program.top_level_count, "one top-level decl");

    decl = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_ASM, (int)decl->kind, "decl is asm");
    asm_decl = &decl->as.asm_decl;
    ASSERT_EQ_STR("my_fn", asm_decl->name, "asm decl name is my_fn");
    ASSERT_EQ_INT(1, (int)asm_decl->modifier_count, "asm has 1 modifier");
    ASSERT_EQ_INT(AST_MODIFIER_EXPORT, (int)asm_decl->modifiers[0], "modifier is export");
    ASSERT_EQ_INT(0, (int)asm_decl->parameters.count, "asm has 0 parameters");

    ast_program_free(&program);
    parser_free(&parser);
}

/* ------------------------------------------------------------------ */
/*  Test: boot declaration                                            */
/* ------------------------------------------------------------------ */

static void test_parse_boot_decl(void) {
    const char *source =
        "boot() -> {\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    AstTopLevelDecl *decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse boot program");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for boot decl");
    ASSERT_EQ_INT(1, (int)program.top_level_count, "one top-level decl");

    decl = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_START, (int)decl->kind, "boot decl uses AST_TOP_LEVEL_START");
    ASSERT_TRUE(decl->as.start_decl.is_boot, "is_boot flag is set");
    ASSERT_EQ_INT(0, (int)decl->as.start_decl.parameters.count, "boot has zero parameters");
    ASSERT_EQ_INT(AST_LAMBDA_BODY_BLOCK, (int)decl->as.start_decl.body.kind, "boot body is block");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_boot_decl_expression_body(void) {
    const char *source = "boot() -> 42;\n";
    Parser parser;
    AstProgram program;
    AstTopLevelDecl *decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse boot expression body");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error");

    decl = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_START, (int)decl->kind, "boot decl kind");
    ASSERT_TRUE(decl->as.start_decl.is_boot, "is_boot set for expression body");
    ASSERT_EQ_INT(AST_LAMBDA_BODY_EXPRESSION, (int)decl->as.start_decl.body.kind,
                  "boot body is expression");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_manual_statement_with_memory_ops(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 ptr = malloc(1024);\n"
        "        ptr = realloc(ptr, 2048);\n"
        "        free(ptr);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstStartDecl *start_decl;
    const AstStatement *manual_stmt;
    const AstBlock *body;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse manual statement");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for manual");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    manual_stmt = start_decl->body.as.block->statements[0];
    ASSERT_EQ_INT(AST_STMT_MANUAL, manual_stmt->kind, "first stmt is manual");

    body = manual_stmt->as.manual.body;
    REQUIRE_TRUE(body != NULL, "manual body is not null");
    ASSERT_EQ_INT(3, (int)body->statement_count, "manual body has 3 statements");

    /* First statement: int64 ptr = malloc(1024) */
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, body->statements[0]->kind,
                  "manual body stmt 0 is local binding");
    ASSERT_EQ_INT(AST_EXPR_MEMORY_OP,
                  body->statements[0]->as.local_binding.initializer->kind,
                  "malloc is a memory op expression");
    ASSERT_EQ_INT(AST_MEMORY_MALLOC,
                  body->statements[0]->as.local_binding.initializer->as.memory_op.kind,
                  "memory op kind is malloc");

    /* Third statement: free(ptr) */
    ASSERT_EQ_INT(AST_STMT_EXPRESSION, body->statements[2]->kind,
                  "manual body stmt 2 is expression");
    ASSERT_EQ_INT(AST_EXPR_MEMORY_OP,
                  body->statements[2]->as.expression->kind,
                  "free is a memory op expression");
    ASSERT_EQ_INT(AST_MEMORY_FREE,
                  body->statements[2]->as.expression->as.memory_op.kind,
                  "memory op kind is free");

    ast_program_free(&program);
    parser_free(&parser);
}

static void test_parse_calloc_memory_op(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 ptr = calloc(10, 8);\n"
        "        free(ptr);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstStartDecl *start_decl;
    const AstStatement *manual_stmt;
    const AstBlock *body;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse calloc manual");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for calloc");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    manual_stmt = start_decl->body.as.block->statements[0];
    body = manual_stmt->as.manual.body;
    REQUIRE_TRUE(body != NULL, "calloc manual body is not null");

    ASSERT_EQ_INT(AST_EXPR_MEMORY_OP,
                  body->statements[0]->as.local_binding.initializer->kind,
                  "calloc is a memory op expression");
    ASSERT_EQ_INT(AST_MEMORY_CALLOC,
                  body->statements[0]->as.local_binding.initializer->as.memory_op.kind,
                  "memory op kind is calloc");

    ast_program_free(&program);
    parser_free(&parser);
}

int main(void) {
    printf("Running parser tests...\n\n");

    RUN_TEST(test_parse_program_structure);
    RUN_TEST(test_parse_expression_precedence);
    RUN_TEST(test_parse_assignment_and_ternary);
    RUN_TEST(test_parse_template_literal);
    RUN_TEST(test_parse_errors);
    RUN_TEST(test_parse_prefix_increment_decrement);
    RUN_TEST(test_parse_postfix_increment_decrement);
    RUN_TEST(test_parse_discard_expression);
    RUN_TEST(test_parse_varargs_parameter);
    RUN_TEST(test_parse_v2_modifiers);
    RUN_TEST(test_parse_import_alias);
    RUN_TEST(test_parse_import_wildcard);
    RUN_TEST(test_parse_import_selective);
    RUN_TEST(test_parse_java_primitive_aliases);
    RUN_TEST(test_parse_union_declaration);
    RUN_TEST(test_parse_union_with_modifiers);
    RUN_TEST(test_parse_internal_local_binding);
    RUN_TEST(test_parse_named_type_in_binding);
    RUN_TEST(test_parse_arr_wildcard_type);
    RUN_TEST(test_parse_nested_generic_rshift);
    RUN_TEST(test_parse_named_type_in_parameter);
    RUN_TEST(test_parse_asm_decl);
    RUN_TEST(test_parse_asm_decl_with_modifiers);
    RUN_TEST(test_parse_boot_decl);
    RUN_TEST(test_parse_boot_decl_expression_body);
    RUN_TEST(test_parse_manual_statement_with_memory_ops);
    RUN_TEST(test_parse_calloc_memory_op);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}