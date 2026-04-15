#include "hir.h"
#include "mir.h"
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
#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)


void test_mir_dump_lowers_minimal_callable_slice(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    int32 total = add(1, 2);\n"
        "    return total;\n"
        "};\n";
    static const char expected[] =
        "MirProgram\n"
        "  Unit name=add kind=binding return=int32 params=2 locals=2 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=left type=int32 final=false\n"
        "      Local index=1 kind=param name=right type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = binary + local(0:left) local(1:right)\n"
        "        return temp(0)\n"
        "  Unit name=start kind=start return=int32 params=1 locals=2 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=args type=string[] final=false\n"
        "      Local index=1 kind=local name=total type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = call global(add)(int32(1), int32(2))\n"
        "        store local(1:total) <- temp(0)\n"
        "        return local(1:total)\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for minimal slice");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "MIR dump string");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}


void test_mir_dump_lowers_ternary_into_branching_blocks(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    bool flag = true;\n"
        "    int32 value = flag ? 1 : 2;\n"
        "    return value + 3;\n"
        "};\n";
    static const char expected[] =
        "MirProgram\n"
        "  Unit name=start kind=start return=int32 params=1 locals=4 blocks=4\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=args type=string[] final=false\n"
        "      Local index=1 kind=local name=flag type=bool final=false\n"
        "      Local index=2 kind=local name=value type=int32 final=false\n"
        "      Local index=3 kind=synthetic name=__mir_ternary0 type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        store local(1:flag) <- bool(true)\n"
        "        branch local(1:flag) -> bb1, bb2\n"
        "      Block bb1:\n"
        "        store local(3:__mir_ternary0) <- int32(1)\n"
        "        goto bb3\n"
        "      Block bb2:\n"
        "        store local(3:__mir_ternary0) <- int32(2)\n"
        "        goto bb3\n"
        "      Block bb3:\n"
        "        store local(2:value) <- local(3:__mir_ternary0)\n"
        "        t0 = binary + local(2:value) int32(3)\n"
        "        return temp(0)\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse ternary MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for ternary MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check ternary MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for ternary MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for ternary branch shape");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render branching MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "branching MIR dump string");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

