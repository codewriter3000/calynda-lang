#define _POSIX_C_SOURCE 200809L

#include "car.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
#define ASSERT_EQ_SIZE(expected, actual, msg) do {                          \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %zu, got %zu\n",     \
                __FILE__, __LINE__, (msg),                                  \
                (size_t)(expected), (size_t)(actual));                      \
    }                                                                       \
} while (0)
#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

void test_car_round_trip_memory(void);
void test_car_find_file(void);
void test_car_write_and_read_file(void);
void test_car_read_invalid_magic(void);
void test_car_read_truncated(void);
void test_car_read_bad_version(void);
void test_car_empty_archive(void);
void test_car_add_directory(void);
void test_car_many_files(void);
void test_car_build_reports_parse_span(void);


/* ================================================================
 * Main
 * ================================================================ */

int main(void) {
    printf("Running CAR archive tests...\n\n");

    RUN_TEST(test_car_round_trip_memory);
    RUN_TEST(test_car_find_file);
    RUN_TEST(test_car_write_and_read_file);
    RUN_TEST(test_car_read_invalid_magic);
    RUN_TEST(test_car_read_truncated);
    RUN_TEST(test_car_read_bad_version);
    RUN_TEST(test_car_empty_archive);
    RUN_TEST(test_car_add_directory);
    RUN_TEST(test_car_many_files);
    RUN_TEST(test_car_build_reports_parse_span);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
