#include "parser.h"
#include "type_resolution.h"

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
#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

void test_type_resolver_resolves_declared_types_and_casts(void);
void test_type_resolver_rejects_void_parameter(void);
void test_type_resolver_rejects_zero_sized_array(void);
void test_type_resolver_resolves_named_types(void);
void test_type_resolver_resolves_arr_wildcard(void);
void test_type_resolver_resolves_union_variant_payload(void);
void test_type_resolver_rejects_void_array_element(void);
void test_type_resolver_resolves_type_alias_and_threading_types(void);
void test_type_resolver_rejects_circular_type_alias(void);
void test_type_resolver_resolves_ptr_generic_type(void);


int main(void) {
    printf("Running type resolution tests...\n\n");

    RUN_TEST(test_type_resolver_resolves_declared_types_and_casts);
    RUN_TEST(test_type_resolver_rejects_void_parameter);
    RUN_TEST(test_type_resolver_rejects_zero_sized_array);
    RUN_TEST(test_type_resolver_resolves_named_types);
    RUN_TEST(test_type_resolver_resolves_arr_wildcard);
    RUN_TEST(test_type_resolver_resolves_union_variant_payload);
    RUN_TEST(test_type_resolver_rejects_void_array_element);
    RUN_TEST(test_type_resolver_resolves_type_alias_and_threading_types);
    RUN_TEST(test_type_resolver_rejects_circular_type_alias);
    RUN_TEST(test_type_resolver_resolves_ptr_generic_type);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
