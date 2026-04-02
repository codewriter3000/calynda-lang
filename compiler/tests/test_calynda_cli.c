#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

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

static bool write_temp_source(const char *source, char *path_buffer, size_t buffer_size) {
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

static bool make_temp_output_path(char *path_buffer, size_t buffer_size) {
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

static bool run_capture(const char *path,
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

static void test_calynda_cli_help_and_emitters(void) {
    static const char source[] =
        "start(string[] args) -> 7;\n";
    char source_path[64];
    char output[4096];
    const char *captured_output;
    char *help_argv[] = { (char *)"./build/calynda", (char *)"help", NULL };
    char *asm_argv[] = { (char *)"./build/calynda", (char *)"asm", source_path, NULL };
    char *bytecode_argv[] = { (char *)"./build/calynda", (char *)"bytecode", source_path, NULL };
    int exit_code;

    REQUIRE_TRUE(write_temp_source(source, source_path, sizeof(source_path)),
                 "write CLI test source file");

    REQUIRE_TRUE(run_capture("./build/calynda", help_argv, output, sizeof(output), &exit_code),
                 "run calynda help");
    captured_output = output;
    ASSERT_EQ_INT(0, exit_code, "calynda help exits successfully");
    ASSERT_CONTAINS("build <source.cal>", captured_output, "help text lists build command");

    REQUIRE_TRUE(run_capture("./build/calynda", asm_argv, output, sizeof(output), &exit_code),
                 "run calynda asm");
    captured_output = output;
    ASSERT_EQ_INT(0, exit_code, "calynda asm exits successfully");
    ASSERT_CONTAINS(".globl main", captured_output, "calynda asm emits executable assembly");

    REQUIRE_TRUE(run_capture("./build/calynda", bytecode_argv, output, sizeof(output), &exit_code),
                 "run calynda bytecode");
    captured_output = output;
    ASSERT_EQ_INT(0, exit_code, "calynda bytecode exits successfully");
    ASSERT_CONTAINS("BytecodeProgram target=portable-v1", captured_output, "calynda bytecode emits bytecode text");

    unlink(source_path);
}

static void test_calynda_cli_builds_native_executable(void) {
    static const char source[] =
        "start(string[] args) -> 7;\n";
    char source_path[64];
    char output_path[64];
    char output[4096];
    char *build_argv[] = {
        (char *)"./build/calynda",
        (char *)"build",
        source_path,
        (char *)"-o",
        output_path,
        NULL
    };
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(write_temp_source(source, source_path, sizeof(source_path)),
                 "write native CLI source file");
    REQUIRE_TRUE(make_temp_output_path(output_path, sizeof(output_path)),
                 "allocate native CLI output path");

    REQUIRE_TRUE(run_capture("./build/calynda", build_argv, output, sizeof(output), &exit_code),
                 "run calynda build");
    ASSERT_EQ_INT(0, exit_code, "calynda build exits successfully");

    REQUIRE_TRUE(run_capture(output_path, run_argv, output, sizeof(output), &exit_code),
                 "run generated CLI-built executable");
    ASSERT_EQ_INT(7, exit_code, "generated executable returns start result");

    unlink(source_path);
    unlink(output_path);
}

int main(void) {
    printf("Running calynda CLI tests...\n\n");

    RUN_TEST(test_calynda_cli_help_and_emitters);
    RUN_TEST(test_calynda_cli_builds_native_executable);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}