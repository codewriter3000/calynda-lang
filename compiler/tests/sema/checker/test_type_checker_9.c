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


void test_type_checker_resolves_union_symbol(void) {
    const char *source =
        "union Direction { North, South, East, West };\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const Symbol *sym;
    const TypeCheckInfo *info;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse union program");

    symbol_table_init(&symbols);
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols");

    type_checker_init(&checker);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "union passes type checking");

    sym = scope_lookup_local(symbol_table_root_scope(&symbols), "Direction");
    REQUIRE_TRUE(sym != NULL, "Direction symbol exists");
    info = type_checker_get_symbol_info(&checker, sym);
    REQUIRE_TRUE(info != NULL, "Direction has type info");
    ASSERT_EQ_INT(CHECKED_TYPE_NAMED, info->type.kind, "Direction type is named");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_resolves_named_type_local(void) {
    const char *source =
        "union Direction { North, South, East, West };\n"
        "start(string[] args) -> {\n"
        "    Direction d = Direction;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse named local program");

    symbol_table_init(&symbols);
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols named local");

    type_checker_init(&checker);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "named type local passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_non_payload_variant(void) {
    const char *source =
        "union Direction { North, South, East, West };\n"
        "start(string[] args) -> {\n"
        "    Direction d = Direction.North;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse variant program");

    symbol_table_init(&symbols);
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols variant");

    type_checker_init(&checker);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "non-payload variant passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_payload_variant_constructor(void) {
    const char *source =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> x = Option.Some(42);\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse payload variant");

    symbol_table_init(&symbols);
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols payload variant");

    type_checker_init(&checker);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "payload variant constructor passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_unknown_variant(void) {
    const char *source =
        "union Direction { North, South, East, West };\n"
        "start(string[] args) -> {\n"
        "    Direction d = Direction.Up;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse unknown variant");

    symbol_table_init(&symbols);
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols unknown variant");

    type_checker_init(&checker);
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "unknown variant fails type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_hetero_array(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true, \"hello\"];\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse hetero array");

    symbol_table_init(&symbols);
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols hetero array");

    type_checker_init(&checker);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "hetero array passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

