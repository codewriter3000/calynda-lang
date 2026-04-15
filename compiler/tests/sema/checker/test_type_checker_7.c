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


/* ------------------------------------------------------------------ */
/* V2: postfix ++/-- numeric validation                                */
/* ------------------------------------------------------------------ */

void test_type_checker_rejects_postfix_increment_on_non_numeric(void) {
    const char *source =
        "string s = () -> \"hi\";\n"
        "start(string[] args) -> {\n"
        "    s++;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse postfix inc non-numeric");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols postfix inc");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "postfix ++ on string fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "postfix inc error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format postfix inc error");
    ASSERT_CONTAINS("Postfix operator requires a numeric operand",
                    diagnostic,
                    "postfix inc diagnostic mentions numeric operand");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2: discard _ as assignment target                                  */
/* ------------------------------------------------------------------ */

void test_type_checker_allows_discard_assignment(void) {
    const char *source =
        "int32 compute = () -> 42;\n"
        "start(string[] args) -> {\n"
        "    _ = compute();\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse discard assignment");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols discard");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "discard assignment passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2: varargs call argument count validation                          */
/* ------------------------------------------------------------------ */

void test_type_checker_allows_varargs_call(void) {
    const char *source =
        "int32 sum = (int32... nums) -> 0;\n"
        "start(string[] args) -> {\n"
        "    sum(1, 2, 3);\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse varargs call");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols varargs");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "varargs call with multiple args passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_varargs_type_mismatch(void) {
    const char *source =
        "int32 sum = (int32... nums) -> 0;\n"
        "start(string[] args) -> {\n"
        "    sum(1, \"bad\", 3);\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse varargs type mismatch");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols varargs mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "varargs call with wrong type fails");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "varargs type mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format varargs mismatch error");
    ASSERT_CONTAINS("expects int32 but got string",
                    diagnostic,
                    "varargs mismatch diagnostic mentions type");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2: Java-style primitive aliases type check                         */
/* ------------------------------------------------------------------ */

void test_type_checker_handles_java_primitive_aliases(void) {
    const char *source =
        "int x = () -> 42;\n"
        "double pi = () -> 3.14;\n"
        "start(string[] args) -> {\n"
        "    var result = x() + int(pi());\n"
        "    return result;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse java alias program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols java alias");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "java primitive aliases pass type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

