#include "hir.h"
#include "mir.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

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

void test_mir_dump_lowers_minimal_callable_slice(void);
void test_mir_dump_lowers_ternary_into_branching_blocks(void);
void test_mir_dump_lowers_short_circuit_logical_operators(void);
void test_mir_dump_lowers_arrays_assignments_members_and_templates(void);
void test_mir_dump_lowers_callable_local_bindings_and_nested_closures(void);
void test_mir_dump_lowers_top_level_value_bindings_into_module_init(void);
void test_mir_dump_lowers_throw_as_terminator(void);
void test_mir_dump_lowers_union_new_instructions(void);
void test_mir_dump_lowers_hetero_array_literal(void);
void test_mir_dump_checked_manual_block_uses_bc_functions(void);
void test_mir_dump_ptr_checked_type_uses_bc_deref(void);
void test_mir_dump_plain_manual_block_uses_plain_functions(void);
void test_mir_dump_cleanup_runs_before_manual_return(void);
void test_mir_dump_stackalloc_auto_registers_free(void);
void test_mir_dump_lowers_union_tag_and_payload_access(void);
void test_mir_dump_valid_program_has_no_error(void);
void test_mir_dump_double_capture_closure(void);
void test_mir_dump_error_api_format_null_is_safe(void);
void test_mir_build_rejects_programs_with_hir_errors(void);


int main(void) {
    printf("Running MIR dump tests...\n\n");

    RUN_TEST(test_mir_dump_lowers_minimal_callable_slice);
    RUN_TEST(test_mir_dump_lowers_ternary_into_branching_blocks);
    RUN_TEST(test_mir_dump_lowers_short_circuit_logical_operators);
    RUN_TEST(test_mir_dump_lowers_arrays_assignments_members_and_templates);
    RUN_TEST(test_mir_dump_lowers_callable_local_bindings_and_nested_closures);
    RUN_TEST(test_mir_dump_lowers_top_level_value_bindings_into_module_init);
    RUN_TEST(test_mir_dump_lowers_throw_as_terminator);
    RUN_TEST(test_mir_dump_lowers_union_new_instructions);
    RUN_TEST(test_mir_dump_lowers_hetero_array_literal);
    RUN_TEST(test_mir_dump_checked_manual_block_uses_bc_functions);
    RUN_TEST(test_mir_dump_ptr_checked_type_uses_bc_deref);
    RUN_TEST(test_mir_dump_plain_manual_block_uses_plain_functions);
    RUN_TEST(test_mir_dump_cleanup_runs_before_manual_return);
    RUN_TEST(test_mir_dump_stackalloc_auto_registers_free);
    RUN_TEST(test_mir_dump_lowers_union_tag_and_payload_access);
    RUN_TEST(test_mir_dump_valid_program_has_no_error);
    RUN_TEST(test_mir_dump_double_capture_closure);
    RUN_TEST(test_mir_dump_error_api_format_null_is_safe);
    RUN_TEST(test_mir_build_rejects_programs_with_hir_errors);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
