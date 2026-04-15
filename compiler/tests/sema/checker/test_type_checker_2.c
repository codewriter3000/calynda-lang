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


void test_type_checker_infers_symbols_and_expressions(void) {
    const char *source =
        "int32 add = (int32 a, int32 b) -> a + b;\n"
        "start(string[] args) -> {\n"
        "    var values = [1, 2, 3];\n"
        "    var total = add(values[0], int32(2.5));\n"
        "    return total;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const Scope *root_scope;
    const Scope *start_block_scope;
    const Symbol *add_symbol;
    const Symbol *values_symbol;
    const Symbol *total_symbol;
    const TypeCheckInfo *add_info;
    const TypeCheckInfo *values_info;
    const TypeCheckInfo *total_info;
    const TypeCheckInfo *call_info;
    const TypeCheckInfo *index_info;
    const AstBlock *start_block;
    const AstExpression *array_literal;
    const AstExpression *call_expression;
    const AstExpression *index_expression;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse valid typed program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for valid typed program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check valid program");

    root_scope = symbol_table_root_scope(&symbols);
    REQUIRE_TRUE(root_scope != NULL, "root scope exists");
    add_symbol = scope_lookup_local(root_scope, "add");
    REQUIRE_TRUE(add_symbol != NULL, "add symbol exists");
    add_info = type_checker_get_symbol_info(&checker, add_symbol);
    REQUIRE_TRUE(add_info != NULL, "type info for add exists");
    ASSERT_TRUE(add_info->is_callable, "add binding is callable");
    ASSERT_EQ_INT(CHECKED_TYPE_VALUE, add_info->type.kind, "add type kind");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, add_info->type.primitive, "add return primitive");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, add_info->callable_return_type.primitive,
                  "add callable return primitive");

    start_block = program.top_level_decls[1]->as.start_decl.body.as.block;
    start_block_scope = symbol_table_find_scope(&symbols, start_block, SCOPE_KIND_BLOCK);
    REQUIRE_TRUE(start_block_scope != NULL, "start block scope exists");
    values_symbol = scope_lookup_local(start_block_scope, "values");
    total_symbol = scope_lookup_local(start_block_scope, "total");
    REQUIRE_TRUE(values_symbol != NULL, "values symbol exists");
    REQUIRE_TRUE(total_symbol != NULL, "total symbol exists");

    values_info = type_checker_get_symbol_info(&checker, values_symbol);
    total_info = type_checker_get_symbol_info(&checker, total_symbol);
    REQUIRE_TRUE(values_info != NULL, "values type info exists");
    REQUIRE_TRUE(total_info != NULL, "total type info exists");
    ASSERT_EQ_INT(CHECKED_TYPE_VALUE, values_info->type.kind, "values type kind");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, values_info->type.primitive, "values element primitive");
    ASSERT_EQ_INT(1, values_info->type.array_depth, "values array depth");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, total_info->type.primitive, "total primitive");
    ASSERT_EQ_INT(0, total_info->type.array_depth, "total array depth");

    array_literal = start_block->statements[0]->as.local_binding.initializer;
    call_expression = start_block->statements[1]->as.local_binding.initializer;
    index_expression = call_expression->as.call.arguments.items[0];
    call_info = type_checker_get_expression_info(&checker, call_expression);
    index_info = type_checker_get_expression_info(&checker, index_expression);
    REQUIRE_TRUE(call_info != NULL, "call expression type info exists");
    REQUIRE_TRUE(index_info != NULL, "index expression type info exists");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, call_info->type.primitive, "call result primitive");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, index_info->type.primitive, "index result primitive");
    ASSERT_EQ_INT(1,
                  type_checker_get_expression_info(&checker, array_literal)->type.array_depth,
                  "array literal type depth");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_binding_type_mismatch(void) {
    const char *source =
        "int32 value = true;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse binding mismatch program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for binding mismatch");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "binding mismatch fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "binding mismatch error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format binding mismatch error");
    ASSERT_EQ_STR("1:15: Cannot assign expression of type bool to top-level binding 'value' of type int32. Related location at 1:7.",
                  diagnostic,
                  "formatted binding mismatch diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_rejects_bad_call_arguments(void) {
    const char *source =
        "int32 add = (int32 a, int32 b) -> a + b;\n"
        "start(string[] args) -> add(true, 2);\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bad call program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for bad call");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "bad call arguments fail type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "bad call error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format bad call error");
    ASSERT_EQ_STR("2:29: Argument 1 to call expects int32 but got bool. Related location at 1:20.",
                  diagnostic,
                  "formatted bad call diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

