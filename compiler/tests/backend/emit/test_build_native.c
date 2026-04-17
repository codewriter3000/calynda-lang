#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;
const char *test_binary_path = NULL;

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

bool write_temp_source(const char *source, char *path_buffer, size_t buffer_size) {
    char template_path[] = "/tmp/calynda-source-XXXXXX";
    FILE *file;
    int fd;
    if (!source || !path_buffer || buffer_size == 0) {
        return false;
    }
    fd = mkstemp(template_path);
    if (fd < 0) return false;
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
    char template_path[] = "/tmp/calynda-exe-XXXXXX";
    int fd;
    if (!path_buffer || buffer_size == 0) {
        return false;
    }
    fd = mkstemp(template_path);
    if (fd < 0) return false;
    close(fd); unlink(template_path);
    if (strlen(template_path) + 1 > buffer_size) {
        return false;
    }
    memcpy(path_buffer, template_path, strlen(template_path) + 1);
    return true;
}

int run_process(const char *path, char *const argv[]) {
    pid_t child;
    int status;
    child = fork();
    if (child < 0) return -1;
    if (child == 0) {
        execv(path, argv);
        _exit(127);
    }
    if (waitpid(child, &status, 0) < 0) {
        return -1;
    }
    if (!WIFEXITED(status)) return -1;
    return WEXITSTATUS(status);
}

bool run_process_capture_stdout(const char *path,
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

bool build_native_executable(const char *source, char *output_path, size_t output_path_size) {
    const char *build_tool = "./build/build_native";
    char build_path[128];
    char source_path[64];
    char *build_argv[] = { NULL, source_path, output_path, NULL };
    int exit_code;
    if (test_binary_path) {
        char *slash = strrchr(test_binary_path, '/');
        if (slash) {
            size_t dir_len = (size_t)(slash - test_binary_path + 1);

            if (dir_len + sizeof("build_native") > sizeof(build_path)) return false;
            memcpy(build_path, test_binary_path, dir_len);
            memcpy(build_path + dir_len, "build_native", sizeof("build_native"));
            build_tool = build_path;
        }
    }
    build_argv[0] = (char *)build_tool;
    if (!write_temp_source(source, source_path, sizeof(source_path)) ||
        !make_temp_output_path(output_path, output_path_size)) {
        unlink(source_path);
        return false;
    }
    exit_code = run_process(build_tool, build_argv);
    unlink(source_path);
    return exit_code == 0;
}

void test_build_native_runs_simple_start_program(void),
     test_build_native_handles_direct_eight_argument_calls(void),
     test_build_native_runs_runtime_lambda_program(void),
     test_build_native_auto_calls_zero_arg_template_callable(void);
void test_build_native_runs_boot_program(void),
     test_build_native_runs_boot_program_with_block(void),
     test_build_native_runs_manual_malloc_free(void),
     test_build_native_typed_ptr_offset_stride(void),
     test_build_native_supports_string_index_and_length(void);
void test_build_native_stackalloc_store_deref(void),
     test_build_native_stackalloc_typed_ptr(void),
     test_build_native_formats_hetero_arrays(void),
     test_build_native_indexes_hetero_arrays(void),
     test_build_native_compares_hetero_arrays_by_identity(void),
     test_build_native_dispatches_union_tag_and_payload(void),
     test_build_native_formats_union_variants(void),
     test_build_native_layout_field_access(void);
void test_build_native_double_capture_closure(void),
     test_build_native_primitive_types(void),
     test_build_native_varargs_passthrough(void),
     test_build_native_template_literal(void),
     test_build_native_threading_helpers(void);
void test_build_native_manual_checked_abort(void),
     test_build_native_car_multifile(void),
     test_build_native_malformed_source_reports_span(void),
     test_build_native_runs_recursive_top_level_lambda_program(void);
int main(int argc, char **argv) {
    test_binary_path = argc > 0 ? argv[0] : NULL;
    printf("Running native build tests...\n\n");

    RUN_TEST(test_build_native_runs_simple_start_program); RUN_TEST(test_build_native_handles_direct_eight_argument_calls);
    RUN_TEST(test_build_native_runs_runtime_lambda_program); RUN_TEST(test_build_native_auto_calls_zero_arg_template_callable);
    RUN_TEST(test_build_native_runs_boot_program);
    RUN_TEST(test_build_native_runs_boot_program_with_block);
    RUN_TEST(test_build_native_runs_manual_malloc_free);
    RUN_TEST(test_build_native_typed_ptr_offset_stride);
    RUN_TEST(test_build_native_supports_string_index_and_length);
    RUN_TEST(test_build_native_stackalloc_store_deref);
    RUN_TEST(test_build_native_stackalloc_typed_ptr);
    RUN_TEST(test_build_native_formats_hetero_arrays); RUN_TEST(test_build_native_indexes_hetero_arrays);
    RUN_TEST(test_build_native_compares_hetero_arrays_by_identity);
    RUN_TEST(test_build_native_dispatches_union_tag_and_payload);
    RUN_TEST(test_build_native_formats_union_variants);
    RUN_TEST(test_build_native_layout_field_access);
    RUN_TEST(test_build_native_double_capture_closure); RUN_TEST(test_build_native_primitive_types);
    RUN_TEST(test_build_native_varargs_passthrough); RUN_TEST(test_build_native_template_literal);
    RUN_TEST(test_build_native_threading_helpers);
    RUN_TEST(test_build_native_manual_checked_abort);
    RUN_TEST(test_build_native_car_multifile);
    RUN_TEST(test_build_native_malformed_source_reports_span);
    RUN_TEST(test_build_native_runs_recursive_top_level_lambda_program);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
