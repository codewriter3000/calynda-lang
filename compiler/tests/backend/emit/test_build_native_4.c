#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

#define ASSERT_EQ_INT(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %d, got %d\n",       \
                __FILE__, __LINE__, (msg), (int)(expected), (int)(actual)); \
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

extern bool build_native_executable(const char *source, char *output_path,
                                    size_t output_path_size);
extern int run_process(const char *path, char *const argv[]);

void test_build_native_compares_hetero_arrays_by_identity(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    arr<?> first = [1, true];\n"
        "    arr<?> alias = first;\n"
        "    arr<?> second = [1, true];\n"
        "    return first == alias\n"
        "        ? (first != second\n"
        "            ? (int32(first[0]) == 1\n"
        "                ? (bool(first[1]) == true ? 0 : 4)\n"
        "                : 3)\n"
        "            : 2)\n"
        "        : 1;\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build hetero array identity executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(0,
                  exit_code,
                  "hetero arrays compare by identity and indexed external values compare after casts");
    unlink(output_path);
}