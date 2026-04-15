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


void test_symbol_table_records_unresolved_identifier(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    return externalName;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const AstBlock *start_block;
    const AstExpression *return_identifier;

    symbol_table_init(&table);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse unresolved identifier program");
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbol table with unresolved identifier");
    ASSERT_TRUE(symbol_table_get_error(&table) == NULL, "unresolved identifier is not a build error");
    ASSERT_EQ_INT(1, table.unresolved_count, "unresolved identifier count");

    start_block = program.top_level_decls[0]->as.start_decl.body.as.block;
    return_identifier = start_block->statements[0]->as.return_expression;
    ASSERT_TRUE(symbol_table_resolve_identifier(&table, return_identifier) == NULL,
                "unresolved identifier has no resolved symbol");
    ASSERT_TRUE(table.unresolved_identifiers[0].identifier == return_identifier,
                "unresolved identifier is recorded");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_symbol_table_duplicate_top_level_binding_error(void) {
    const char *source =
        "int32 value = 1;\n"
        "int32 value = 2;\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const SymbolTableError *error;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&table);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse duplicate top-level binding program");
    ASSERT_TRUE(!symbol_table_build(&table, &program), "duplicate top-level binding fails");

    error = symbol_table_get_error(&table);
    REQUIRE_TRUE(error != NULL, "duplicate top-level binding error exists");
    ASSERT_EQ_INT(2, error->primary_span.start_line, "duplicate top-level primary line");
    ASSERT_EQ_INT(7, error->primary_span.start_column, "duplicate top-level primary column");
    ASSERT_EQ_INT(1, error->related_span.start_line, "duplicate top-level related line");
    ASSERT_EQ_INT(7, error->related_span.start_column, "duplicate top-level related column");
    REQUIRE_TRUE(symbol_table_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format duplicate top-level diagnostic");
    ASSERT_EQ_STR("2:7: Duplicate symbol 'value' in program scope. Previous declaration at 1:7.",
                  diagnostic,
                  "formatted duplicate top-level diagnostic");
    ASSERT_CONTAINS("value", error->message, "duplicate top-level binding name in error");
    ASSERT_CONTAINS("program scope", error->message, "duplicate top-level binding scope in error");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_symbol_table_duplicate_parameter_error(void) {
    const char *source =
        "int32 add = (int32 value, int32 value) -> value;\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const SymbolTableError *error;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&table);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse duplicate parameter program");
    ASSERT_TRUE(!symbol_table_build(&table, &program), "duplicate parameter fails");

    error = symbol_table_get_error(&table);
    REQUIRE_TRUE(error != NULL, "duplicate parameter error exists");
    ASSERT_EQ_INT(1, error->primary_span.start_line, "duplicate parameter primary line");
    ASSERT_EQ_INT(33, error->primary_span.start_column, "duplicate parameter primary column");
    ASSERT_EQ_INT(1, error->related_span.start_line, "duplicate parameter related line");
    ASSERT_EQ_INT(20, error->related_span.start_column, "duplicate parameter related column");
    REQUIRE_TRUE(symbol_table_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format duplicate parameter diagnostic");
    ASSERT_EQ_STR("1:33: Duplicate symbol 'value' in lambda scope. Previous declaration at 1:20.",
                  diagnostic,
                  "formatted duplicate parameter diagnostic");
    ASSERT_CONTAINS("value", error->message, "duplicate parameter name in error");
    ASSERT_CONTAINS("lambda scope", error->message, "duplicate parameter scope in error");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_symbol_table_duplicate_local_error(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    var value = 1;\n"
        "    var value = 2;\n"
        "    return value;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const SymbolTableError *error;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&table);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse duplicate local program");
    ASSERT_TRUE(!symbol_table_build(&table, &program), "duplicate local fails");

    error = symbol_table_get_error(&table);
    REQUIRE_TRUE(error != NULL, "duplicate local error exists");
    ASSERT_EQ_INT(3, error->primary_span.start_line, "duplicate local primary line");
    ASSERT_EQ_INT(9, error->primary_span.start_column, "duplicate local primary column");
    ASSERT_EQ_INT(2, error->related_span.start_line, "duplicate local related line");
    ASSERT_EQ_INT(9, error->related_span.start_column, "duplicate local related column");
    REQUIRE_TRUE(symbol_table_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format duplicate local diagnostic");
    ASSERT_EQ_STR("3:9: Duplicate symbol 'value' in block scope. Previous declaration at 2:9.",
                  diagnostic,
                  "formatted duplicate local diagnostic");
    ASSERT_CONTAINS("value", error->message, "duplicate local name in error");
    ASSERT_CONTAINS("block scope", error->message, "duplicate local scope in error");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_symbol_table_duplicate_import_error(void) {
    const char *source =
        "import io.stdlib;\n"
        "import other.stdlib;\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const SymbolTableError *error;
    char diagnostic_buffer[256];
    char *diagnostic = diagnostic_buffer;

    symbol_table_init(&table);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse duplicate import program");
    ASSERT_TRUE(!symbol_table_build(&table, &program), "duplicate import fails");

    error = symbol_table_get_error(&table);
    REQUIRE_TRUE(error != NULL, "duplicate import error exists");
    ASSERT_EQ_INT(2, error->primary_span.start_line, "duplicate import primary line");
    ASSERT_EQ_INT(14, error->primary_span.start_column, "duplicate import primary column");
    ASSERT_EQ_INT(1, error->related_span.start_line, "duplicate import related line");
    ASSERT_EQ_INT(11, error->related_span.start_column, "duplicate import related column");
    REQUIRE_TRUE(symbol_table_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format duplicate import diagnostic");
    ASSERT_EQ_STR("2:14: Duplicate import alias 'stdlib'. Previous declaration at 1:11.",
                  diagnostic,
                  "formatted duplicate import diagnostic");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

