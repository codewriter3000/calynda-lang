#include "parser.h"
#include "symbol_table.h"
#include "type_checker.h"

#include <stdio.h>
#include <string.h>

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

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

void test_type_checker_accepts_recursive_top_level_lambda_binding(void) {
    static const char source[] =
        "int32 sum_down = (int32 value) -> value <= 0 ? 0 : value + sum_down(value - 1);\n"
        "start(string[] args) -> {\n"
        "    return sum_down(4);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse recursive top-level lambda binding program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for recursive top-level lambda binding");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "recursive top-level lambda binding passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_recursive_top_level_manual_lambda_binding(void) {
    static const char source[] =
        "int32 factorial = manual(int32 value) -> {\n"
        "    return value <= 1 ? 1 : value * factorial(value - 1);\n"
        "};\n"
        "start(string[] args) -> {\n"
        "    return factorial(5);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse recursive top-level manual lambda binding program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for recursive top-level manual lambda binding");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "recursive top-level manual lambda binding passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_recursive_non_lambda_top_level_binding(void) {
    static const char source[] =
        "int32 countdown = countdown + 1;\n"
        "start(string[] args) -> {\n"
        "    return countdown;\n"
        "};\n";
    char diagnostic[256];
    const char *captured_diagnostic = NULL;
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse recursive non-lambda top-level binding program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for recursive non-lambda top-level binding");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "recursive non-lambda top-level binding is rejected");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format recursive non-lambda top-level binding diagnostic");
    captured_diagnostic = diagnostic;
    ASSERT_CONTAINS("Circular definition involving 'countdown'.",
                    captured_diagnostic,
                    "recursive non-lambda top-level binding reports circular definition");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
