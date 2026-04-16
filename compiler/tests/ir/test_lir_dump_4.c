#include "hir.h"
#include "lir.h"
#include "mir.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
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

void test_lir_dump_lowers_union_tag_and_payload_access(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> value = Option.Some(7);\n"
        "    int32 tag = value.tag;\n"
        "    int32 payload_value = int32(value.payload);\n"
        "    return tag + payload_value;\n"
        "};\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse union access LIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for union access LIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check union access LIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for union access LIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "lower MIR for union access LIR program");
    REQUIRE_TRUE(lir_build_program(&lir_program, &mir_program),
                 "lower LIR for union tag/payload access");

    dump = lir_dump_program_to_string(&lir_program);
    REQUIRE_TRUE(dump != NULL, "render union access LIR dump to string");
    ASSERT_TRUE(strstr(dump, "union_get_tag slot(1:value)") != NULL,
                "union .tag lowers to a dedicated LIR instruction");
    ASSERT_TRUE(strstr(dump, "union_get_payload slot(1:value)") != NULL,
                "union .payload lowers to a dedicated LIR instruction");
    ASSERT_TRUE(strstr(dump, "member slot(1:value).tag") == NULL &&
                strstr(dump, "member slot(1:value).payload") == NULL,
                "union metadata access no longer falls back to generic LIR members");

    free(dump);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}


void test_lir_error_api_returns_null_on_success(void) {
    static const char source[] =
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    const LirBuildError *error;
    char buffer[256];

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse LIR error API program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for LIR error API");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check LIR error API program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for LIR error API program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "lower MIR for LIR error API program");
    REQUIRE_TRUE(lir_build_program(&lir_program, &mir_program),
                 "LIR lowering succeeds for valid program");

    ASSERT_TRUE(!lir_program.has_error, "successful LIR build has no error flag");
    error = lir_get_error(&lir_program);
    ASSERT_TRUE(error == NULL, "lir_get_error returns NULL on success");
    ASSERT_TRUE(!lir_format_error(NULL, buffer, sizeof(buffer)),
                "lir_format_error returns false for NULL error");

    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

void test_lir_builder_preserves_upstream_error_spans(void) {
    static const char source[] = "start(string[] args) -> missing + 1;\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    char expected[256];
    char actual[256];

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse invalid LIR input program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for invalid LIR input");
    ASSERT_TRUE(!type_checker_check_program(&checker, &ast_program, &symbols),
                "type check fails before LIR precondition setup");
    ASSERT_TRUE(!hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                "HIR lowering rejects invalid LIR input");
    REQUIRE_TRUE(hir_format_error(hir_get_error(&hir_program), expected, sizeof(expected)),
                 "format propagated HIR error for LIR precondition");
    mir_program.has_error = true;
    mir_program.error.primary_span = hir_get_error(&hir_program)->primary_span;
    mir_program.error.related_span = hir_get_error(&hir_program)->related_span;
    mir_program.error.has_related_span = hir_get_error(&hir_program)->has_related_span;
    strncpy(mir_program.error.message,
            hir_get_error(&hir_program)->message,
            sizeof(mir_program.error.message) - 1);
    mir_program.error.message[sizeof(mir_program.error.message) - 1] = '\0';

    ASSERT_TRUE(!lir_build_program(&lir_program, &mir_program),
                "LIR lowering rejects MIR programs with structured errors");
    REQUIRE_TRUE(lir_format_error(lir_get_error(&lir_program), actual, sizeof(actual)),
                 "format propagated LIR build error");
    ASSERT_TRUE(strcmp(expected, actual) == 0,
                "LIR build error preserves upstream diagnostic text");
    ASSERT_TRUE(lir_get_error(&lir_program)->primary_span.start_line ==
                    hir_get_error(&hir_program)->primary_span.start_line &&
                lir_get_error(&lir_program)->primary_span.start_column ==
                    hir_get_error(&hir_program)->primary_span.start_column,
                "LIR build error preserves the primary diagnostic span");

    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}
