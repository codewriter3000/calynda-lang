#include "hir.h"
#include "mir.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

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

void test_mir_dump_rewrites_self_tail_recursion_into_loop(void) {
    static const char source[] =
        "int32 count_down = (int32 value) -> value <= 0 ? 0 : count_down(value - 1);\n"
        "start(string[] args) -> {\n"
        "    return count_down(3);\n"
        "};\n";
    static const char expected[] =
        "MirProgram\n"
        "  Unit name=count_down kind=binding return=int32 params=1 locals=3 blocks=5\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=value type=int32 final=false\n"
        "      Local index=1 kind=synthetic name=__mir_ternary0 type=int32 final=false\n"
        "      Local index=2 kind=synthetic name=__mir_tco0 type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        goto bb4\n"
        "      Block bb1:\n"
        "        store local(1:__mir_ternary0) <- int32(0)\n"
        "        goto bb3\n"
        "      Block bb2:\n"
        "        t2 = binary - local(0:value) int32(1)\n"
        "        store local(2:__mir_tco0) <- temp(2)\n"
        "        store local(0:value) <- local(2:__mir_tco0)\n"
        "        goto bb4\n"
        "      Block bb3:\n"
        "        return local(1:__mir_ternary0)\n"
        "      Block bb4:\n"
        "        t0 = binary <= local(0:value) int32(0)\n"
        "        branch temp(0) -> bb1, bb2\n"
        "  Unit name=start kind=start return=int32 params=1 locals=1 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=args type=string[] final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = call global(count_down)(int32(3))\n"
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

    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse self-tail-recursive MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program),
                 "build symbols for self-tail-recursive MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check self-tail-recursive MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for self-tail-recursive MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "lower MIR for self-tail-recursive MIR program");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render self-tail-recursive MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "self-tail-recursive MIR dump string");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}
