#include "hir.h"
#include "mir.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

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
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\" in dump\n",    \
                __FILE__, __LINE__, (msg), (needle));                       \
    }                                                                       \
} while (0)
#define ASSERT_NOT_CONTAINS(needle, haystack, msg) do {                     \
    tests_run++;                                                            \
    if ((haystack) != NULL && strstr((haystack), (needle)) == NULL) {       \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: unexpected \"%s\" in dump\n", \
                __FILE__, __LINE__, (msg), (needle));                       \
    }                                                                       \
} while (0)

void test_mir_dump_checked_manual_block_uses_bc_functions(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual checked {\n"
        "        int64 p = malloc(64);\n"
        "        free(p);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program),
                 "parse manual checked source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program),
                 "build symbols for manual checked source");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check manual checked source");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for manual checked source");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "lower MIR for manual checked source");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "manual checked MIR dump string is not NULL");

    ASSERT_CONTAINS("__calynda_bc_malloc", dump,
                    "manual checked block uses __calynda_bc_malloc");
    ASSERT_CONTAINS("__calynda_bc_free", dump,
                    "manual checked block uses __calynda_bc_free");
    ASSERT_NOT_CONTAINS("call global(malloc)", dump,
                        "manual checked block does not use plain malloc");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

void test_mir_dump_ptr_checked_type_uses_bc_deref(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        ptr<int64, checked> p = malloc(8);\n"
        "        int64 v = deref(p);\n"
        "        free(p);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program),
                 "parse ptr<T, checked> source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program),
                 "build symbols for ptr<T, checked> source");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check ptr<T, checked> source");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for ptr<T, checked> source");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "lower MIR for ptr<T, checked> source");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "ptr<T, checked> MIR dump string is not NULL");

    ASSERT_CONTAINS("__calynda_bc_deref", dump,
                    "ptr<T, checked> deref uses __calynda_bc_deref");
    ASSERT_NOT_CONTAINS("call global(deref)", dump,
                        "ptr<T, checked> does not use plain deref");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

void test_mir_dump_plain_manual_block_uses_plain_functions(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 p = malloc(64);\n"
        "        free(p);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program),
                 "parse plain manual source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program),
                 "build symbols for plain manual source");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check plain manual source");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for plain manual source");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "lower MIR for plain manual source");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "plain manual MIR dump string is not NULL");

    ASSERT_CONTAINS("call global(malloc)", dump,
                    "plain manual block uses plain malloc");
    ASSERT_NOT_CONTAINS("__calynda_bc_malloc", dump,
                        "plain manual block does not use __calynda_bc_malloc");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}
