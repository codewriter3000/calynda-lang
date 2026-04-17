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


void test_type_checker_rejects_assignment_to_temporary_index_target(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    [1, 2, 3][0] = 4;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse temporary index assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for temporary index assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "temporary index assignment fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "temporary index assignment error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format temporary index assignment error");
    ASSERT_EQ_STR("2:5: Operator '=' requires an assignable target.",
                  diagnostic,
                  "formatted temporary index assignment diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_assignment_to_import_symbol(void) {
    const char *source =
        "import io.stdlib;\n"
        "start(string[] args) -> {\n"
        "    stdlib = 1;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse import assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for import assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "assignment to import symbol fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "import assignment error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format import assignment error");
    ASSERT_EQ_STR("3:5: Operator '=' requires an assignable target. Related location at 1:11.",
                  diagnostic,
                  "formatted import assignment diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_assignment_to_final_index_target(void) {
    const char *source =
        "final int32[] values = [1, 2, 3];\n"
        "start(string[] args) -> {\n"
        "    values[0] = 4;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse final index assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for final index assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "assignment through final root fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "final index assignment error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format final index assignment error");
    ASSERT_EQ_STR("3:5: Cannot assign to final symbol 'values'. Related location at 1:15.",
                  diagnostic,
                  "formatted final index assignment diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_incompatible_compound_assignment(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    bool ready = true;\n"
        "    ready += 1;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bad compound assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for bad compound assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "incompatible compound assignment fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "bad compound assignment error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format bad compound assignment error");
    ASSERT_EQ_STR("3:5: Operator '+' cannot be applied to types bool and int32.",
                  diagnostic,
                  "formatted bad compound assignment diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_swap_between_array_elements(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    values[0] >< values[2];\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse swap program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for swap program");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "swap between array elements passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_swap_with_mismatched_types(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    int32 left = 1;\n"
        "    string right = \"hi\";\n"
        "    left >< right;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse mismatched swap program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for mismatched swap");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "mismatched swap fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "mismatched swap error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format mismatched swap error");
    ASSERT_EQ_STR("4:13: Swap statement requires matching target types but got int32 and string. Related location at 4:5.",
                  diagnostic,
                  "formatted mismatched swap diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
