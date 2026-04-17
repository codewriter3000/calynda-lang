#include "hir.h"
#include "hir_dump.h"
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


void test_hir_dump_shows_export_and_static_flags(void) {
    static const char source[] =
        "export int32 visible = () -> 1;\n"
        "static int32 counter = () -> 0;\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse export/static HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols export/static HIR");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check export/static HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for export/static");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "HIR dump string is not NULL");

    ASSERT_CONTAINS("exported", dump, "HIR dump contains exported flag");
    ASSERT_CONTAINS("static", dump, "HIR dump contains static flag");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_hir_dump_lowers_union_declarations(void) {
    const char *source =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse union HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols union HIR");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check union HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for union declarations");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "union HIR dump string is not NULL");

    ASSERT_CONTAINS("Union", dump, "HIR dump contains Union declaration");
    ASSERT_CONTAINS("Option", dump, "HIR dump contains Option name");
    ASSERT_CONTAINS("Some", dump, "HIR dump contains Some variant");
    ASSERT_CONTAINS("None", dump, "HIR dump contains None variant");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_hir_dump_lowers_manual_block_with_memory_ops(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 mem = malloc(64);\n"
        "        free(mem);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse manual HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols manual HIR");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check manual HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for manual block");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "manual HIR dump string is not NULL");

    ASSERT_CONTAINS("Manual", dump, "HIR dump contains Manual statement");
    ASSERT_CONTAINS("MemoryOp", dump, "HIR dump contains MemoryOp expression");
    ASSERT_CONTAINS("malloc", dump, "HIR dump contains malloc");
    ASSERT_CONTAINS("free", dump, "HIR dump contains free");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_lowers_threading_helpers(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    Thread t = spawn () -> { };\n"
        "    Mutex m = Mutex.new();\n"
        "    t.join();\n"
        "    m.lock();\n"
        "    m.unlock();\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse threading HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build threading HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check threading HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build threading HIR");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "threading HIR dump string is not NULL");
    ASSERT_CONTAINS("__calynda_rt_thread_spawn", dump, "HIR lowers spawn to helper");
    ASSERT_CONTAINS("__calynda_rt_thread_join", dump, "HIR lowers join to helper");
    ASSERT_CONTAINS("__calynda_rt_mutex_new", dump, "HIR lowers mutex new to helper");
    ASSERT_CONTAINS("__calynda_rt_mutex_lock", dump, "HIR lowers mutex lock to helper");
    ASSERT_CONTAINS("__calynda_rt_mutex_unlock", dump, "HIR lowers mutex unlock to helper");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_lowers_array_car_cdr_helpers(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    int32 head = car(values);\n"
        "    int32[] tail = cdr(values);\n"
        "    return head + int32(tail.length);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse car/cdr HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build car/cdr HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check car/cdr HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build car/cdr HIR");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "car/cdr HIR dump string is not NULL");
    ASSERT_CONTAINS("__calynda_rt_array_car", dump, "HIR lowers car to a direct helper");
    ASSERT_CONTAINS("__calynda_rt_array_cdr", dump, "HIR lowers cdr to a direct helper");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_inlines_default_arguments(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right = left + 1) -> left + right;\n"
        "start -> add(4);\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse HIR default argument source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build HIR default argument symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check HIR default argument source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for default arguments");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "HIR default-argument dump is not NULL");
    ASSERT_CONTAINS("Arg 1:\n                Literal kind=0 type=int32 text=4",
                    dump,
                    "HIR preserves explicit argument");
    ASSERT_CONTAINS("Arg 2:\n                Binary op=+ type=int32",
                    dump,
                    "HIR synthesizes omitted default argument");
    ASSERT_CONTAINS("Left:\n                    Literal kind=0 type=int32 text=4",
                    dump,
                    "default argument inlines earlier explicit arguments at the call site");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_expands_swap_statement(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    values[0] >< values[2];\n"
        "    return values[0];\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse swap HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build swap HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check swap HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for swap statement");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "swap HIR dump string is not NULL");
    ASSERT_CONTAINS("Block statements=5", dump, "swap expands into extra HIR statements");
    ASSERT_CONTAINS("Local name=__swap_tmp_0 type=int32", dump, "swap introduces temp local");
    ASSERT_CONTAINS("Assignment op=0 type=int32", dump, "swap lowers to assignment expressions");
    ASSERT_CONTAINS("Symbol name=__swap_tmp_0 kind=local type=int32", dump,
                    "swap reuses temp local in final assignment");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_lowers_overload_calls_to_selected_signature(void) {
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
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse overload HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build overload HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check overload HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build overload HIR");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "overload HIR dump string is not NULL");
    ASSERT_CONTAINS("callable=(int32) -> int32", dump,
                    "HIR dump keeps unary overload signature");
    ASSERT_CONTAINS("Symbol name=pick kind=top-level binding type=int32 callable=(int32, int32) -> int32",
                    dump,
                    "HIR call uses the selected binary overload signature");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
