#include "hir.h"
#include "hir_dump.h"
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

void test_hir_dump_lowers_typed_program_to_normalized_blocks(void);
void test_hir_dump_normalizes_exit_and_keeps_external_callable_metadata(void);
void test_hir_dump_covers_templates_casts_arrays_assignments_and_nested_lambdas(void);
void test_hir_builder_rejects_programs_with_type_errors(void);
void test_hir_dump_lowers_v2_expressions(void);
void test_hir_dump_shows_export_and_static_flags(void);
void test_hir_dump_lowers_union_declarations(void);
void test_hir_dump_lowers_manual_block_with_memory_ops(void);


int main(void) {
    printf("Running HIR dump tests...\n\n");

    RUN_TEST(test_hir_dump_lowers_typed_program_to_normalized_blocks);
    RUN_TEST(test_hir_dump_normalizes_exit_and_keeps_external_callable_metadata);
    RUN_TEST(test_hir_dump_covers_templates_casts_arrays_assignments_and_nested_lambdas);
    RUN_TEST(test_hir_builder_rejects_programs_with_type_errors);
    RUN_TEST(test_hir_dump_lowers_v2_expressions);
    RUN_TEST(test_hir_dump_shows_export_and_static_flags);
    RUN_TEST(test_hir_dump_lowers_union_declarations);
    RUN_TEST(test_hir_dump_lowers_manual_block_with_memory_ops);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
