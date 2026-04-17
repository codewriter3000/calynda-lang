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
extern const char *test_binary_path;

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

extern bool build_native_executable(const char *source, char *output_path,
                                    size_t output_path_size);
extern int run_process(const char *path, char *const argv[]);

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

static bool resolve_build_native_binary(char *buffer, size_t buffer_size) {
    const char *path = test_binary_path;
    char *slash;
    size_t dir_len;
    if (!path || !buffer) return false;
    slash = strrchr(path, '/');
    if (!slash) return false;
    dir_len = (size_t) (slash - path + 1);
    if (dir_len + sizeof("build_native") > buffer_size) return false;
    memcpy(buffer, path, dir_len);
    memcpy(buffer + dir_len, "build_native", sizeof("build_native"));
    return true;
}

void test_build_native_malformed_source_reports_span(void) {
    static const char source[] = "start(string[] args) -> {\n    @\n};\n";
    char build_native_path[128];
    char source_path[128];
    char output_path[128];
    char output[4096];
    const char *captured_output = NULL;
    FILE *file;
    int exit_code = 0;
    char *argv[] = { build_native_path, source_path, output_path, NULL };

    REQUIRE_TRUE(resolve_build_native_binary(build_native_path, sizeof(build_native_path)),
                 "resolve build_native path relative to test binary");
    snprintf(source_path, sizeof(source_path), "build/test-build-native-malformed-%ld.cal", (long) getpid());
    snprintf(output_path, sizeof(output_path), "build/test-build-native-malformed-%ld.out", (long) getpid());
    file = fopen(source_path, "wb");
    REQUIRE_TRUE(file != NULL, "create malformed native-build source");
    REQUIRE_TRUE(fputs(source, file) != EOF && fclose(file) == 0, "write malformed native-build source");
    REQUIRE_TRUE(run_capture_combined(build_native_path, argv, output, sizeof(output), &exit_code),
                 "run build_native with malformed source span capture");
    ASSERT_TRUE(exit_code != 0, "malformed native-build source exits non-zero");
    captured_output = output;
    ASSERT_CONTAINS(source_path, captured_output, "native-build diagnostic includes source path");
    ASSERT_CONTAINS(":2:5: parse error:", captured_output, "native-build diagnostic includes line and column");
    ASSERT_CONTAINS("Unexpected character", captured_output, "native-build diagnostic includes parse message");
    unlink(source_path);
    unlink(output_path);
}

void test_build_native_runs_recursive_top_level_lambda_program(void) {
    static const char source[] =
        "int32 sum_down = (int32 value) -> value <= 0 ? 0 : value + sum_down(value - 1);\n"
        "start(string[] args) -> {\n"
        "    return sum_down(4);\n"
        "};\n";
    char output_path[64];
    char *argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build recursive top-level lambda executable");
    exit_code = run_process(output_path, argv);
    ASSERT_TRUE(exit_code == 10,
                "recursive top-level lambda program builds and returns the recursive result");
    unlink(output_path);
}
