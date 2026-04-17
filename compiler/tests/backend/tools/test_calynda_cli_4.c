#define _POSIX_C_SOURCE 200809L

#include "runtime.h"

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
#define ASSERT_CONTAINS(needle, haystack, msg) do {                         \
    tests_run++;                                                            \
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {       \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\" in \"%s\"\n", \
                __FILE__, __LINE__, (msg), (needle),                        \
                (haystack) ? (haystack) : "(null)");                        \
    }                                                                       \
} while (0)

static bool run_capture_combined(const char *path, char *const argv[],
                                 char *buffer, size_t buffer_size,
                                 int *exit_code) {
    int pipe_fds[2], status;
    pid_t child;
    size_t length = 0;
    if (!path || !argv || !buffer || buffer_size == 0 || !exit_code || pipe(pipe_fds) != 0) return false;
    buffer[0] = '\0';
    child = fork();
    if (child < 0) return close(pipe_fds[0]), close(pipe_fds[1]), false;
    if (child == 0) {
        close(pipe_fds[0]);
        dup2(pipe_fds[1], STDOUT_FILENO);
        dup2(pipe_fds[1], STDERR_FILENO);
        close(pipe_fds[1]);
        execv(path, argv);
        _exit(127);
    }
    close(pipe_fds[1]);
    while (length + 1 < buffer_size) {
        ssize_t read_size = read(pipe_fds[0], buffer + length, buffer_size - length - 1);
        if (read_size <= 0) break;
        length += (size_t) read_size;
    }
    buffer[length] = '\0';
    close(pipe_fds[0]);
    if (waitpid(child, &status, 0) < 0 || !WIFEXITED(status)) return false;
    *exit_code = WEXITSTATUS(status);
    return true;
}

extern bool write_temp_source(const char *source, char *path_buffer, size_t buffer_size);

void test_calynda_cli_malformed_source_reports_span(void) {
    static const char source[] = "start(string[] args) -> {\n    @\n};\n";
    char source_path[128];
    char output[4096];
    FILE *file;
    int exit_code = 0;
    char *argv[] = { (char *) "./build/calynda", (char *) "asm", source_path, NULL };

    snprintf(source_path, sizeof(source_path), "build/test-calynda-cli-malformed-%ld.cal", (long) getpid());
    file = fopen(source_path, "wb");
    REQUIRE_TRUE(file != NULL, "create malformed CLI source");
    REQUIRE_TRUE(fputs(source, file) != EOF && fclose(file) == 0, "write malformed CLI source");
    REQUIRE_TRUE(run_capture_combined("./build/calynda", argv, output, sizeof(output), &exit_code),
                 "run calynda asm with malformed source span capture");
    ASSERT_TRUE(exit_code != 0, "malformed CLI source exits non-zero");
    ASSERT_CONTAINS(source_path, output, "CLI diagnostic includes source path");
    ASSERT_CONTAINS(":2:5: parse error:", output, "CLI diagnostic includes line and column");
    ASSERT_CONTAINS("Unexpected character", output, "CLI diagnostic includes parse message");
    unlink(source_path);
}

void test_calynda_cli_run_propagates_runtime_error_exit(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual checked {\n"
        "        ptr<int32> p = stackalloc(4);\n"
        "        store(p, 42);\n"
        "        ptr<int32> bad = offset(p, 100);\n"
        "        return deref(bad);\n"
        "    };\n"
        "};\n";
    char source_path[128];
    char output[4096];
    int exit_code = 0;
    char *argv[] = {
        (char *)"./build/calynda",
        (char *)"run",
        source_path,
        NULL
    };

    REQUIRE_TRUE(write_temp_source(source, source_path, sizeof(source_path)),
                 "write runtime failure CLI source");
    REQUIRE_TRUE(run_capture_combined("./build/calynda", argv, output, sizeof(output), &exit_code),
                 "run calynda with runtime failure program");
    ASSERT_TRUE(exit_code == CALYNDA_RT_EXIT_RUNTIME_ERROR,
                "runtime failure propagates as a normal runtime error exit code");
    ASSERT_CONTAINS("calynda bounds-check: out-of-bounds access",
                    output,
                    "runtime failure is reported to the CLI user");
    unlink(source_path);
}
