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


void test_mir_dump_lowers_short_circuit_logical_operators(void) {
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
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
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


void test_mir_dump_lowers_arrays_assignments_members_and_templates(void) {
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
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
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

