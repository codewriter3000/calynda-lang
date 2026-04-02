#include "../src/codegen.h"
#include "../src/hir.h"
#include "../src/lir.h"
#include "../src/machine.h"
#include "../src/mir.h"
#include "../src/parser.h"
#include "../src/runtime_abi.h"

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

static bool build_machine_dump_from_source(const char *source, char **dump_out) {
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    CodegenProgram codegen_program;
    MachineProgram machine_program;
    bool ok = false;

    if (!source || !dump_out) {
        return false;
    }

    *dump_out = NULL;
    memset(&ast_program, 0, sizeof(ast_program));
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    machine_program_init(&machine_program);
    parser_init(&parser, source);

    if (!parser_parse_program(&parser, &ast_program) ||
        !symbol_table_build(&symbols, &ast_program) ||
        !type_checker_check_program(&checker, &ast_program, &symbols) ||
        !hir_build_program(&hir_program, &ast_program, &symbols, &checker) ||
        !mir_build_program(&mir_program, &hir_program) ||
        !lir_build_program(&lir_program, &mir_program) ||
        !codegen_build_program(&codegen_program, &lir_program) ||
        !machine_build_program(&machine_program, &lir_program, &codegen_program)) {
        goto cleanup;
    }

    *dump_out = machine_dump_program_to_string(&machine_program);
    ok = *dump_out != NULL;

cleanup:
    machine_program_free(&machine_program);
    codegen_program_free(&codegen_program);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
    return ok;
}

static void test_runtime_abi_dump_defines_helper_surface(void) {
    static const char expected[] =
        "RuntimeAbi target=x86_64_sysv_elf env=r15 helper_args=[rdi, rsi, rdx, rcx, r8, r9]\n"
        "  entry captures=r15[value-word]\n"
        "  helper __calynda_rt_closure_new return=rax args=[rdi=unit_label, rsi=capture_count, rdx=capture_pack] pack=value-word\n"
        "  helper __calynda_rt_call_callable return=rax args=[rdi=callable, rsi=argument_count, rdx=argument_pack] pack=value-word\n"
        "  helper __calynda_rt_member_load return=rax args=[rdi=target_value, rsi=member_symbol]\n"
        "  helper __calynda_rt_index_load return=rax args=[rdi=target_value, rsi=index_value]\n"
        "  helper __calynda_rt_array_literal return=rax args=[rdi=element_count, rsi=element_pack] pack=value-word\n"
        "  helper __calynda_rt_template_build return=rax args=[rdi=part_count, rsi=part_pack] pack=template-part(tag,payload)\n"
        "  helper __calynda_rt_store_index return=void args=[rdi=target_value, rsi=index_value, rdx=store_value]\n"
        "  helper __calynda_rt_store_member return=void args=[rdi=target_value, rsi=member_symbol, rdx=store_value]\n"
        "  helper __calynda_rt_throw return=noreturn args=[rdi=throw_value]\n"
        "  helper __calynda_rt_cast_value return=rax args=[rdi=source_value, rsi=target_type_tag]\n";
    char *dump;

    dump = runtime_abi_dump_surface_to_string(CODEGEN_TARGET_X86_64_SYSV_ELF);
    REQUIRE_TRUE(dump != NULL, "render runtime ABI dump to string");
    ASSERT_EQ_STR(expected, dump, "runtime ABI dump string");
    free(dump);
}

static void test_machine_dump_emits_minimal_direct_instruction_stream(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    int32 total = add(1, 2);\n"
        "    return total;\n"
        "};\n";
    static const char expected[] =
        "MachineProgram target=x86_64_sysv_elf scratch=r14\n"
        "RuntimeAbi target=x86_64_sysv_elf env=r15 helper_args=[rdi, rsi, rdx, rcx, r8, r9]\n"
        "  entry captures=r15[value-word]\n"
        "  helper __calynda_rt_closure_new return=rax args=[rdi=unit_label, rsi=capture_count, rdx=capture_pack] pack=value-word\n"
        "  helper __calynda_rt_call_callable return=rax args=[rdi=callable, rsi=argument_count, rdx=argument_pack] pack=value-word\n"
        "  helper __calynda_rt_member_load return=rax args=[rdi=target_value, rsi=member_symbol]\n"
        "  helper __calynda_rt_index_load return=rax args=[rdi=target_value, rsi=index_value]\n"
        "  helper __calynda_rt_array_literal return=rax args=[rdi=element_count, rsi=element_pack] pack=value-word\n"
        "  helper __calynda_rt_template_build return=rax args=[rdi=part_count, rsi=part_pack] pack=template-part(tag,payload)\n"
        "  helper __calynda_rt_store_index return=void args=[rdi=target_value, rsi=index_value, rdx=store_value]\n"
        "  helper __calynda_rt_store_member return=void args=[rdi=target_value, rsi=member_symbol, rdx=store_value]\n"
        "  helper __calynda_rt_throw return=noreturn args=[rdi=throw_value]\n"
        "  helper __calynda_rt_cast_value return=rax args=[rdi=source_value, rsi=target_type_tag]\n"
        "  Unit name=add kind=binding return=int32 frame_slots=2 spills=0 helper_slots=0 outgoing_stack=0 blocks=1\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        push rbp\n"
        "        mov rbp, rsp\n"
        "        push r14\n"
        "        sub rsp, 16\n"
        "        mov frame(left), rdi\n"
        "        mov frame(right), rsi\n"
        "        mov r10, frame(left)\n"
        "        add r10, frame(right)\n"
        "        mov rax, r10\n"
        "        add rsp, 16\n"
        "        pop r14\n"
        "        pop rbp\n"
        "        ret\n"
        "  Unit name=start kind=start return=int32 frame_slots=2 spills=0 helper_slots=0 outgoing_stack=0 blocks=1\n"
        "    Blocks:\n"
        "      Block bb0:\n"
        "        push rbp\n"
        "        mov rbp, rsp\n"
        "        push r14\n"
        "        sub rsp, 16\n"
        "        mov frame(args), rdi\n"
        "        mov rdi, int32(1)\n"
        "        mov rsi, int32(2)\n"
        "        push r10\n"
        "        call add\n"
        "        pop r10\n"
        "        mov r10, rax\n"
        "        mov frame(total), r10\n"
        "        mov rax, frame(total)\n"
        "        add rsp, 16\n"
        "        pop r14\n"
        "        pop rbp\n"
        "        ret\n";
    char *dump;

    REQUIRE_TRUE(build_machine_dump_from_source(source, &dump), "emit minimal machine program");
    ASSERT_EQ_STR(expected, dump, "machine dump string");
    free(dump);
}

static void test_machine_dump_routes_runtime_backed_ops_through_helpers(void) {
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
    char *dump;

    REQUIRE_TRUE(build_machine_dump_from_source(source, &dump), "emit runtime-backed machine program");
    ASSERT_CONTAINS("Unit name=start kind=start return=int32 frame_slots=6 spills=6 helper_slots=3",
                    dump,
                    "start unit reserves helper slots for runtime pack setup");
    ASSERT_CONTAINS("mov helper(0), global(seed)", dump,
                    "array literal elements stage through helper scratch slots");
    ASSERT_CONTAINS("call __calynda_rt_array_literal", dump,
                    "array literals lower through the runtime ABI");
    ASSERT_CONTAINS("mov helper(0), tag(text)", dump,
                    "template lowering writes tagged runtime template parts");
    ASSERT_CONTAINS("call __calynda_rt_template_build", dump,
                    "template building lowers through the runtime ABI");
    ASSERT_CONTAINS("mov rdi, global(stdlib)", dump,
                    "member load helper receives the target value in rdi");
    ASSERT_CONTAINS("mov rsi, symbol(print)", dump,
                    "member load helper receives the member symbol in rsi");
    ASSERT_CONTAINS("call __calynda_rt_member_load", dump,
                    "member access lowers through the runtime ABI");
    ASSERT_CONTAINS("mov helper(0), int32(2)", dump,
                    "callable dispatch stages arguments through helper scratch slots");
    ASSERT_CONTAINS("call __calynda_rt_call_callable", dump,
                    "callable-object dispatch lowers through the runtime ABI");
    ASSERT_CONTAINS("call __calynda_rt_store_index", dump,
                    "index writes lower through the runtime ABI");
    ASSERT_CONTAINS("mov frame(values), r10", dump,
                    "runtime helper results can still flow back into local frame slots");
    free(dump);
}

static void test_machine_dump_routes_throw_terminator_to_runtime_helper(void) {
    static const char source[] =
        "int32 fail = () -> {\n"
        "    throw \"boom\";\n"
        "    return 0;\n"
        "};\n"
        "start(string[] args) -> fail();\n";
    char *dump;

    REQUIRE_TRUE(build_machine_dump_from_source(source, &dump), "emit throw machine program");
    ASSERT_CONTAINS("mov rdi, string(\"boom\")", dump,
                    "throw passes the thrown value in rdi");
    ASSERT_CONTAINS("call __calynda_rt_throw", dump,
                    "throw terminators lower through the throw runtime helper");
    free(dump);
}

int main(void) {
    printf("Running machine dump tests...\n\n");

    RUN_TEST(test_runtime_abi_dump_defines_helper_surface);
    RUN_TEST(test_machine_dump_emits_minimal_direct_instruction_stream);
    RUN_TEST(test_machine_dump_routes_runtime_backed_ops_through_helpers);
    RUN_TEST(test_machine_dump_routes_throw_terminator_to_runtime_helper);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}