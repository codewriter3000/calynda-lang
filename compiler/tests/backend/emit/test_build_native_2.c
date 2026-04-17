#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

bool write_temp_source(const char *source, char *path_buffer, size_t buffer_size);
bool make_temp_output_path(char *path_buffer, size_t buffer_size);
int run_process(const char *path, char *const argv[]);
bool run_process_capture_stdout(const char *path,
                                       char *const argv[],
                                       char *buffer,
                                       size_t buffer_size,
                                       int *exit_code);
bool build_native_executable(const char *source, char *output_path, size_t output_path_size);


void test_build_native_runs_simple_start_program(void) {
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


void test_build_native_handles_direct_eight_argument_calls(void) {
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


void test_build_native_runs_runtime_lambda_program(void) {
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


void test_build_native_auto_calls_zero_arg_template_callable(void) {
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


void test_build_native_runs_boot_program(void) {
    static const char source[] =
        "boot -> 13;\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build boot native executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(13, exit_code, "boot native executable returns the boot result");
    unlink(output_path);
}


void test_build_native_runs_boot_program_with_block(void) {
    static const char source[] =
        "boot -> {\n"
        "    int32 x = 3;\n"
        "    int32 y = 4;\n"
        "    return x + y;\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build boot block native executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(7, exit_code, "boot block native executable returns computed result");
    unlink(output_path);
}

void test_build_native_runs_void_start_program(void) {
    static const char source[] =
        "start -> {\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build void start native executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(0, exit_code, "void start native executable returns zero");
    unlink(output_path);
}


void test_build_native_runs_manual_malloc_free(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 mem = malloc(64);\n"
        "        free(mem);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build manual malloc/free native executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(0, exit_code, "manual malloc/free native executable returns 0");
    unlink(output_path);
}


void test_build_native_typed_ptr_offset_stride(void) {
    /* ptr<int32> offset must advance by 4, not 8, so p[2] == 99 */
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        ptr<int32> p = malloc(12);\n"
        "        store(p, 0);\n"
        "        ptr<int32> q = offset(p, 1);\n"
        "        store(q, 0);\n"
        "        ptr<int32> r = offset(p, 2);\n"
        "        store(r, 99);\n"
        "        int64 val = deref(r);\n"
        "        free(p);\n"
        "        return val;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build typed ptr offset stride executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(99, exit_code, "typed ptr<int32> offset uses 4-byte stride");
    unlink(output_path);
}


void test_build_native_supports_string_index_and_length(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    return int32(args.length) + int32(args[0].length) + int32(args[0][0]);\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, (char *)"A", NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build string index/length executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(67, exit_code, "native executable supports array length, string length, and string indexing");
    unlink(output_path);
}
