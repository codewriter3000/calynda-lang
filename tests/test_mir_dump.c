#include "../src/hir.h"
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

#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

static void test_mir_dump_lowers_minimal_callable_slice(void) {
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

static void test_mir_dump_lowers_ternary_into_branching_blocks(void) {
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

static void test_mir_dump_lowers_short_circuit_logical_operators(void) {
    static const char source[] =
        "bool both = (bool left, bool right) -> left && right;\n"
        "bool either = (bool left, bool right) -> left || right;\n"
        "start(string[] args) -> 0;\n";
    static const char expected[] =
        "MirProgram\n"
        "  Unit name=both kind=binding return=bool params=2 locals=3 blocks=4\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=left type=bool final=false\n"
        "      Local index=1 kind=param name=right type=bool final=false\n"
        "      Local index=2 kind=synthetic name=__mir_logical0 type=bool final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        branch local(0:left) -> bb1, bb2\n"
        "      Block bb1:\n"
        "        store local(2:__mir_logical0) <- local(1:right)\n"
        "        goto bb3\n"
        "      Block bb2:\n"
        "        store local(2:__mir_logical0) <- bool(false)\n"
        "        goto bb3\n"
        "      Block bb3:\n"
        "        return local(2:__mir_logical0)\n"
        "  Unit name=either kind=binding return=bool params=2 locals=3 blocks=4\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=left type=bool final=false\n"
        "      Local index=1 kind=param name=right type=bool final=false\n"
        "      Local index=2 kind=synthetic name=__mir_logical0 type=bool final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        branch local(0:left) -> bb1, bb2\n"
        "      Block bb1:\n"
        "        store local(2:__mir_logical0) <- bool(true)\n"
        "        goto bb3\n"
        "      Block bb2:\n"
        "        store local(2:__mir_logical0) <- local(1:right)\n"
        "        goto bb3\n"
        "      Block bb3:\n"
        "        return local(2:__mir_logical0)\n"
        "  Unit name=start kind=start return=int32 params=1 locals=1 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=args type=string[] final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse logical MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for logical MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check logical MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for logical MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for short-circuit logical operators");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render logical MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "logical MIR dump string");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

static void test_mir_dump_lowers_arrays_assignments_members_and_templates(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "start(string[] args) -> {\n"
        "    var values = [1, 2, 3];\n"
        "    values[0] += values[1];\n"
        "    stdlib.print(`value ${values[0]}`);\n"
        "    return values[0];\n"
        "};\n";
    static const char expected[] =
        "MirProgram\n"
        "  Unit name=start kind=start return=int32 params=1 locals=2 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=args type=string[] final=false\n"
        "      Local index=1 kind=local name=values type=int32[] final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = array(int32(1), int32(2), int32(3))\n"
        "        store local(1:values) <- temp(0)\n"
        "        t1 = index local(1:values)[int32(0)]\n"
        "        t2 = index local(1:values)[int32(1)]\n"
        "        t3 = binary + temp(1) temp(2)\n"
        "        store index local(1:values)[int32(0)] <- temp(3)\n"
        "        t4 = member global(stdlib).print\n"
        "        t7 = index local(1:values)[int32(0)]\n"
        "        t6 = template(text(value ), value(temp(7)), text())\n"
        "        t5 = call temp(4)(temp(6))\n"
        "        t8 = index local(1:values)[int32(0)]\n"
        "        return temp(8)\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse rich MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for rich MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check rich MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for rich MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for arrays assignments members and templates");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render rich MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "rich MIR dump string");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

static void test_mir_dump_lowers_callable_local_bindings_and_nested_closures(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32 offset = 3;\n"
        "    var render = (int32 x) -> ((int32 y) -> y + x + offset)(1);\n"
        "    return render(2);\n"
        "};\n";
    static const char expected[] =
        "MirProgram\n"
        "  Unit name=start$lambda0$lambda1 kind=lambda return=int32 params=1 locals=3 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=capture name=x type=int32 final=false\n"
        "      Local index=1 kind=capture name=offset type=int32 final=false\n"
        "      Local index=2 kind=param name=y type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = binary + local(2:y) local(0:x)\n"
        "        t1 = binary + temp(0) local(1:offset)\n"
        "        return temp(1)\n"
        "  Unit name=start$lambda0 kind=lambda return=int32 params=1 locals=2 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=capture name=offset type=int32 final=false\n"
        "      Local index=1 kind=param name=x type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = closure unit=start$lambda0$lambda1(local(1:x), local(0:offset))\n"
        "        t1 = call temp(0)(int32(1))\n"
        "        return temp(1)\n"
        "  Unit name=start kind=start return=int32 params=1 locals=3 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=args type=string[] final=false\n"
        "      Local index=1 kind=local name=offset type=int32 final=false\n"
        "      Local index=2 kind=local name=render type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        store local(1:offset) <- int32(3)\n"
        "        t0 = closure unit=start$lambda0(local(1:offset))\n"
        "        store local(2:render) <- temp(0)\n"
        "        t1 = call local(2:render)(int32(2))\n"
        "        return temp(1)\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse closure MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for closure MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check closure MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for closure MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for callable locals and nested closures");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render closure MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "closure MIR dump string");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

static void test_mir_dump_lowers_top_level_value_bindings_into_module_init(void) {
    static const char source[] =
        "int32 value = 1;\n"
        "int32 doubled = value + value;\n"
        "start(string[] args) -> {\n"
        "    return doubled;\n"
        "};\n";
    static const char expected[] =
        "MirProgram\n"
        "  Unit name=__mir$module_init kind=init return=void params=0 locals=0 blocks=1\n"
        "    Locals:\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        store global(value) <- int32(1)\n"
        "        t0 = binary + global(value) global(value)\n"
        "        store global(doubled) <- temp(0)\n"
        "        return\n"
        "  Unit name=start kind=start return=int32 params=1 locals=1 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=args type=string[] final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        call global(__mir$module_init)()\n"
        "        return global(doubled)\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse module-init MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for module-init MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check module-init MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for module-init MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for top-level value bindings");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render module-init MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "module-init MIR dump string");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

static void test_mir_dump_lowers_throw_as_terminator(void) {
    static const char source[] =
        "int32 fail = () -> {\n"
        "    throw \"boom\";\n"
        "    return 0;\n"
        "};\n"
        "start(string[] args) -> fail();\n";
    static const char expected[] =
        "MirProgram\n"
        "  Unit name=fail kind=binding return=int32 params=0 locals=0 blocks=1\n"
        "    Locals:\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        throw string(\"boom\")\n"
        "  Unit name=start kind=start return=int32 params=1 locals=1 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=args type=string[] final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = call global(fail)()\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse throw MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for throw MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check throw MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for throw MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for throw terminator");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render throw MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "throw MIR dump string");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

int main(void) {
    printf("Running MIR dump tests...\n\n");

    RUN_TEST(test_mir_dump_lowers_minimal_callable_slice);
    RUN_TEST(test_mir_dump_lowers_ternary_into_branching_blocks);
    RUN_TEST(test_mir_dump_lowers_short_circuit_logical_operators);
    RUN_TEST(test_mir_dump_lowers_arrays_assignments_members_and_templates);
    RUN_TEST(test_mir_dump_lowers_callable_local_bindings_and_nested_closures);
    RUN_TEST(test_mir_dump_lowers_top_level_value_bindings_into_module_init);
    RUN_TEST(test_mir_dump_lowers_throw_as_terminator);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}