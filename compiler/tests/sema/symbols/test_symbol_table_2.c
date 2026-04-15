#include "parser.h"
#include "symbol_table.h"

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


void test_symbol_table_source_spans_and_unresolved_diagnostic(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    return externalName;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const AstStartDecl *start_decl;
    const AstParameter *args_parameter;
    const AstBlock *start_block;
    const AstExpression *return_identifier;
    const UnresolvedIdentifier *unresolved;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&table);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse program with unresolved identifier span checks");
    REQUIRE_TRUE(symbol_table_build(&table, &program),
                 "build symbol table for unresolved identifier span checks");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    args_parameter = &start_decl->parameters.items[0];
    start_block = start_decl->body.as.block;
    return_identifier = start_block->statements[0]->as.return_expression;
    unresolved = symbol_table_get_unresolved_identifier(&table, 0);

    ASSERT_EQ_INT(1, start_decl->start_span.start_line, "start keyword line");
    ASSERT_EQ_INT(1, start_decl->start_span.start_column, "start keyword column");
    ASSERT_EQ_INT(1, args_parameter->name_span.start_line, "parameter name line");
    ASSERT_EQ_INT(16, args_parameter->name_span.start_column, "parameter name column");
    ASSERT_EQ_INT(2, return_identifier->source_span.start_line, "identifier line");
    ASSERT_EQ_INT(12, return_identifier->source_span.start_column, "identifier column");

    REQUIRE_TRUE(unresolved != NULL, "unresolved identifier record exists");
    ASSERT_EQ_INT(2, unresolved->source_span.start_line, "unresolved identifier line");
    ASSERT_EQ_INT(12, unresolved->source_span.start_column, "unresolved identifier column");
    REQUIRE_TRUE(symbol_table_format_unresolved_identifier(unresolved, diagnostic,
                                                           sizeof(diagnostic_buffer)),
                 "format unresolved identifier diagnostic");
    ASSERT_EQ_STR("2:12: Unresolved identifier 'externalName'.",
                  diagnostic,
                  "formatted unresolved identifier diagnostic");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

