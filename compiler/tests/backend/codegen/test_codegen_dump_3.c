#include "codegen.h"
#include "hir.h"
#include "lir.h"
#include "mir.h"
#include "parser.h"
#include "target.h"

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


void test_codegen_dump_allocates_registers_then_spills(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32 a = 1 + 2;\n"
        "    int32 b = 3 + 4;\n"
        "    int32 c = 5 + 6;\n"
        "    int32 d = 7 + 8;\n"
        "    int32 e = 9 + 10;\n"
        "    return a + b + c + d + e;\n"
        "};\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    CodegenProgram codegen_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse spilling codegen program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for spilling codegen program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check spilling codegen program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for spilling codegen program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for spilling codegen program");
    REQUIRE_TRUE(lir_build_program(&lir_program, &mir_program),
                 "lower LIR for spilling codegen program");
    REQUIRE_TRUE(codegen_build_program(&codegen_program, &lir_program, target_get_default()),
                 "build codegen plan for spilling program");

    dump = codegen_dump_program_to_string(&codegen_program);
    REQUIRE_TRUE(dump != NULL, "render spilling codegen dump to string");
    ASSERT_CONTAINS("Unit name=start kind=start return=int32 frame_slots=6 spills=5 vregs=9 blocks=1",
                    dump,
                    "register allocator reports spill count after the allocatable set is exhausted");
    ASSERT_CONTAINS("VReg index=0 type=int32 -> reg(r10)",
                    dump,
                    "first virtual register uses r10");
    ASSERT_CONTAINS("VReg index=1 type=int32 -> reg(r11)",
                    dump,
                    "second virtual register uses r11");
    ASSERT_CONTAINS("VReg index=2 type=int32 -> reg(r12)",
                    dump,
                    "third virtual register uses r12");
    ASSERT_CONTAINS("VReg index=3 type=int32 -> reg(r13)",
                    dump,
                    "fourth virtual register uses r13");
    ASSERT_CONTAINS("VReg index=4 type=int32 -> spill(0)",
                    dump,
                    "fifth virtual register spills to the stack");
    ASSERT_CONTAINS("VReg index=8 type=int32 -> spill(4)",
                    dump,
                    "later virtual registers continue spilling in order");

    free(dump);
    codegen_program_free(&codegen_program);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}


void test_codegen_dump_routes_throw_through_runtime_helper(void) {
    static const char source[] =
        "int32 fail = () -> {\n"
        "    throw \"boom\";\n"
        "    return 0;\n"
        "};\n"
        "start(string[] args) -> fail();\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    CodegenProgram codegen_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse throw codegen program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for throw codegen program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check throw codegen program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for throw codegen program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for throw codegen program");
    REQUIRE_TRUE(lir_build_program(&lir_program, &mir_program),
                 "lower LIR for throw codegen program");
    REQUIRE_TRUE(codegen_build_program(&codegen_program, &lir_program, target_get_default()),
                 "build codegen plan for throw codegen program");

    dump = codegen_dump_program_to_string(&codegen_program);
    REQUIRE_TRUE(dump != NULL, "render throw codegen dump to string");
    ASSERT_CONTAINS("terminator throw -> runtime __calynda_rt_throw",
                    dump,
                    "throw remains runtime-backed at the first instruction-selection target");

    free(dump);
    codegen_program_free(&codegen_program);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

