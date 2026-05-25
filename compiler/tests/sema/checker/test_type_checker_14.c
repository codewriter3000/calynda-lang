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

void test_type_checker_accepts_spawn_non_void_callable_as_future(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    Future<int32> t = spawn () -> 1;\n"
        "    return t.get();\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse future spawn source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols future spawn");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "spawn accepts non-void callable as Future<T>");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_future_and_atomic_builtins(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    Future<int32> future = spawn () -> 41;\n"
        "    Atomic<int32> counter = Atomic.new(1);\n"
        "    int32 value = future.get();\n"
        "    final int32 oldValue = counter.exchange(value);\n"
        "    Thread worker = spawn () -> {\n"
        "        _ = counter.exchange(oldValue);\n"
        "        exit;\n"
        "    };\n"
        "    future.cancel();\n"
        "    worker.cancel();\n"
        "    worker.join();\n"
        "    fence();\n"
        "    return counter.load();\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse future/atomic source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols future/atomic");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "future/thread/atomic builtins type check");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_warns_on_spawn_mutable_capture(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32 shared = 1;\n"
        "    Thread worker = spawn () -> {\n"
        "        int32 copy = shared;\n"
        "        _ = copy;\n"
        "        exit;\n"
        "    };\n"
        "    worker.join();\n"
        "    return 0;\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *warning;

#include "test_type_checker_14_p2.inc"

void test_type_checker_warns_on_external_callable_dispatch(void) {
    static const char source[] =
        "int32 apply = (var fn) -> int32(fn());\n"
        "start(string[] args) -> {\n"
        "    return apply(() -> 7);\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *warning;

    type_checker_set_global_performance_warnings(true);
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse external callable dispatch source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols external callable dispatch");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "dynamic external callable dispatch still type checks");

    ASSERT_EQ_INT(1, (int)type_checker_warning_count(&checker),
                  "dynamic external callable dispatch records one warning");
    warning = type_checker_get_warning(&checker);
    REQUIRE_TRUE(warning != NULL, "external callable dispatch warning exists");
    REQUIRE_TRUE(type_checker_format_error(warning, diagnostic, sizeof(diagnostic)),
                 "format external callable dispatch warning");
    ASSERT_CONTAINS("Dynamic callable dispatch", diagnostic,
                    "warning explains runtime helper dispatch");

    type_checker_free(&checker);
    type_checker_init(&checker);
    type_checker_set_global_performance_warnings(false);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "dynamic external callable dispatch still type checks when warnings disabled");
    ASSERT_EQ_INT(0, (int)type_checker_warning_count(&checker),
                  "disabling performance warnings suppresses the warning");

    type_checker_set_global_performance_warnings(true);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_controls_template_literal_advisories(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    string text = `hello ${args[0]}`;\n"
        "    return int32(text.length);\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *advisory;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse template advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols template advisory source");

    type_checker_set_global_performance_advisories(false);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "template literal program type checks with advisories disabled");
    ASSERT_EQ_INT(0, (int)type_checker_advisory_count(&checker),
                  "template literal advisory is disabled by default");

    type_checker_free(&checker);
    type_checker_init(&checker);
    type_checker_set_global_performance_advisories(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "template literal program type checks with advisories enabled");
    ASSERT_EQ_INT(1, (int)type_checker_advisory_count(&checker),
                  "template literal records one advisory when enabled");
    advisory = type_checker_get_advisory(&checker);
    REQUIRE_TRUE(advisory != NULL, "template advisory exists");
    REQUIRE_TRUE(type_checker_format_error(advisory, diagnostic, sizeof(diagnostic)),
                 "format template advisory");
    ASSERT_CONTAINS("Complex template literals", diagnostic,
                    "advisory explains hosted complex template helper cost");

    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_suppresses_simple_hosted_template_advisory(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    string text = `${args[0]}`;\n"
        "    return int32(text.length);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse simple hosted template advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for simple hosted template advisory source");

    type_checker_set_global_performance_advisories(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "simple hosted template still type checks with advisories enabled");
    ASSERT_EQ_INT(0, (int)type_checker_advisory_count(&checker),
                  "simple hosted single-expression template stays silent");

    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_emits_strong_template_advisory_in_manual_block(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        string text = `${42}`;\n"
        "        _ = text.length;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *advisory;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse manual template advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for manual template advisory source");

    type_checker_set_global_performance_advisories(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "manual template still type checks with advisories enabled");
    ASSERT_EQ_INT(1, (int)type_checker_advisory_count(&checker),
                  "manual template records one strong advisory");
    advisory = type_checker_get_advisory(&checker);
    REQUIRE_TRUE(advisory != NULL, "manual template advisory exists");
    REQUIRE_TRUE(type_checker_format_error(advisory, diagnostic, sizeof(diagnostic)),
                 "format manual template advisory");
    ASSERT_CONTAINS("boot, manual, or size-focused code", diagnostic,
                    "manual template advisory uses the strong context wording");

    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_emits_strong_template_advisory_in_boot(void) {
    static const char source[] =
        "boot -> {\n"
        "    string text = `${42}`;\n"
        "    _ = text.length;\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *advisory;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse boot template advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for boot template advisory source");

    type_checker_set_global_performance_advisories(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "boot template still type checks with advisories enabled");
    ASSERT_EQ_INT(1, (int)type_checker_advisory_count(&checker),
                  "boot template records one strong advisory");
    advisory = type_checker_get_advisory(&checker);
    REQUIRE_TRUE(advisory != NULL, "boot template advisory exists");
    REQUIRE_TRUE(type_checker_format_error(advisory, diagnostic, sizeof(diagnostic)),
                 "format boot template advisory");
    ASSERT_CONTAINS("boot, manual, or size-focused code", diagnostic,
                    "boot template advisory uses the strong context wording");

    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_emits_strong_template_advisory_in_size_focus_mode(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    string text = `${42}`;\n"
        "    return int32(text.length);\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *advisory;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse size-focus template advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for size-focus template advisory source");

    type_checker_set_global_performance_advisories(true);
    type_checker_set_global_size_focus(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "size-focus template still type checks with advisories enabled");
    ASSERT_EQ_INT(1, (int)type_checker_advisory_count(&checker),
                  "size-focus template records one strong advisory");
    advisory = type_checker_get_advisory(&checker);
    REQUIRE_TRUE(advisory != NULL, "size-focus template advisory exists");
    REQUIRE_TRUE(type_checker_format_error(advisory, diagnostic, sizeof(diagnostic)),
                 "format size-focus template advisory");
    ASSERT_CONTAINS("boot, manual, or size-focused code", diagnostic,
                    "size-focus template advisory uses the strong context wording");

    type_checker_set_global_size_focus(false);
    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_omitted_static_array_extent_stays_silent(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    return values[0];\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse static omitted-array-extent advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for static omitted-array-extent advisory source");

    type_checker_set_global_performance_advisories(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "static omitted array extent program type checks");
    ASSERT_EQ_INT(0, (int)type_checker_advisory_count(&checker),
                  "statically inferred omitted array extent stays silent");

    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_omitted_runtime_array_extent_emits_advisory(void) {
    static const char source[] =
        "var passthrough = (int32[] input) -> input;\n"
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    int32[] copy = passthrough(values);\n"
        "    return int32(copy.length);\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *advisory;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse runtime omitted-array-extent advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for runtime omitted-array-extent advisory source");

    type_checker_set_global_performance_advisories(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "runtime omitted array extent program type checks");
    ASSERT_EQ_INT(1, (int)type_checker_advisory_count(&checker),
                  "runtime-derived omitted array extent records one advisory");
    advisory = type_checker_get_advisory(&checker);
    REQUIRE_TRUE(advisory != NULL, "runtime omitted array extent advisory exists");
    REQUIRE_TRUE(type_checker_format_error(advisory, diagnostic, sizeof(diagnostic)),
                 "format runtime omitted array extent advisory");
    ASSERT_CONTAINS("length will be determined at runtime", diagnostic,
                    "advisory explains runtime-derived array length");

    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_omitted_static_array_return_stays_silent(void) {
    static const char source[] =
        "int32[] make_values = () -> [1, 2, 3];\n"
        "start(string[] args) -> int32(make_values().length);\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse static omitted-array-return advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for static omitted-array-return advisory source");

    type_checker_set_global_performance_advisories(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "static omitted array return program type checks");
    ASSERT_EQ_INT(0, (int)type_checker_advisory_count(&checker),
                  "statically inferred omitted array return stays silent");

    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_omitted_runtime_array_return_emits_advisory(void) {
    static const char source[] =
        "var passthrough = (int32[] input) -> input;\n"
        "int32[] make_values = () -> passthrough([1, 2, 3]);\n"
        "start(string[] args) -> int32(make_values().length);\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *advisory;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse runtime omitted-array-return advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for runtime omitted-array-return advisory source");

    type_checker_set_global_performance_advisories(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "runtime omitted array return program type checks");
    ASSERT_EQ_INT(1, (int)type_checker_advisory_count(&checker),
                  "runtime-derived omitted array return records one advisory");
    advisory = type_checker_get_advisory(&checker);
    REQUIRE_TRUE(advisory != NULL, "runtime omitted array return advisory exists");
    REQUIRE_TRUE(type_checker_format_error(advisory, diagnostic, sizeof(diagnostic)),
                 "format runtime omitted array return advisory");
    ASSERT_CONTAINS("Return type for lambda body omits an array length", diagnostic,
                    "advisory identifies the return boundary");

    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_omitted_static_default_array_extent_stays_silent(void) {
    static const char source[] =
        "int32 consume = (int32[] values = [1, 2, 3]) -> int32(values.length);\n"
        "start(string[] args) -> consume();\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse static omitted-default-array-extent advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for static omitted-default-array-extent advisory source");

    type_checker_set_global_performance_advisories(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "static omitted default array extent program type checks");
    ASSERT_EQ_INT(0, (int)type_checker_advisory_count(&checker),
                  "statically inferred omitted default array extent stays silent");

    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_omitted_runtime_default_array_extent_emits_advisory(void) {
    static const char source[] =
        "var passthrough = (int32[] input) -> input;\n"
        "int32 consume = (int32[] values = passthrough([1, 2, 3])) -> int32(values.length);\n"
        "start(string[] args) -> consume();\n";
    char diagnostic[512];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const TypeCheckError *advisory;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse runtime omitted-default-array-extent advisory source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for runtime omitted-default-array-extent advisory source");

    type_checker_set_global_performance_advisories(true);
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "runtime omitted default array extent program type checks");
    ASSERT_EQ_INT(1, (int)type_checker_advisory_count(&checker),
                  "runtime-derived omitted default array extent records one advisory");
    advisory = type_checker_get_advisory(&checker);
    REQUIRE_TRUE(advisory != NULL, "runtime omitted default array extent advisory exists");
    REQUIRE_TRUE(type_checker_format_error(advisory, diagnostic, sizeof(diagnostic)),
                 "format runtime omitted default array extent advisory");
    ASSERT_CONTAINS("Default value for parameter 'values' omits an array length", diagnostic,
                    "advisory identifies the default-value boundary");

    type_checker_set_global_performance_advisories(false);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_preserves_static_extent_across_declared_array_return_boundary(void) {
    static const char source[] =
        "int32[] make_values = () -> [1, 2, 3];\n"
        "start(string[] args) -> {\n"
        "    int32[3] values = make_values();\n"
        "    return values[0];\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse declared array return boundary source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for declared array return boundary source");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "declared omitted array return preserves static extent for callers");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
