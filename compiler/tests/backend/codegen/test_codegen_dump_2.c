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


void test_codegen_dump_defines_x86_64_sysv_target_and_direct_patterns(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    int32 total = add(1, 2);\n"
        "    return total;\n"
        "};\n";
    static const char expected[] =
        "CodegenProgram target=x86_64_sysv_elf return=rax env=r15 args=[rdi, rsi, rdx, rcx, r8, r9] alloc=[r10, r11, r12, r13]\n"
        "  Unit name=add kind=binding return=int32 frame_slots=2 spills=0 vregs=1 blocks=1\n"
        "    FrameSlots:\n"
        "      FrameSlot index=0 from=param name=left type=int32 final=false\n"
        "      FrameSlot index=1 from=param name=right type=int32 final=false\n"
        "    VRegs:\n"
        "      VReg index=0 type=int32 -> reg(r10)\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        incoming-arg -> direct abi-arg-move\n"
        "        incoming-arg -> direct abi-arg-move\n"
        "        binary -> direct scalar-binary\n"
        "        terminator return -> direct return\n"
        "  Unit name=start kind=start return=int32 frame_slots=2 spills=0 vregs=1 blocks=1\n"
        "    FrameSlots:\n"
        "      FrameSlot index=0 from=param name=args type=string[] final=false\n"
        "      FrameSlot index=1 from=local name=total type=int32 final=false\n"
        "    VRegs:\n"
        "      VReg index=0 type=int32 -> reg(r10)\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        incoming-arg -> direct abi-arg-move\n"
        "        outgoing-arg -> direct abi-outgoing-arg\n"
        "        outgoing-arg -> direct abi-outgoing-arg\n"
        "        call -> direct call-global\n"
        "        store-slot -> direct store-slot\n"
        "        terminator return -> direct return\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse codegen program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for codegen program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check codegen program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for codegen program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for codegen program");
    REQUIRE_TRUE(lir_build_program(&lir_program, &mir_program),
                 "lower LIR for codegen program");
    REQUIRE_TRUE(codegen_build_program(&codegen_program, &lir_program, target_get_default()),
                 "build codegen plan for minimal direct patterns");

    dump = codegen_dump_program_to_string(&codegen_program);
    REQUIRE_TRUE(dump != NULL, "render codegen dump to string");
    ASSERT_EQ_STR(expected, dump, "codegen dump string");

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


void test_codegen_dump_distinguishes_runtime_backed_operations(void) {
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
    CodegenProgram codegen_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse runtime-boundary program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for runtime-boundary program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check runtime-boundary program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for runtime-boundary program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program),
                 "lower MIR for runtime-boundary program");
    REQUIRE_TRUE(lir_build_program(&lir_program, &mir_program),
                 "lower LIR for runtime-boundary program");
    REQUIRE_TRUE(codegen_build_program(&codegen_program, &lir_program, target_get_default()),
                 "build codegen plan for runtime boundary program");

    dump = codegen_dump_program_to_string(&codegen_program);
    REQUIRE_TRUE(dump != NULL, "render runtime-boundary codegen dump to string");
    ASSERT_CONTAINS("Unit name=__mir$module_init kind=init", dump,
                    "module init survives into codegen plan");
    ASSERT_CONTAINS("call -> direct call-global", dump,
                    "global calls lower directly to machine patterns");
    ASSERT_CONTAINS("closure -> runtime __calynda_rt_closure_new", dump,
                    "closure construction stays runtime-backed");
    ASSERT_CONTAINS("array-literal -> runtime __calynda_rt_array_literal", dump,
                    "array literals stay runtime-backed");
    ASSERT_CONTAINS("template -> runtime __calynda_rt_template_build", dump,
                    "templates stay runtime-backed");
    ASSERT_CONTAINS("member -> runtime __calynda_rt_member_load", dump,
                    "member access stays runtime-backed");
    ASSERT_CONTAINS("index-load -> runtime __calynda_rt_index_load", dump,
                    "index loads stay runtime-backed");
    ASSERT_CONTAINS("store-index -> runtime __calynda_rt_store_index", dump,
                    "index stores stay runtime-backed");
    ASSERT_CONTAINS("call -> runtime __calynda_rt_call_callable", dump,
                    "closure or callable-object dispatch stays runtime-backed");
    ASSERT_CONTAINS("binary -> direct scalar-binary", dump,
                    "scalar arithmetic lowers directly");
    ASSERT_CONTAINS("branch -> direct branch", dump,
                    "control-flow branches lower directly");

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

