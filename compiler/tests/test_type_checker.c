#include "../src/parser.h"
#include "../src/symbol_table.h"
#include "../src/type_checker.h"

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

static void test_type_checker_infers_symbols_and_expressions(void) {
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

static void test_type_checker_rejects_binding_type_mismatch(void) {
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

static void test_type_checker_rejects_bad_call_arguments(void) {
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

static void test_type_checker_rejects_non_bool_ternary_condition(void) {
    const char *source = "start(string[] args) -> 1 ? 2 : 3;\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse ternary condition program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for ternary condition");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "non-bool ternary condition fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "ternary condition error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format ternary condition error");
    ASSERT_EQ_STR("1:25: Ternary condition must have type bool but got int32.",
                  diagnostic,
                  "formatted ternary condition diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_void_lambda_body_for_typed_binding(void) {
    const char *source =
        "int32 bad = (int32 value) -> { value + 1; };\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse void lambda body program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for void lambda body");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "void lambda body fails typed binding check");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "void lambda body error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format void lambda body error");
    ASSERT_EQ_STR("1:13: Lambda body must produce int32 but got void. Related location at 1:7.",
                  diagnostic,
                  "formatted void lambda body diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_bad_start_return_type(void) {
    const char *source = "start(string[] args) -> true;\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bad start program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for bad start");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "bad start return type fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "bad start error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format bad start error");
    ASSERT_EQ_STR("1:25: start body must produce int32 but got bool. Related location at 1:1.",
                  diagnostic,
                  "formatted bad start diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_void_zero_arg_callable_in_template(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    var message = `${() -> { exit; }}`;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse void template callable program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for void template callable");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "void zero-arg callable in template fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "void template callable error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format void template callable error");
    ASSERT_EQ_STR(
        "2:22: Template interpolation cannot auto-call a zero-argument callable returning void.",
        diagnostic,
        "formatted void template callable diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_reports_type_resolution_errors(void) {
    const char *source = "start(void args) -> 0;\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse type resolution error program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for type resolution error");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "type resolution error fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "type resolution error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format type resolution error");
    ASSERT_EQ_STR("1:12: Parameter 'args' cannot have type void.",
                  diagnostic,
                  "formatted type resolution diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_requires_start_entry_point(void) {
    const char *source = "int32 value = 1;\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse missing start program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for missing start");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "missing start fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "missing start error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format missing start error");
    ASSERT_EQ_STR("Program must declare exactly one start entry point.",
                  diagnostic,
                  "formatted missing start diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_duplicate_start_entry_points(void) {
    const char *source =
        "start(string[] args) -> 0;\n"
        "start(string[] args) -> 1;\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse duplicate start program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for duplicate start");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "duplicate start fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "duplicate start error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format duplicate start error");
    ASSERT_EQ_STR("2:1: Program cannot declare multiple start entry points. Related location at 1:1.",
                  diagnostic,
                  "formatted duplicate start diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_invalid_start_parameter_type(void) {
    const char *source = "start(int32 args) -> 0;\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse invalid start parameter program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for invalid start parameter");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "invalid start parameter fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "invalid start parameter error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format invalid start parameter error");
    ASSERT_EQ_STR("1:13: start parameter must have type string[]. Related location at 1:1.",
                  diagnostic,
                  "formatted invalid start parameter diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_allows_exit_in_void_lambda_block(void) {
    const char *source =
        "void cleanup = () -> { exit; };\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse void lambda exit program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for void lambda exit");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "void lambda exit passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_bare_return_in_non_void_lambda(void) {
    const char *source =
        "int32 bad = () -> { return; };\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bare return lambda program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for bare return lambda");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "bare return in non-void lambda fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "bare return lambda error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format bare return lambda error");
    ASSERT_EQ_STR("1:21: Return statement in lambda body must produce int32. Related location at 1:7.",
                  diagnostic,
                  "formatted bare return lambda diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_exit_in_start_block(void) {
    const char *source = "start(string[] args) -> { exit; };\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse start exit program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for start exit");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "exit in start fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "start exit error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format start exit error");
    ASSERT_EQ_STR("1:27: exit is only permitted in void-typed lambda blocks. Related location at 1:1.",
                  diagnostic,
                  "formatted start exit diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_bare_return_in_start_block(void) {
    const char *source = "start(string[] args) -> { return; };\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bare start return program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for bare start return");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "bare return in start fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "bare start return error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format bare start return error");
    ASSERT_EQ_STR("1:27: Return statement in start must produce int32. Related location at 1:1.",
                  diagnostic,
                  "formatted bare start return diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_allows_assignment_to_local_array_element(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    var values = [1, 2, 3];\n"
        "    values[0] = 4;\n"
        "    return values[0];\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse local array assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for local array assignment");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "local array element assignment passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_assignment_to_temporary_index_target(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    [1, 2, 3][0] = 4;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse temporary index assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for temporary index assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "temporary index assignment fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "temporary index assignment error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format temporary index assignment error");
    ASSERT_EQ_STR("2:5: Operator '=' requires an assignable target.",
                  diagnostic,
                  "formatted temporary index assignment diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_assignment_to_import_symbol(void) {
    const char *source =
        "import io.stdlib;\n"
        "start(string[] args) -> {\n"
        "    stdlib = 1;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse import assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for import assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "assignment to import symbol fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "import assignment error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format import assignment error");
    ASSERT_EQ_STR("3:5: Operator '=' requires an assignable target. Related location at 1:11.",
                  diagnostic,
                  "formatted import assignment diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_assignment_to_final_index_target(void) {
    const char *source =
        "final int32[] values = [1, 2, 3];\n"
        "start(string[] args) -> {\n"
        "    values[0] = 4;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse final index assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for final index assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "assignment through final root fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "final index assignment error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format final index assignment error");
    ASSERT_EQ_STR("3:5: Cannot assign to final symbol 'values'. Related location at 1:15.",
                  diagnostic,
                  "formatted final index assignment diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_checker_rejects_incompatible_compound_assignment(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    bool ready = true;\n"
        "    ready += 1;\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse bad compound assignment program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for bad compound assignment");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "incompatible compound assignment fails type checking");

    error = type_checker_get_error(&checker);
    REQUIRE_TRUE(error != NULL, "bad compound assignment error exists");
    REQUIRE_TRUE(type_checker_format_error(error, diagnostic, sizeof(diagnostic_buffer)),
                 "format bad compound assignment error");
    ASSERT_EQ_STR("3:5: Operator '+' cannot be applied to types bool and int32.",
                  diagnostic,
                  "formatted bad compound assignment diagnostic");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

int main(void) {
    printf("Running type checker tests...\n\n");

    RUN_TEST(test_type_checker_infers_symbols_and_expressions);
    RUN_TEST(test_type_checker_rejects_binding_type_mismatch);
    RUN_TEST(test_type_checker_rejects_bad_call_arguments);
    RUN_TEST(test_type_checker_rejects_non_bool_ternary_condition);
    RUN_TEST(test_type_checker_rejects_void_lambda_body_for_typed_binding);
    RUN_TEST(test_type_checker_rejects_bad_start_return_type);
    RUN_TEST(test_type_checker_rejects_void_zero_arg_callable_in_template);
    RUN_TEST(test_type_checker_reports_type_resolution_errors);
    RUN_TEST(test_type_checker_requires_start_entry_point);
    RUN_TEST(test_type_checker_rejects_duplicate_start_entry_points);
    RUN_TEST(test_type_checker_rejects_invalid_start_parameter_type);
    RUN_TEST(test_type_checker_allows_exit_in_void_lambda_block);
    RUN_TEST(test_type_checker_rejects_bare_return_in_non_void_lambda);
    RUN_TEST(test_type_checker_rejects_exit_in_start_block);
    RUN_TEST(test_type_checker_rejects_bare_return_in_start_block);
    RUN_TEST(test_type_checker_allows_assignment_to_local_array_element);
    RUN_TEST(test_type_checker_rejects_assignment_to_temporary_index_target);
    RUN_TEST(test_type_checker_rejects_assignment_to_import_symbol);
    RUN_TEST(test_type_checker_rejects_assignment_to_final_index_target);
    RUN_TEST(test_type_checker_rejects_incompatible_compound_assignment);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}