#include "../src/hir.h"
#include "../src/lir.h"
#include "../src/mir.h"
#include "../src/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

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

static void test_lir_dump_lowers_minimal_callable_slice(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    int32 total = add(1, 2);\n"
        "    return total;\n"
        "};\n";
    static const char expected[] =
        "LirProgram\n"
        "  Unit name=add kind=binding return=int32 captures=0 params=2 slots=2 vregs=1 blocks=1\n"
        "    Slots:\n"
        "      Slot index=0 kind=param name=left type=int32 final=false\n"
        "      Slot index=1 kind=param name=right type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        incoming arg0 -> slot(0:left)\n"
        "        incoming arg1 -> slot(1:right)\n"
        "        v0 = binary + slot(0:left) slot(1:right)\n"
        "        return vreg(0)\n"
        "  Unit name=start kind=start return=int32 captures=0 params=1 slots=2 vregs=1 blocks=1\n"
        "    Slots:\n"
        "      Slot index=0 kind=param name=args type=string[] final=false\n"
        "      Slot index=1 kind=local name=total type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        incoming arg0 -> slot(0:args)\n"
        "        out arg0 <- int32(1)\n"
        "        out arg1 <- int32(2)\n"
        "        v0 = call global(add) argc=2\n"
        "        store slot(1:total) <- vreg(0)\n"
        "        return slot(1:total)\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse LIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for LIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check LIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for LIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for LIR program");
    REQUIRE_TRUE(lir_build_program(&lir_program, &mir_program),
                 "lower LIR for minimal callable slice");

    dump = lir_dump_program_to_string(&lir_program);
    REQUIRE_TRUE(dump != NULL, "render LIR dump to string");
    ASSERT_EQ_STR(expected, dump, "LIR dump string");

    free(dump);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

static void test_lir_dump_covers_globals_branches_closures_and_runtime_like_ops(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "int32 seed = 1;\n"
        "start(string[] args) -> {\n"
        "    bool flag = true;\n"
        "    var values = [seed, 2, 3];\n"
        "    int32 chosen = flag ? values[1] : values[0];\n"
        "    var render = (int32 x) -> `value ${chosen + ((int32 y) -> y + x)(1)}`;\n"
        "    values[0] += chosen;\n"
        "    stdlib.print(render(2));\n"
        "    return values[0];\n"
        "};\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse broad LIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for broad LIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check broad LIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for broad LIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for broad LIR program");
    REQUIRE_TRUE(lir_build_program(&lir_program, &mir_program),
                 "lower LIR for broad MIR surface");

    dump = lir_dump_program_to_string(&lir_program);
    REQUIRE_TRUE(dump != NULL, "render broad LIR dump to string");
    ASSERT_CONTAINS("Unit name=__mir$module_init kind=init", dump,
                    "module init unit is present in LIR dump");
    ASSERT_CONTAINS("store global(seed) <- int32(1)", dump,
                    "global initialization lowers into LIR");
    ASSERT_CONTAINS("call global(__mir$module_init) argc=0", dump,
                    "start calls module init through explicit call ABI ops");
    ASSERT_CONTAINS("branch slot(", dump,
                    "branching stays explicit in LIR");
    ASSERT_CONTAINS("Unit name=start$lambda0 kind=lambda", dump,
                    "outer closure unit is present in LIR dump");
    ASSERT_CONTAINS("Unit name=start$lambda0$lambda1 kind=lambda", dump,
                    "nested closure unit is present in LIR dump");
    ASSERT_CONTAINS("incoming capture0 -> slot(", dump,
                    "closure captures lower into explicit incoming capture ops");
    ASSERT_CONTAINS("closure unit=start$lambda0(", dump,
                    "closure construction lowers into LIR");
    ASSERT_CONTAINS("template(text(value ), value(", dump,
                    "template construction lowers into LIR");
    ASSERT_CONTAINS("member global(stdlib).print", dump,
                    "external member access lowers into LIR");
    ASSERT_CONTAINS("store index slot(", dump,
                    "index stores lower into LIR");
    ASSERT_CONTAINS("out arg0 <- int32(2)", dump,
                    "call arguments lower into explicit outgoing ABI ops");

    free(dump);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

static void test_lir_dump_lowers_throw_terminator(void) {
    static const char source[] =
        "int32 fail = () -> {\n"
        "    throw \"boom\";\n"
        "    return 0;\n"
        "};\n"
        "start(string[] args) -> fail();\n";
    static const char expected[] =
        "LirProgram\n"
        "  Unit name=fail kind=binding return=int32 captures=0 params=0 slots=0 vregs=0 blocks=1\n"
        "    Slots:\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        throw string(\"boom\")\n"
        "  Unit name=start kind=start return=int32 captures=0 params=1 slots=1 vregs=1 blocks=1\n"
        "    Slots:\n"
        "      Slot index=0 kind=param name=args type=string[] final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        incoming arg0 -> slot(0:args)\n"
        "        v0 = call global(fail) argc=0\n"
        "        return vreg(0)\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse throw LIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for throw LIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check throw LIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for throw LIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for throw LIR program");
    REQUIRE_TRUE(lir_build_program(&lir_program, &mir_program),
                 "lower LIR for throw terminator");

    dump = lir_dump_program_to_string(&lir_program);
    REQUIRE_TRUE(dump != NULL, "render throw LIR dump to string");
    ASSERT_EQ_STR(expected, dump, "throw LIR dump string");

    free(dump);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

int main(void) {
    printf("Running LIR dump tests...\n\n");

    RUN_TEST(test_lir_dump_lowers_minimal_callable_slice);
    RUN_TEST(test_lir_dump_covers_globals_branches_closures_and_runtime_like_ops);
    RUN_TEST(test_lir_dump_lowers_throw_terminator);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}