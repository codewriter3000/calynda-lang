#include "car.h"
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
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {       \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\" in \"%s\"\n", \
                __FILE__, __LINE__, (msg), (needle),                        \
                (haystack) ? (haystack) : "(null)");                       \
    }                                                                       \
} while (0)

static bool add_archive_source(CarArchive *archive,
                               const char *path,
                               const char *source) {
    return archive &&
           path &&
           source &&
           car_archive_add_file(archive, path, source, strlen(source));
}

void test_type_checker_accepts_recursive_top_level_lambda_binding(void) {
    static const char source[] =
        "int32 sum_down = (int32 value) -> value <= 0 ? 0 : value + sum_down(value - 1);\n"
        "start(string[] args) -> {\n"
        "    return sum_down(4);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse recursive top-level lambda binding program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for recursive top-level lambda binding");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "recursive top-level lambda binding passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_recursive_top_level_manual_lambda_binding(void) {
    static const char source[] =
        "int32 factorial = manual(int32 value) -> {\n"
        "    return value <= 1 ? 1 : value * factorial(value - 1);\n"
        "};\n"
        "start(string[] args) -> {\n"
        "    return factorial(5);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse recursive top-level manual lambda binding program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for recursive top-level manual lambda binding");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "recursive top-level manual lambda binding passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_recursive_non_lambda_top_level_binding(void) {
    static const char source[] =
        "int32 countdown = countdown + 1;\n"
        "start(string[] args) -> {\n"
        "    return countdown;\n"
        "};\n";
    char diagnostic[256];
    const char *captured_diagnostic = NULL;
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse recursive non-lambda top-level binding program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for recursive non-lambda top-level binding");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "recursive non-lambda top-level binding is rejected");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format recursive non-lambda top-level binding diagnostic");
    captured_diagnostic = diagnostic;
    ASSERT_CONTAINS("Circular definition involving 'countdown'.",
                    captured_diagnostic,
                    "recursive non-lambda top-level binding reports circular definition");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_wildcard_dep_archive_import(void) {
    static const char lib_source[] =
        "package lib.math;\n"
        "export int32 squarePower = (int32 value) -> value * value;\n";
    static const char main_source[] =
        "import lib.math.*;\n"
        "start(string[] args) -> {\n"
        "    return squarePower(5) == 25 ? 0 : 1;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    CarArchive archive;

    car_archive_init(&archive);
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, main_source);
    REQUIRE_TRUE(add_archive_source(&archive, "lib/math.cal", lib_source),
                 "build wildcard dependency archive");
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse wildcard dependency archive program");
    REQUIRE_TRUE(symbol_table_build_with_archive_deps(&symbols, &program,
                                                      &archive, 1),
                 "build symbols for wildcard dependency archive program");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "wildcard dependency archive import passes type checking");

    car_archive_free(&archive);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_resolves_overloads_by_exact_and_widening_match(void) {
    static const char source[] =
        "int32 pick = (int32 value) -> 11;\n"
        "int32 pick = (string value) -> 22;\n"
        "start(string[] args) -> {\n"
        "    int32 exact = pick(7);\n"
        "    int32 widened = pick(int16(7));\n"
        "    return exact + widened - 22;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const Symbol *int_pick_symbol;
    const AstExpression *exact_call;
    const AstExpression *widened_call;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse overload exact/widening program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for overload exact/widening program");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "overload exact/widening program passes type checking");

    int_pick_symbol = symbol_table_find_symbol_for_declaration(&symbols,
                                                               &program.top_level_decls[0]->as.binding_decl);
    REQUIRE_TRUE(int_pick_symbol != NULL, "int32 overload symbol found");
    exact_call = program.top_level_decls[2]
                     ->as.start_decl.body.as.block->statements[0]
                     ->as.local_binding.initializer;
    widened_call = program.top_level_decls[2]
                       ->as.start_decl.body.as.block->statements[1]
                       ->as.local_binding.initializer;
    REQUIRE_TRUE(exact_call && exact_call->kind == AST_EXPR_CALL,
                 "exact call initializer captured");
    REQUIRE_TRUE(widened_call && widened_call->kind == AST_EXPR_CALL,
                 "widened call initializer captured");
    ASSERT_TRUE(symbol_table_resolve_identifier(&symbols, exact_call->as.call.callee) ==
                    int_pick_symbol,
                "exact overload call selects int32 overload");
    ASSERT_TRUE(symbol_table_resolve_identifier(&symbols, widened_call->as.call.callee) ==
                    int_pick_symbol,
                "widening overload call selects int32 overload");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_ambiguous_overload_call(void) {
    static const char source[] =
        "int32 pick = (int32 value) -> 1;\n"
        "int32 pick = (int64 value) -> 2;\n"
        "start(string[] args) -> {\n"
        "    return pick(int16(7));\n"
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
                 "parse ambiguous overload program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for ambiguous overload program");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "ambiguous overload call is rejected");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format ambiguous overload diagnostic");
    ASSERT_CONTAINS("ambiguous", diagnostic,
                    "ambiguous overload diagnostic mentions ambiguity");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_resolves_overloads_by_arity(void) {
    static const char source[] =
        "int32 pick = (int32 value) -> value;\n"
        "int32 pick = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    return pick(1, 2);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const Symbol *binary_pick_symbol;
    const AstExpression *call_expr;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse overload arity program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for overload arity program");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "overload arity program passes type checking");

    binary_pick_symbol = symbol_table_find_symbol_for_declaration(
        &symbols,
        &program.top_level_decls[1]->as.binding_decl);
    REQUIRE_TRUE(binary_pick_symbol != NULL, "binary overload symbol found");
    call_expr = program.top_level_decls[2]
                    ->as.start_decl.body.as.block->statements[0]
                    ->as.return_expression;
    REQUIRE_TRUE(call_expr && call_expr->kind == AST_EXPR_CALL,
                 "arity-selected return expression is call");
    ASSERT_TRUE(symbol_table_resolve_identifier(&symbols, call_expr->as.call.callee) ==
                    binary_pick_symbol,
                "two-argument call selects two-parameter overload");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
