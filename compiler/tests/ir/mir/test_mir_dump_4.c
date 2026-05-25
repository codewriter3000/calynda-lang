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


void test_mir_dump_lowers_callable_local_bindings_and_nested_closures(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32 delta = 3;\n"
        "    var render = (int32 x) -> ((int32 y) -> y + x + delta)(1);\n"
        "    return render(2);\n"
        "};\n";
    static const char expected[] =
        "MirProgram\n"
        "  Unit name=start$lambda0$lambda1 kind=lambda return=int32 params=1 locals=3 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=capture name=x type=int32 final=false\n"
        "      Local index=1 kind=capture name=delta type=<external> final=false\n"
        "      Local index=2 kind=param name=y type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = binary + local(2:y) local(0:x)\n"
        "        t1 = call global(__calynda_rt_cell_read)(local(1:delta))\n"
        "        t2 = binary + temp(0) temp(1)\n"
        "        return temp(2)\n"
        "  Unit name=start$lambda0 kind=lambda return=int32 params=1 locals=2 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=capture name=delta type=<external> final=false\n"
        "      Local index=1 kind=param name=x type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = closure unit=start$lambda0$lambda1(local(1:x), local(0:delta))\n"
        "        t1 = call temp(0)(int32(1))\n"
        "        return temp(1)\n"
        "  Unit name=start kind=start return=int32 params=1 locals=3 blocks=1\n"
        "    Locals:\n"
        "      Local index=0 kind=param name=args type=string[] final=false\n"
        "      Local index=1 kind=local name=delta type=<external> final=false\n"
        "      Local index=2 kind=local name=render type=int32 final=false\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = call global(__calynda_rt_cell_alloc)(int32(3))\n"
        "        store local(1:delta) <- temp(0)\n"
        "        t1 = closure unit=start$lambda0(local(1:delta))\n"
        "        store local(2:render) <- temp(1)\n"
        "        t2 = call local(2:render)(int32(2))\n"
        "        return temp(2)\n";
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
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
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


void test_mir_dump_lowers_top_level_value_bindings_into_module_init(void) {
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
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
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


void test_mir_dump_prunes_unreachable_top_level_callable_units(void) {
    static const char source[] =
        "int32 helper = () -> 7;\n"
        "start(string[] args) -> 0;\n";
    static const char expected[] =
        "MirProgram\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program),
                 "parse dead callable MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program),
                 "build symbols for dead callable MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check dead callable MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for dead callable MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "prune unreachable top-level callable MIR unit");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render dead callable MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "dead callable MIR unit should be pruned");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}


void test_mir_dump_prunes_unreachable_pure_module_init_bindings(void) {
    static const char source[] =
        "int32 value = 1;\n"
        "int32 doubled = value + value;\n"
        "start(string[] args) -> 0;\n";
    static const char expected[] =
        "MirProgram\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program),
                 "parse dead pure module-init MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program),
                 "build symbols for dead pure module-init MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check dead pure module-init MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for dead pure module-init MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "prune unreachable pure module-init bindings");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render dead pure module-init MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "dead pure module-init bindings should be pruned");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}


void test_mir_dump_prunes_unreachable_branching_module_init_binding(void) {
    static const char source[] =
        "int32 value = true ? 1 : 2;\n"
        "start(string[] args) -> 0;\n";
    static const char expected[] =
        "MirProgram\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program),
                 "parse dead branching module-init MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program),
                 "build symbols for dead branching module-init MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check dead branching module-init MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for dead branching module-init MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "prune unreachable branching module-init binding");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render dead branching module-init MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "dead branching module-init binding should be pruned");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}


void test_mir_dump_prunes_unreachable_short_circuit_module_init_bindings(void) {
    static const char source[] =
        "bool both = true && false;\n"
        "bool either = false || true;\n"
        "start(string[] args) -> 0;\n";
    static const char expected[] =
        "MirProgram\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program),
                 "parse dead short-circuit module-init MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program),
                 "build symbols for dead short-circuit module-init MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check dead short-circuit module-init MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for dead short-circuit module-init MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "prune unreachable short-circuit module-init bindings");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL,
                 "render dead short-circuit module-init MIR dump to string");
    ASSERT_EQ_STR(expected,
                  dump,
                  "dead short-circuit module-init bindings should be pruned");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}


void test_mir_dump_keeps_call_backed_module_init_bindings(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "int32 total = add(1, 2);\n"
        "start(string[] args) -> 0;\n";
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
        "  Unit name=__mir$module_init kind=init return=void params=0 locals=0 blocks=1\n"
        "    Locals:\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        t0 = call global(add)(int32(1), int32(2))\n"
        "        store global(total) <- temp(0)\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program),
                 "parse call-backed module-init MIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program),
                 "build symbols for call-backed module-init MIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check call-backed module-init MIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for call-backed module-init MIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "keep call-backed module-init bindings conservatively");

    dump = mir_dump_program_to_string(&mir_program);
    REQUIRE_TRUE(dump != NULL, "render call-backed module-init MIR dump to string");
    ASSERT_EQ_STR(expected, dump, "call-backed module-init bindings stay reachable");

    free(dump);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}


void test_mir_dump_lowers_throw_as_terminator(void) {
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
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
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

