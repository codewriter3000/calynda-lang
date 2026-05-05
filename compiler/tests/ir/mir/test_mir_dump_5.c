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


void test_mir_dump_lowers_union_new_instructions(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "Option<int32> x = Option.Some(42);\n"
        "Option<int32> y = Option.None;\n"
        "start(string[] args) -> 0;\n";
    static const char expected[] =
        "MirProgram\n"
        "  Unit name=__mir$module_init kind=init return=void params=0 locals=0 blocks=1\n"
        "    Locals:\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = union_new typedesc(Option|1|g0:int32|Some:int32|None:void) variant 0 payload int32(42)\n"
        "        store global(x) <- temp(0)\n"
        "        t1 = union_new typedesc(Option|1|g0:raw_word|Some:raw_word|None:void) variant 1\n"
        "        store global(y) <- temp(1)\n"
        "        return\n"
        "  Unit name=start kind=start return=int32 params=1 locals=1 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=args type=string[] final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        call global(__mir$module_init)()\n"
        "        return int32(0)\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse union MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for union MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check union MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for union MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "lower MIR for union new instructions");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render union MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "union new MIR dump string");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

