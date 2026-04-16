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
#define ASSERT_CONTAINS(needle, haystack, msg) do {                         \
    tests_run++;                                                            \
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {      \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\" in \"%s\"\n", \
                __FILE__, __LINE__, (msg), (needle),                        \
                (haystack) ? (haystack) : "(null)");                       \
    }                                                                       \
} while (0)


/* ------------------------------------------------------------------ */
/*  G-TC-8: Error diagnostic includes line:column span                */
/* ------------------------------------------------------------------ */

void test_type_checker_error_has_source_span(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32 x = true;\n"
        "    return x;\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *error;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse for span test");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for span test");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "type mismatch is rejected");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "error object is non-null");
    ASSERT_TRUE(error->primary_span.start_line > 0,
                "error primary span has valid start_line");
    ASSERT_TRUE(error->primary_span.start_column > 0,
                "error primary span has valid start_column");

    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic)),
                 "format type mismatch error");
    ASSERT_CONTAINS("2:", diagnostic,
                    "formatted error includes line number");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/*  G-TC-2: Pre-increment type preservation                           */
/* ------------------------------------------------------------------ */

void test_type_checker_accepts_pre_increment(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    var x = 5;\n"
        "    int32 y = ++x;\n"
        "    return y;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse pre-increment");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for pre-increment");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "pre-increment on int32 is accepted and yields int32");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_threading_builtins_and_type_alias(void) {
    static const char source[] =
        "type ExitCode = int32;\n"
        "start(string[] args) -> {\n"
        "    Thread t = spawn () -> {\n"
        "        exit;\n"
        "    };\n"
        "    Mutex m = Mutex.new();\n"
        "    t.join();\n"
        "    m.lock();\n"
        "    m.unlock();\n"
        "    ExitCode code = 0;\n"
        "    return code;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse threading type checker source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols threading type checker");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "threading builtins and aliases type check");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_spawn_non_void_callable(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    Thread t = spawn () -> 1;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bad spawn source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols bad spawn");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "spawn rejects non-void callable");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
