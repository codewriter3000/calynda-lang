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

void test_type_checker_rejects_sized_array_literal_length_mismatch(void) {
    const char *source =
        "boot -> {\n"
        "    uint32[2] values = [uint32(1), uint32(2), uint32(3)];\n"
        "    return int32(values[0]);\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse sized array mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for sized array mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "oversized literal for sized array fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "sized array mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format sized array mismatch error");
    ASSERT_CONTAINS("Array literal assigned to local 'values' has 3 elements, but target type uint32[2] requires 2.",
                    diagnostic,
                    "formatted sized array mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_matching_sized_multidimensional_literal(void) {
    const char *source =
        "boot -> {\n"
        "    int32[2][2] matrix = [[1, 2], [3, 4]];\n"
        "    return matrix[0][1];\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse matching sized matrix program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for matching sized matrix");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "matching multidimensional literal passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_sized_array_slice_assignment_mismatch(void) {
    const char *source =
        "boot -> {\n"
        "    int32[2][2] matrix = [[1, 2], [3, 4]];\n"
        "    matrix[0] = [5, 6, 7];\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse sized slice mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for sized slice mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "oversized slice assignment fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "sized slice mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format sized slice mismatch error");
    ASSERT_CONTAINS("Array literal assigned to local 'matrix' has 3 elements, but target type int32[2] requires 2.",
                    diagnostic,
                    "formatted sized slice mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_sized_array_call_argument_mismatch(void) {
    const char *source =
        "boot -> {\n"
        "    var use = (uint32[3] values) -> int32(values[0]);\n"
        "    uint32[2] left = [uint32(1), uint32(2)];\n"
        "    return use(left);\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse sized call mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for sized call mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "mismatched sized array call argument fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "sized call mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format sized call mismatch error");
    ASSERT_CONTAINS("Cannot pass array of declared type uint32[2] to parameter 'values' of type uint32[3].",
                    diagnostic,
                    "formatted sized call mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_sized_array_default_value_mismatch(void) {
    const char *source =
        "boot -> {\n"
        "    var use = (int32[3] values = [1, 2]) -> values[0];\n"
        "    return use();\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse sized default mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for sized default mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "mismatched sized array default value fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "sized default mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format sized default mismatch error");
    ASSERT_CONTAINS("Array literal default value for parameter 'values' has 2 elements, but target type int32[3] requires 3.",
                    diagnostic,
                    "formatted sized default mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_sized_array_lambda_body_mismatch(void) {
    const char *source =
        "uint32[3] bad = () -> [uint32(1), uint32(2)];\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse sized lambda body mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for sized lambda body mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "mismatched sized array lambda body fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "sized lambda body mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format sized lambda body mismatch error");
    ASSERT_CONTAINS("Array literal returned from lambda body has 2 elements, but target type uint32[3] requires 3.",
                    diagnostic,
                    "formatted sized lambda body mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_sized_array_return_statement_mismatch(void) {
    const char *source =
        "uint32[3] bad = () -> {\n"
        "    uint32[2] values = [uint32(1), uint32(2)];\n"
        "    return values;\n"
        "};\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse sized return statement mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for sized return statement mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "mismatched sized array return statement fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "sized return statement mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format sized return statement mismatch error");
    ASSERT_CONTAINS("Cannot return array of declared type uint32[2] from return statement in lambda body expecting uint32[3].",
                    diagnostic,
                    "formatted sized return statement mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_sized_array_call_result_mismatch(void) {
    const char *source =
        "boot -> {\n"
        "    var id = (uint32[2] values) -> values;\n"
        "    uint32[2] left = [uint32(1), uint32(2)];\n"
        "    uint32[3] right = id(left);\n"
        "    return int32(right[0]);\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse sized call result mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for sized call result mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "mismatched sized array call result fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "sized call result mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format sized call result mismatch error");
    ASSERT_CONTAINS("Cannot assign array of declared type uint32[2] to local 'right' of type uint32[3].",
                    diagnostic,
                    "formatted sized call result mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_sized_array_block_call_result_mismatch(void) {
    const char *source =
        "boot -> {\n"
        "    var id = () -> {\n"
        "        uint32[2] values = [uint32(1), uint32(2)];\n"
        "        return values;\n"
        "    };\n"
        "    uint32[3] right = id();\n"
        "    return int32(right[0]);\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse sized block call result mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for sized block call result mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "mismatched sized array block call result fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "sized block call result mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format sized block call result mismatch error");
    ASSERT_CONTAINS("Cannot assign array of declared type uint32[2] to local 'right' of type uint32[3].",
                    diagnostic,
                    "formatted sized block call result mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_sized_array_ternary_result_mismatch(void) {
    const char *source =
        "boot -> {\n"
        "    uint32[2] left = [uint32(1), uint32(2)];\n"
        "    uint32[2] right = [uint32(3), uint32(4)];\n"
        "    uint32[3] out = true ? left : right;\n"
        "    return int32(out[0]);\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse sized ternary result mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for sized ternary result mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "mismatched sized array ternary result fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "sized ternary result mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format sized ternary result mismatch error");
    ASSERT_CONTAINS("Cannot assign array of declared type uint32[2] to local 'out' of type uint32[3].",
                    diagnostic,
                    "formatted sized ternary result mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_sized_array_ternary_branch_mismatch(void) {
    const char *source =
        "boot -> {\n"
        "    uint32[2] left = [uint32(1), uint32(2)];\n"
        "    uint32[3] right = [uint32(3), uint32(4), uint32(5)];\n"
        "    var pick = true ? left : right;\n"
        "    return int32(pick[0]);\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse sized ternary branch mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for sized ternary branch mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "mismatched sized array ternary branches fail type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "sized ternary branch mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format sized ternary branch mismatch error");
    ASSERT_CONTAINS("Ternary branches must have compatible declared array types, but got uint32[2] and uint32[3].",
                    diagnostic,
                    "formatted sized ternary branch mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

#include "test_type_checker_6_p2.inc"
