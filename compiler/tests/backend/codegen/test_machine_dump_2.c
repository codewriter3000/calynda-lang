#include "codegen.h"
#include "hir.h"
#include "lir.h"
#include "machine.h"
#include "mir.h"
#include "parser.h"
#include "runtime_abi.h"
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

bool build_machine_dump_from_source(const char *source, char **dump_out);


void test_runtime_abi_dump_defines_helper_surface(void) {
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
        "  helper __calynda_rt_cast_value return=rax args=[rdi=source_value, rsi=target_type_tag]\n"
        "  helper __calynda_rt_union_new return=rax args=[rdi=type_descriptor, rsi=variant_tag, rdx=payload_value]\n"
        "  helper __calynda_rt_union_get_tag return=rax args=[rdi=target_value]\n"
        "  helper __calynda_rt_union_get_payload return=rax args=[rdi=target_value]\n"
        "  helper __calynda_rt_hetero_array_new return=rax args=[rdi=type_descriptor, rsi=element_count, rdx=element_pack] pack=value-word\n"
        "  helper __calynda_rt_hetero_array_get_tag return=rax args=[rdi=target_value, rsi=index_value]\n"
        "  helper __calynda_rt_thread_spawn return=rax args=[rdi=callable]\n"
        "  helper __calynda_rt_thread_join return=void args=[rdi=thread_value]\n"
        "  helper __calynda_rt_mutex_new return=rax args=[]\n"
        "  helper __calynda_rt_mutex_lock return=void args=[rdi=mutex_value]\n"
        "  helper __calynda_rt_mutex_unlock return=void args=[rdi=mutex_value]\n"
        "  helper __calynda_rt_thread_cancel return=void args=[rdi=thread_value]\n"
        "  helper __calynda_rt_future_spawn return=rax args=[rdi=callable]\n"
        "  helper __calynda_rt_future_get return=rax args=[rdi=future_value]\n"
        "  helper __calynda_rt_future_cancel return=void args=[rdi=future_value]\n"
        "  helper __calynda_rt_atomic_new return=rax args=[rdi=atomic_new_value]\n"
        "  helper __calynda_rt_atomic_load return=rax args=[rdi=atomic_value]\n"
        "  helper __calynda_rt_atomic_store return=void args=[rdi=atomic_value, rsi=atomic_new_value]\n"
        "  helper __calynda_rt_atomic_exchange return=rax args=[rdi=atomic_value, rsi=atomic_new_value]\n"
        "  helper __calynda_typeof return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_isint return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_isfloat return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_isbool return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_isstring return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_isarray return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_issametype return=rax args=[rdi=left_value, rsi=left_type_text, rdx=right_value, rcx=right_type_text]\n";
    char *dump;

    dump = runtime_abi_dump_surface_to_string(CODEGEN_TARGET_X86_64_SYSV_ELF);
    REQUIRE_TRUE(dump != NULL, "render runtime ABI dump to string");
    ASSERT_EQ_STR(expected, dump, "runtime ABI dump string");
    free(dump);
}


void test_machine_dump_emits_minimal_direct_instruction_stream(void) {
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
        "  helper __calynda_rt_union_new return=rax args=[rdi=type_descriptor, rsi=variant_tag, rdx=payload_value]\n"
        "  helper __calynda_rt_union_get_tag return=rax args=[rdi=target_value]\n"
        "  helper __calynda_rt_union_get_payload return=rax args=[rdi=target_value]\n"
        "  helper __calynda_rt_hetero_array_new return=rax args=[rdi=type_descriptor, rsi=element_count, rdx=element_pack] pack=value-word\n"
        "  helper __calynda_rt_hetero_array_get_tag return=rax args=[rdi=target_value, rsi=index_value]\n"
        "  helper __calynda_rt_thread_spawn return=rax args=[rdi=callable]\n"
        "  helper __calynda_rt_thread_join return=void args=[rdi=thread_value]\n"
        "  helper __calynda_rt_mutex_new return=rax args=[]\n"
        "  helper __calynda_rt_mutex_lock return=void args=[rdi=mutex_value]\n"
        "  helper __calynda_rt_mutex_unlock return=void args=[rdi=mutex_value]\n"
        "  helper __calynda_rt_thread_cancel return=void args=[rdi=thread_value]\n"
        "  helper __calynda_rt_future_spawn return=rax args=[rdi=callable]\n"
        "  helper __calynda_rt_future_get return=rax args=[rdi=future_value]\n"
        "  helper __calynda_rt_future_cancel return=void args=[rdi=future_value]\n"
        "  helper __calynda_rt_atomic_new return=rax args=[rdi=atomic_new_value]\n"
        "  helper __calynda_rt_atomic_load return=rax args=[rdi=atomic_value]\n"
        "  helper __calynda_rt_atomic_store return=void args=[rdi=atomic_value, rsi=atomic_new_value]\n"
        "  helper __calynda_rt_atomic_exchange return=rax args=[rdi=atomic_value, rsi=atomic_new_value]\n"
        "  helper __calynda_typeof return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_isint return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_isfloat return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_isbool return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_isstring return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_isarray return=rax args=[rdi=source_value, rsi=type_text]\n"
        "  helper __calynda_issametype return=rax args=[rdi=left_value, rsi=left_type_text, rdx=right_value, rcx=right_type_text]\n"
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


void test_machine_dump_routes_runtime_backed_ops_through_helpers(void) {
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


void test_machine_dump_routes_throw_terminator_to_runtime_helper(void) {
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
