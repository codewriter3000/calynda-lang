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
