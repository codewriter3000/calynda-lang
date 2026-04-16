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
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {      \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n", __FILE__, __LINE__, (msg)); \
    }                                                                       \
} while (0)

void test_type_checker_rejects_hetero_array_external_equality_without_cast(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true];\n"
        "    bool same = mixed[0] == 1;\n"
        "    return 0;\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse hetero array external equality program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for hetero array external equality");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "hetero array external values require explicit casts before equality");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format hetero array equality diagnostic");
    ASSERT_CONTAINS("requires explicit casts",
                    diagnostic,
                    "hetero array equality diagnostics require explicit casts");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_hetero_array_equality_after_explicit_casts(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true];\n"
        "    bool same = int32(mixed[0]) == 1 && bool(mixed[1]) == true;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse hetero array cast equality program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for hetero array cast equality");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "hetero array values can be compared after explicit casts");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_arr_wildcard_from_multi_dimensional_array(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[][] grid = [[1], [2]];\n"
        "    arr<?> mixed = grid;\n"
        "    return 0;\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse arr wildcard from multidimensional array program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for arr wildcard from multidimensional array");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "arr<?> rejects multidimensional array assignment");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format arr wildcard multidimensional diagnostic");
    ASSERT_CONTAINS("Cannot assign expression of type int32[][]",
                    diagnostic,
                    "arr<?> diagnostics report the rejected multidimensional source type");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}