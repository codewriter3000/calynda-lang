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


/* ------------------------------------------------------------------ */
/* V2 feature: discard expression _                                   */
/* ------------------------------------------------------------------ */
void test_parse_discard_expression(void) {
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
void test_parse_varargs_parameter(void) {
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

void test_parse_default_parameter(void) {
    const char *source = "int32 add = (int32 left, int32 right = left + 1) -> left + right;\n";
    Parser parser;
    AstProgram program;
    const AstParameter *right;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse default parameter binding");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for default parameter");
    ASSERT_EQ_INT(2,
                  program.top_level_decls[0]->as.binding_decl.initializer
                      ->as.lambda.parameters.count,
                  "default parameter preserves arity");
    right = &program.top_level_decls[0]->as.binding_decl.initializer
                 ->as.lambda.parameters.items[1];
    ASSERT_EQ_STR("right", right->name, "default parameter name");
    ASSERT_TRUE(right->default_expr != NULL, "default parameter expression is present");
    ASSERT_EQ_INT(AST_EXPR_BINARY, right->default_expr->kind,
                  "default parameter parses full expression");

    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2 feature: export / static / internal modifiers                   */
/* ------------------------------------------------------------------ */
void test_parse_v2_modifiers(void) {
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

void test_parse_thread_local_binding_modifier(void) {
    const char *source = "thread_local int32 counter = 5;\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse thread_local binding");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for thread_local");
    ASSERT_EQ_INT(1, program.top_level_count, "one top-level decl");
    ASSERT_EQ_INT(1, program.top_level_decls[0]->as.binding_decl.modifier_count,
                  "thread_local decl modifier count");
    ASSERT_EQ_INT(AST_MODIFIER_THREAD_LOCAL,
                  program.top_level_decls[0]->as.binding_decl.modifiers[0],
                  "thread_local modifier");

    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2 feature: import alias, wildcard, selective                      */
/* ------------------------------------------------------------------ */
void test_parse_import_alias(void) {
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


void test_parse_import_wildcard(void) {
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


void test_parse_import_selective(void) {
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
void test_parse_java_primitive_aliases(void) {
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
