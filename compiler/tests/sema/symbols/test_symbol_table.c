#include "parser.h"
#include "symbol_table.h"

#include <stdio.h>
#include <string.h>

int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

#define ASSERT_EQ_INT(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %d, got %d\n",      \
                __FILE__, __LINE__, (msg), (int)(expected), (int)(actual)); \
    }                                                                       \
} while (0)
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

void test_symbol_table_source_spans_and_unresolved_diagnostic(void);
void test_symbol_table_build_and_resolution(void);
void test_symbol_table_records_unresolved_identifier(void);
void test_symbol_table_duplicate_top_level_binding_error(void);
void test_symbol_table_duplicate_parameter_error(void);
void test_symbol_table_duplicate_local_error(void);
void test_symbol_table_duplicate_import_error(void);
void test_symbol_table_import_alias(void);
void test_symbol_table_import_selective(void);
void test_symbol_table_ambiguous_selective_import(void);
void test_symbol_table_export_static_flags(void);
void test_symbol_table_internal_flag(void);
void test_symbol_table_union_and_type_params(void);
void test_symbol_table_union_no_generics(void);


int main(void) {
    printf("Running symbol table tests...\n\n");

    RUN_TEST(test_symbol_table_build_and_resolution);
    RUN_TEST(test_symbol_table_source_spans_and_unresolved_diagnostic);
    RUN_TEST(test_symbol_table_records_unresolved_identifier);
    RUN_TEST(test_symbol_table_duplicate_top_level_binding_error);
    RUN_TEST(test_symbol_table_duplicate_parameter_error);
    RUN_TEST(test_symbol_table_duplicate_local_error);
    RUN_TEST(test_symbol_table_duplicate_import_error);
    RUN_TEST(test_symbol_table_import_alias);
    RUN_TEST(test_symbol_table_import_selective);
    RUN_TEST(test_symbol_table_ambiguous_selective_import);
    RUN_TEST(test_symbol_table_export_static_flags);
    RUN_TEST(test_symbol_table_internal_flag);
    RUN_TEST(test_symbol_table_union_and_type_params);
    RUN_TEST(test_symbol_table_union_no_generics);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
