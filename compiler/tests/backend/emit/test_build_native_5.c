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
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {      \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\" in \"%s\"\n", \
                __FILE__, __LINE__, (msg), (needle),                        \
                (haystack) ? (haystack) : "(null)");                       \
    }                                                                       \
} while (0)

extern bool build_native_executable(const char *source, char *output_path,
                                    size_t output_path_size);
extern int run_process(const char *path, char *const argv[]);
extern bool run_process_capture_stdout(const char *path,
                                       char *const argv[],
                                       char *buffer,
                                       size_t buffer_size,
                                       int *exit_code);

static bool run_capture_combined(const char *path,
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
        dup2(pipe_fds[1], STDERR_FILENO);
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


/* ------------------------------------------------------------------ */
/*  G-NAT-2: Closure with double capture (nested captures)            */
/* ------------------------------------------------------------------ */

void test_build_native_double_capture_closure(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    var x = 10;\n"
        "    var inner = () -> {\n"
        "        var nested = () -> x;\n"
        "        return nested();\n"
        "    };\n"
        "    return inner();\n"
        "};\n";
    char output_path[64];
    char stdout_buf[256];
    int exit_code = -1;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build double capture closure program");
    char *argv[] = { output_path, NULL };
    REQUIRE_TRUE(run_process_capture_stdout(output_path, argv, stdout_buf,
                                            sizeof(stdout_buf), &exit_code),
                 "run double capture closure program");
    ASSERT_EQ_INT(10, exit_code, "double capture closure returns captured value");
    unlink(output_path);
}


/* ------------------------------------------------------------------ */
/*  G-NAT-1: All primitive types in E2E native build                  */
/* ------------------------------------------------------------------ */

void test_build_native_primitive_types(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32 a = 1;\n"
        "    int64 b = 2;\n"
        "    bool e = true;\n"
        "    return a + int32(b);\n"
        "};\n";
    char output_path[64];
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build primitive types program");
    char *argv[] = { output_path, NULL };
    exit_code = run_process(output_path, argv);
    ASSERT_EQ_INT(3, exit_code, "primitive types: 1 + int32(2) = 3");
    unlink(output_path);
}


/* ------------------------------------------------------------------ */
/*  G-NAT-7: Varargs passthrough E2E                                  */
/* ------------------------------------------------------------------ */

void test_build_native_varargs_passthrough(void) {
    static const char source[] =
        "int32 accept_varargs = (int32... nums) -> 42;\n"
        "start(string[] args) -> {\n"
        "    return accept_varargs(10, 20, 30);\n"
        "};\n";
    char output_path[64];
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build varargs program");
    char *argv[] = { output_path, NULL };
    exit_code = run_process(output_path, argv);
    ASSERT_EQ_INT(42, exit_code, "varargs callable call-site passes through");
    unlink(output_path);
}


/* ------------------------------------------------------------------ */
/*  G-NAT-5: Template literal with multiple interpolations E2E        */
/* ------------------------------------------------------------------ */

void test_build_native_template_literal(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    var x = 1;\n"
        "    var y = 2;\n"
        "    string s = `${x} and ${y}`;\n"
        "    return x + y;\n"
        "};\n";
    char output_path[64];
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build template literal program");
    char *argv[] = { output_path, NULL };
    exit_code = run_process(output_path, argv);
    ASSERT_EQ_INT(3, exit_code, "template literal program returns 3");
    unlink(output_path);
}


/* ------------------------------------------------------------------ */
/*  B5: manual checked out-of-bounds reports a runtime error exit      */
/* ------------------------------------------------------------------ */

void test_build_native_manual_checked_abort(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual checked {\n"
        "        ptr<int32> p = stackalloc(4);\n"
        "        store(p, 42);\n"
        "        ptr<int32> bad = offset(p, 100);\n"
        "        int32 val = deref(bad);\n"
        "        return val;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    char output_path[64];
    char output[1024];
    int exit_code = 0;

#include "test_build_native_5_p2.inc"
