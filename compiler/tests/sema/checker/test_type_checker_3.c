#include "parser.h"
#include "symbol_table.h"
#include "type_checker.h"

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


void test_type_checker_rejects_non_bool_ternary_condition(void) {
    const char *source = "start(string[] args) -> 1 ? 2 : 3;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *error;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse ternary condition program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for ternary condition");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "non-bool ternary condition fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "ternary condition error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format ternary condition error");
    ASSERT_EQ_STR("1:25: Ternary condition must have type bool but got int32.",
                  diagnostic,
                  "formatted ternary condition diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_void_lambda_body_for_typed_binding(void) {
    const char *source =
        "int32 bad = (int32 value) -> { value + 1; };\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *error;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse void lambda body program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for void lambda body");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "void lambda body fails typed binding check");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "void lambda body error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format void lambda body error");
    ASSERT_EQ_STR("1:13: Lambda body must produce int32 but got void. Related location at 1:7.",
                  diagnostic,
                  "formatted void lambda body diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_bad_start_return_type(void) {
    const char *source = "start(string[] args) -> true;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *error;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bad start program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for bad start");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "bad start return type fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "bad start error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format bad start error");
    ASSERT_EQ_STR("1:25: start body must produce int32 but got bool. Related location at 1:1.",
                  diagnostic,
                  "formatted bad start diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_void_zero_arg_callable_in_template(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    var message = `${() -> { exit; }}`;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *error;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse void template callable program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for void template callable");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "void zero-arg callable in template fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "void template callable error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format void template callable error");
    ASSERT_EQ_STR(
        "2:22: Template interpolation cannot auto-call a zero-argument callable returning void.",
        diagnostic,
        "formatted void template callable diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_reports_type_resolution_errors(void) {
    const char *source = "start(void args) -> 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *error;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse type resolution error program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for type resolution error");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "type resolution error fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "type resolution error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format type resolution error");
    ASSERT_EQ_STR("1:12: Parameter 'args' cannot have type void.",
                  diagnostic,
                  "formatted type resolution diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

