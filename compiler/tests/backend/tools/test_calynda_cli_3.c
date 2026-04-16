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
#define ASSERT_EQ_INT(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %d, got %d\n",       \
                __FILE__, __LINE__, (msg), (expected), (actual));           \
    }                                                                       \
} while (0)

extern bool run_capture(const char *path,
                        char *const argv[],
                        char *buffer,
                        size_t buffer_size,
                        int *exit_code);

void test_calynda_cli_unknown_option_reports_usage(void);


/* ------------------------------------------------------------------ */
/*  G-CLI-3: Missing source file produces non-zero exit               */
/* ------------------------------------------------------------------ */

void test_calynda_cli_missing_source_file(void) {
    char output[4096];
    char *argv[] = {
        (char *)"./build/calynda",
        (char *)"asm",
        (char *)"/tmp/nonexistent_file_calynda_test.cal",
        NULL
    };
    int exit_code = 0;

    REQUIRE_TRUE(run_capture("./build/calynda", argv, output, sizeof(output), &exit_code),
                 "run calynda asm with missing file");
    ASSERT_TRUE(exit_code != 0,
                "missing source file produces non-zero exit code");
}


/* ------------------------------------------------------------------ */
/*  G-CLI-4: Malformed source produces non-zero exit                  */
/* ------------------------------------------------------------------ */

void test_calynda_cli_malformed_source(void) {
    static const char source[] = "this is not valid calynda {{{\n";
    char source_path[64];
    char output[4096];
    char *argv[] = {
        (char *)"./build/calynda",
        (char *)"asm",
        NULL, /* filled below */
        NULL
    };
    int exit_code = 0;
    int fd;

    snprintf(source_path, sizeof(source_path), "/tmp/calynda_cli_malformed_XXXXXX");
    fd = mkstemp(source_path);
    REQUIRE_TRUE(fd >= 0, "create temp file for malformed source");
    write(fd, source, strlen(source));
    close(fd);

    argv[2] = source_path;
    REQUIRE_TRUE(run_capture("./build/calynda", argv, output, sizeof(output), &exit_code),
                 "run calynda asm with malformed source");
    ASSERT_TRUE(exit_code != 0,
                "malformed source produces non-zero exit code");

    unlink(source_path);
}

void test_calynda_cli_unknown_option_reports_usage(void) {
    char output[4096];
    char *argv[] = {
        (char *)"./build/calynda",
        (char *)"asm",
        (char *)"--strict-race",
        (char *)"fake.cal",
        NULL
    };
    int exit_code = 0;

    REQUIRE_TRUE(run_capture("./build/calynda", argv, output, sizeof(output), &exit_code),
                 "run calynda asm with unknown option");
    ASSERT_TRUE(exit_code != 0,
                "unknown CLI option produces non-zero exit code");
}
