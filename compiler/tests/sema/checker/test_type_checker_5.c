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


void test_type_checker_rejects_duplicate_boot_entry_points(void) {
    const char *source =
        "boot() -> 0;\n"
        "boot() -> 1;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *error;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse duplicate boot program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for duplicate boot");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "duplicate boot fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "duplicate boot error exists");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_allows_exit_in_void_lambda_block(void) {
    const char *source =
        "void disposer = () -> { exit; };\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse void lambda exit program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for void lambda exit");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "void lambda exit passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_bare_return_in_non_void_lambda(void) {
    const char *source =
        "int32 bad = () -> { return; };\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bare return lambda program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for bare return lambda");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "bare return in non-void lambda fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "bare return lambda error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format bare return lambda error");
    ASSERT_EQ_STR("1:21: Return statement in lambda body must produce int32. Related location at 1:7.",
                  diagnostic,
                  "formatted bare return lambda diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_exit_in_start_block(void) {
    const char *source = "start(string[] args) -> { exit; };\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse start exit program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for start exit");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "exit in start fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "start exit error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format start exit error");
    ASSERT_EQ_STR("1:27: exit is only permitted in void-typed lambda blocks. Related location at 1:1.",
                  diagnostic,
                  "formatted start exit diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_bare_return_in_start_block(void) {
    const char *source = "start(string[] args) -> { return; };\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bare start return program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for bare start return");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "bare return in start fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "bare start return error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format bare start return error");
    ASSERT_EQ_STR("1:27: Return statement in start must produce int32. Related location at 1:1.",
                  diagnostic,
                  "formatted bare start return diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_allows_assignment_to_local_array_element(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    var values = [1, 2, 3];\n"
        "    values[0] = 4;\n"
        "    return values[0];\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse local array assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for local array assignment");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "local array element assignment passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

