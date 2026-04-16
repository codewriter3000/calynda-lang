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
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\" in \"%s\"\n", \
                __FILE__, __LINE__, (msg), (needle),                        \
                (haystack) ? (haystack) : "(null)");                       \
    }                                                                       \
} while (0)

void test_type_checker_accepts_union_tag_and_payload_access(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> value = Option.Some(7);\n"
        "    int32 tag = value.tag;\n"
        "    int32 payload = int32(value.payload);\n"
        "    return tag + payload;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse union tag/payload program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for union tag/payload program");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "union values allow .tag and .payload access");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_unknown_union_value_member(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> value = Option.Some(7);\n"
        "    return value.variant;\n"
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
                 "parse unknown union value member program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for unknown union value member");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "unknown union value members are rejected");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format unknown union member diagnostic");
    ASSERT_CONTAINS("support only '.tag' and '.payload'",
                    diagnostic,
                    "union member diagnostics describe the supported accessors");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_union_tag_assignment(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> value = Option.Some(7);\n"
        "    value.tag = 1;\n"
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
                 "parse union tag assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for union tag assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "union tag access stays read-only");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format union tag assignment diagnostic");
    ASSERT_CONTAINS("requires an assignable target",
                    diagnostic,
                    "union tag writes are rejected as non-assignable metadata access");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}