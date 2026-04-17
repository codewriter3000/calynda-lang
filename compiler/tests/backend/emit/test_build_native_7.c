#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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

extern bool build_native_executable(const char *source, char *output_path,
                                    size_t output_path_size);
extern int run_process(const char *path, char *const argv[]);

void test_build_native_runs_deep_self_tail_recursion_without_stack_growth(void) {
    static const char source[] =
        "int32 count_down = (int32 value) -> value <= 0 ? 0 : count_down(value - 1);\n"
        "start(string[] args) -> {\n"
        "    return count_down(10000000);\n"
        "};\n";
    char output_path[64];
    char *argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build deep self-tail-recursive executable");
    exit_code = run_process(output_path, argv);
    ASSERT_TRUE(exit_code == 0,
                "deep self-tail-recursive program completes without stack overflow");
    unlink(output_path);
}

void test_build_native_dispatches_overloads_with_widening(void) {
    static const char source[] =
        "int32 pick = (int32 value) -> 40 + value;\n"
        "int32 pick = (string value) -> 7;\n"
        "start(string[] args) -> {\n"
        "    return pick(int16(2));\n"
        "};\n";
    char output_path[64];
    char *argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build overload dispatch executable");
    exit_code = run_process(output_path, argv);
    ASSERT_TRUE(exit_code == 42,
                "native executable dispatches widening overload to int32 implementation");
    unlink(output_path);
}
