#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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

bool write_temp_source(const char *source, char *path_buffer, size_t buffer_size) {
    char template_path[] = "/tmp/calynda-cli-XXXXXX";
    FILE *file;
    int fd;

    if (!source || !path_buffer || buffer_size == 0) {
        return false;
    }

    fd = mkstemp(template_path);
    if (fd < 0) {
        return false;
    }

    file = fdopen(fd, "wb");
    if (!file) {
        close(fd);
        unlink(template_path);
        return false;
    }
    if (fputs(source, file) == EOF || fclose(file) != 0) {
        unlink(template_path);
        return false;
    }

    if (strlen(template_path) + 1 > buffer_size) {
        unlink(template_path);
        return false;
    }

    memcpy(path_buffer, template_path, strlen(template_path) + 1);
    return true;
}

bool make_temp_output_path(char *path_buffer, size_t buffer_size) {
    char template_path[] = "/tmp/calynda-cli-exe-XXXXXX";
    int fd;

    if (!path_buffer || buffer_size == 0) {
        return false;
    }

    fd = mkstemp(template_path);
    if (fd < 0) {
        return false;
    }
    close(fd);
    unlink(template_path);

    if (strlen(template_path) + 1 > buffer_size) {
        return false;
    }
    memcpy(path_buffer, template_path, strlen(template_path) + 1);
    return true;
}

bool run_capture(const char *path,
                        char *const argv[],
                        char *buffer,
                        size_t buffer_size,
                        int *exit_code) {
    int pipe_fds[2];
    pid_t child;
    int status;
    size_t length = 0;

    if (!path || !argv || !buffer || buffer_size == 0 || !exit_code) {
        return false;
    }

    buffer[0] = '\0';
    if (pipe(pipe_fds) != 0) {
        return false;
    }

    child = fork();
    if (child < 0) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return false;
    }

    if (child == 0) {
        close(pipe_fds[0]);
        dup2(pipe_fds[1], STDOUT_FILENO);
        close(pipe_fds[1]);
        execv(path, argv);
        _exit(127);
    }

    close(pipe_fds[1]);
    while (length + 1 < buffer_size) {
        ssize_t read_size = read(pipe_fds[0], buffer + length, buffer_size - length - 1);

        if (read_size <= 0) {
            break;
        }
        length += (size_t)read_size;
    }
    buffer[length] = '\0';
    close(pipe_fds[0]);

    if (waitpid(child, &status, 0) < 0 || !WIFEXITED(status)) {
        return false;
    }
    *exit_code = WEXITSTATUS(status);
    return true;
}

void test_calynda_cli_help_and_emitters(void);
void test_calynda_cli_builds_native_executable(void);
void test_calynda_cli_asm_accepts_car_archive(void);
void test_calynda_cli_missing_source_file(void);
void test_calynda_cli_malformed_source(void);
void test_calynda_cli_unknown_option_reports_usage(void);
void test_calynda_cli_malformed_source_reports_span(void);
void test_calynda_cli_run_propagates_runtime_error_exit(void);


int main(void) {
    printf("Running calynda CLI tests...\n\n");

    RUN_TEST(test_calynda_cli_help_and_emitters);
    RUN_TEST(test_calynda_cli_builds_native_executable);
    RUN_TEST(test_calynda_cli_asm_accepts_car_archive);
    RUN_TEST(test_calynda_cli_missing_source_file);
    RUN_TEST(test_calynda_cli_malformed_source);
    RUN_TEST(test_calynda_cli_unknown_option_reports_usage);
    RUN_TEST(test_calynda_cli_malformed_source_reports_span);
    RUN_TEST(test_calynda_cli_run_propagates_runtime_error_exit);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
