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


void test_type_checker_requires_start_entry_point(void) {
    const char *source = "int32 value = 1;\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse missing start program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for missing start");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "missing start fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "missing start error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format missing start error");
    ASSERT_EQ_STR("Program must declare exactly one start or boot entry point.",
                  diagnostic,
                  "formatted missing start diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_duplicate_start_entry_points(void) {
    const char *source =
        "start(string[] args) -> 0;\n"
        "start(string[] args) -> 1;\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse duplicate start program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for duplicate start");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "duplicate start fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "duplicate start error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format duplicate start error");
    ASSERT_EQ_STR("2:1: Program cannot declare multiple start entry points. Related location at 1:1.",
                  diagnostic,
                  "formatted duplicate start diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_invalid_start_parameter_type(void) {
    const char *source = "start(int32 args) -> 0;\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse invalid start parameter program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for invalid start parameter");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "invalid start parameter fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "invalid start parameter error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format invalid start parameter error");
    ASSERT_EQ_STR("1:13: start parameter must have type string[]. Related location at 1:1.",
                  diagnostic,
                  "formatted invalid start parameter diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_boot_entry_point(void) {
    const char *source = "boot() -> 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse boot program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for boot program");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "boot entry point passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_boot_and_start_together(void) {
    const char *source =
        "boot() -> 0;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse boot+start program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for boot+start");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "boot+start fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "boot+start error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format boot+start error");
    ASSERT_TRUE(strstr(diagnostic, "boot and start") != NULL ||
                strstr(diagnostic, "mutually exclusive") != NULL ||
                strstr(diagnostic, "cannot declare") != NULL,
                "boot+start error contains relevant message");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

