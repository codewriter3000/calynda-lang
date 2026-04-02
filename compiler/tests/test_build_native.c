#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

static bool write_temp_source(const char *source, char *path_buffer, size_t buffer_size) {
    char template_path[] = "/tmp/calynda-source-XXXXXX";
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
    char template_path[] = "/tmp/calynda-exe-XXXXXX";
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

static int run_process(const char *path, char *const argv[]) {
    pid_t child;
    int status;

    child = fork();
    if (child < 0) {
        return -1;
    }

    if (child == 0) {
        execv(path, argv);
        _exit(127);
    }

    if (waitpid(child, &status, 0) < 0) {
        return -1;
    }
    if (!WIFEXITED(status)) {
        return -1;
    }
    return WEXITSTATUS(status);
}

static bool run_process_capture_stdout(const char *path,
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

static bool build_native_executable(const char *source, char *output_path, size_t output_path_size) {
    char source_path[64];
    char *build_argv[] = {
        (char *)"./build/build_native",
        source_path,
        output_path,
        NULL
    };
    int exit_code;

    if (!write_temp_source(source, source_path, sizeof(source_path)) ||
        !make_temp_output_path(output_path, output_path_size)) {
        unlink(source_path);
        return false;
    }

    exit_code = run_process("./build/build_native", build_argv);
    unlink(source_path);
    return exit_code == 0;
}

static void test_build_native_runs_simple_start_program(void) {
    static const char source[] =
        "start(string[] args) -> 7;\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build simple native executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(7, exit_code, "native executable returns the start result");
    unlink(output_path);
}

static void test_build_native_handles_direct_eight_argument_calls(void) {
    static const char source[] =
        "int32 add8 = (int32 a, int32 b, int32 c, int32 d, int32 e, int32 f, int32 g, int32 h) -> a + b + c + d + e + f + g + h;\n"
        "start(string[] args) -> add8(1, 2, 3, 4, 5, 6, 7, 8);\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build direct-call native executable with stack arguments");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(36, exit_code, "native executable preserves direct-call stack arguments end to end");
    unlink(output_path);
}

static void test_build_native_runs_runtime_lambda_program(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "start(string[] args) -> {\n"
        "    var render = (string name) -> `hello ${name}`;\n"
        "    stdlib.print(render(args[0]));\n"
        "    return 9;\n"
        "};\n";
    char output_path[64];
    char stdout_buffer[128];
    char *run_argv[] = { output_path, (char *)"world", NULL };
    const char *captured_stdout;
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build runtime-backed native executable");
    REQUIRE_TRUE(run_process_capture_stdout(output_path,
                                            run_argv,
                                            stdout_buffer,
                                            sizeof(stdout_buffer),
                                            &exit_code),
                 "run runtime-backed native executable and capture stdout");
    captured_stdout = stdout_buffer;
    ASSERT_EQ_INT(9, exit_code, "runtime-backed native executable returns the start result");
    ASSERT_EQ_STR("hello world\n",
                  captured_stdout,
                  "runtime-backed native executable boxes args and dispatches callable lambdas");
    unlink(output_path);
}

static void test_build_native_auto_calls_zero_arg_template_callable(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "start(string[] args) -> {\n"
        "    stdlib.print(`They are printing: ${() -> \"Hello World\"}`);\n"
        "    return 0;\n"
        "};\n";
    char output_path[64];
    char stdout_buffer[128];
    char *run_argv[] = { output_path, NULL };
    const char *captured_stdout;
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build template auto-call native executable");
    REQUIRE_TRUE(run_process_capture_stdout(output_path,
                                            run_argv,
                                            stdout_buffer,
                                            sizeof(stdout_buffer),
                                            &exit_code),
                 "run template auto-call native executable and capture stdout");
    ASSERT_EQ_INT(0, exit_code, "template auto-call native executable returns the start result");
    captured_stdout = stdout_buffer;
    ASSERT_EQ_STR("They are printing: Hello World\n",
                  captured_stdout,
                  "template interpolation auto-calls zero-argument callables");
    unlink(output_path);
}

int main(void) {
    printf("Running native build tests...\n\n");

    RUN_TEST(test_build_native_runs_simple_start_program);
    RUN_TEST(test_build_native_handles_direct_eight_argument_calls);
    RUN_TEST(test_build_native_runs_runtime_lambda_program);
    RUN_TEST(test_build_native_auto_calls_zero_arg_template_callable);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}