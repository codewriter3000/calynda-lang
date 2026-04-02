#include "../src/parser.h"
#include "../src/symbol_table.h"

#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

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

static void test_symbol_table_source_spans_and_unresolved_diagnostic(void) {
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

static void test_symbol_table_build_and_resolution(void) {
    const char *source =
        "package sample.app;\n"
        "import io.stdlib;\n"
        "import math.core;\n"
        "public int32 inc = (int32 value) -> value + 1;\n"
        "public int32 useImport = stdlib.value;\n"
        "start(string[] args) -> {\n"
        "    var total = inc(41);\n"
        "    var computed = ((int32 total) -> {\n"
        "        var inner = total;\n"
        "        return inner;\n"
        "    })(total);\n"
        "    return computed;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const Symbol *package_symbol;
    const Symbol *stdlib_import;
    const Scope *root_scope;
    const Scope *start_scope;
    const Scope *start_block_scope;
    const Scope *inc_lambda_scope;
    const Scope *inner_lambda_scope;
    const Scope *inner_block_scope;
    const Symbol *inc_symbol;
    const Symbol *args_symbol;
    const Symbol *outer_total_symbol;
    const Symbol *inner_total_symbol;
    const Symbol *inner_symbol;
    const AstExpression *inc_lambda_expr;
    const AstExpression *use_import_identifier;
    const AstBlock *start_block;
    const AstExpression *call_inc_identifier;
    const AstExpression *computed_initializer;
    const AstExpression *call_argument_total;
    const AstExpression *inner_lambda_expr;
    const AstBlock *inner_block;
    const AstExpression *inner_initializer_total;
    const AstExpression *inner_return_identifier;
    const AstExpression *inc_lambda_value_identifier;

    symbol_table_init(&table);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse source program");
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbol table");
    ASSERT_TRUE(symbol_table_get_error(&table) == NULL, "no symbol table error");

    package_symbol = symbol_table_get_package(&table);
    REQUIRE_TRUE(package_symbol != NULL, "package symbol exists");
    ASSERT_EQ_INT(SYMBOL_KIND_PACKAGE, package_symbol->kind, "package symbol kind");
    ASSERT_EQ_STR("app", package_symbol->name, "package symbol short name");
    ASSERT_EQ_STR("sample.app", package_symbol->qualified_name,
                  "package symbol qualified name");

    stdlib_import = symbol_table_find_import(&table, "stdlib");
    REQUIRE_TRUE(stdlib_import != NULL, "stdlib import exists");
    ASSERT_EQ_INT(2, table.import_count, "import count");
    ASSERT_EQ_INT(SYMBOL_KIND_IMPORT, stdlib_import->kind, "import symbol kind");
    ASSERT_EQ_STR("io.stdlib", stdlib_import->qualified_name,
                  "import qualified name");

    root_scope = symbol_table_root_scope(&table);
    REQUIRE_TRUE(root_scope != NULL, "root scope exists");
    ASSERT_EQ_INT(SCOPE_KIND_PROGRAM, root_scope->kind, "root scope kind");
    ASSERT_EQ_INT(2, root_scope->symbol_count, "program scope symbol count");

    inc_symbol = scope_lookup_local(root_scope, "inc");
    REQUIRE_TRUE(inc_symbol != NULL, "top-level inc symbol exists");
    ASSERT_EQ_INT(SYMBOL_KIND_TOP_LEVEL_BINDING, inc_symbol->kind,
                  "inc symbol kind");

    inc_lambda_expr = program.top_level_decls[0]->as.binding_decl.initializer;
    inc_lambda_scope = symbol_table_find_scope(&table, inc_lambda_expr, SCOPE_KIND_LAMBDA);
    REQUIRE_TRUE(inc_lambda_scope != NULL, "inc lambda scope exists");
    ASSERT_EQ_INT(1, inc_lambda_scope->symbol_count, "inc lambda parameter count");

    start_scope = symbol_table_find_scope(&table,
                                          &program.top_level_decls[2]->as.start_decl,
                                          SCOPE_KIND_START);
    REQUIRE_TRUE(start_scope != NULL, "start scope exists");
    ASSERT_EQ_INT(1, start_scope->symbol_count, "start scope parameter count");

    start_block = program.top_level_decls[2]->as.start_decl.body.as.block;
    start_block_scope = symbol_table_find_scope(&table, start_block, SCOPE_KIND_BLOCK);
    REQUIRE_TRUE(start_block_scope != NULL, "start block scope exists");
    ASSERT_EQ_INT(2, start_block_scope->symbol_count, "start block local count");

    args_symbol = scope_lookup_local(start_scope, "args");
    REQUIRE_TRUE(args_symbol != NULL, "start args symbol exists");
    ASSERT_EQ_INT(SYMBOL_KIND_PARAMETER, args_symbol->kind, "args symbol kind");
    ASSERT_TRUE(symbol_table_lookup(&table, start_block_scope, "args") == args_symbol,
                "start block can look up args parameter");

    outer_total_symbol = scope_lookup_local(start_block_scope, "total");
    REQUIRE_TRUE(outer_total_symbol != NULL, "outer total symbol exists");
    ASSERT_EQ_INT(SYMBOL_KIND_LOCAL, outer_total_symbol->kind, "outer total symbol kind");

    use_import_identifier =
        program.top_level_decls[1]->as.binding_decl.initializer->as.member.target;
    ASSERT_TRUE(symbol_table_resolve_identifier(&table, use_import_identifier) == stdlib_import,
                "import alias resolves inside member access target");

    inc_lambda_value_identifier =
        program.top_level_decls[0]->as.binding_decl.initializer
            ->as.lambda.body.as.expression->as.binary.left;
    ASSERT_TRUE(symbol_table_resolve_identifier(&table, inc_lambda_value_identifier) ==
                    scope_lookup_local(inc_lambda_scope, "value"),
                "lambda body identifier resolves to lambda parameter");

    call_inc_identifier = start_block->statements[0]->as.local_binding.initializer->as.call.callee;
    ASSERT_TRUE(symbol_table_resolve_identifier(&table, call_inc_identifier) == inc_symbol,
                "call inside start block resolves to top-level binding");

    computed_initializer = start_block->statements[1]->as.local_binding.initializer;
    call_argument_total = computed_initializer->as.call.arguments.items[0];
    ASSERT_TRUE(symbol_table_resolve_identifier(&table, call_argument_total) == outer_total_symbol,
                "call argument resolves to outer local");

    inner_lambda_expr = computed_initializer->as.call.callee->as.grouping.inner;
    inner_lambda_scope = symbol_table_find_scope(&table, inner_lambda_expr, SCOPE_KIND_LAMBDA);
    REQUIRE_TRUE(inner_lambda_scope != NULL, "inner lambda scope exists");
    inner_total_symbol = scope_lookup_local(inner_lambda_scope, "total");
    REQUIRE_TRUE(inner_total_symbol != NULL, "inner lambda total parameter exists");

    inner_block = inner_lambda_expr->as.lambda.body.as.block;
    inner_block_scope = symbol_table_find_scope(&table, inner_block, SCOPE_KIND_BLOCK);
    REQUIRE_TRUE(inner_block_scope != NULL, "inner block scope exists");
    ASSERT_EQ_INT(1, inner_block_scope->symbol_count, "inner block local count");

    inner_symbol = scope_lookup_local(inner_block_scope, "inner");
    REQUIRE_TRUE(inner_symbol != NULL, "inner local symbol exists");
    ASSERT_TRUE(symbol_table_lookup(&table, inner_block_scope, "total") == inner_total_symbol,
                "inner block lookup prefers lambda parameter over outer local");

    inner_initializer_total = inner_block->statements[0]->as.local_binding.initializer;
    ASSERT_TRUE(symbol_table_resolve_identifier(&table, inner_initializer_total) == inner_total_symbol,
                "inner local initializer resolves to lambda parameter");

    inner_return_identifier = inner_block->statements[1]->as.return_expression;
    ASSERT_TRUE(symbol_table_resolve_identifier(&table, inner_return_identifier) == inner_symbol,
                "return expression resolves to inner local");

    ASSERT_EQ_INT(0, table.unresolved_count, "no unresolved identifiers in resolved sample");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_symbol_table_records_unresolved_identifier(void) {
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

static void test_symbol_table_duplicate_top_level_binding_error(void) {
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

static void test_symbol_table_duplicate_parameter_error(void) {
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

static void test_symbol_table_duplicate_local_error(void) {
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

static void test_symbol_table_duplicate_import_error(void) {
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

int main(void) {
    printf("Running symbol table tests...\n\n");

    RUN_TEST(test_symbol_table_build_and_resolution);
    RUN_TEST(test_symbol_table_source_spans_and_unresolved_diagnostic);
    RUN_TEST(test_symbol_table_records_unresolved_identifier);
    RUN_TEST(test_symbol_table_duplicate_top_level_binding_error);
    RUN_TEST(test_symbol_table_duplicate_parameter_error);
    RUN_TEST(test_symbol_table_duplicate_local_error);
    RUN_TEST(test_symbol_table_duplicate_import_error);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}