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
#define ASSERT_EQ_INT(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %d, got %d\n",      \
                __FILE__, __LINE__, (msg), (int)(expected), (int)(actual)); \
    }                                                                       \
} while (0)
#define ASSERT_EQ_STR(expected, actual, msg) do {                           \
    const char *actual_text = (actual);                                     \
    tests_run++;                                                            \
    if (actual_text != NULL && strcmp((expected), actual_text) == 0) {      \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected \"%s\", got \"%s\"\n", \
                __FILE__, __LINE__, (msg), (expected),                      \
                actual_text ? actual_text : "(null)");                     \
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
extern int  run_process(const char *path, char *const argv[]);
extern bool run_process_capture_stdout(const char *path,
                                       char *const argv[],
                                       char *buffer,
                                       size_t buffer_size,
                                       int *exit_code);


void test_build_native_stackalloc_store_deref(void) {
    /* stackalloc 8 bytes, store 77, deref and return it */
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 buf = stackalloc(8);\n"
        "        store(buf, 77);\n"
        "        int64 val = deref(buf);\n"
        "        return val;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build stackalloc store/deref executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(77, exit_code, "stackalloc store/deref returns stored value");
    unlink(output_path);
}

void test_build_native_stackalloc_typed_ptr(void) {
    /* stackalloc 12 bytes for 3 int32s, store 55 at index 2 via offset stride */
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        ptr<int32> p = stackalloc(12);\n"
        "        store(p, 0);\n"
        "        ptr<int32> q = offset(p, 1);\n"
        "        store(q, 0);\n"
        "        ptr<int32> r = offset(p, 2);\n"
        "        store(r, 55);\n"
        "        int64 val = deref(r);\n"
        "        return val;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build stackalloc typed ptr executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(55, exit_code, "stackalloc typed ptr<int32> offset stores and reads correctly");
    unlink(output_path);
}

void test_build_native_layout_field_access(void) {
    /* layout Point { int32 x; int32 y; };
       Allocate 8 bytes (2 int32 fields), store 33 at offset 0 (x),
       store 44 at offset 1 (y, since element_size=4, index 1 = byte 4).
       Return value at offset 1 = 44. */
    static const char source[] =
        "layout Point { int32 x; int32 y; };\n"
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        ptr<Point> p = stackalloc(8);\n"
        "        store(p, 33);\n"
        "        ptr<Point> q = offset(p, 1);\n"
        "        store(q, 44);\n"
        "        int32 v = deref(q);\n"
        "        return v;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build layout field access executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(44, exit_code, "layout ptr<Point> offset field access returns 44");
    unlink(output_path);
}

void test_build_native_formats_hetero_arrays(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true, \"hello\"];\n"
        "    stdlib.print(mixed);\n"
        "    return 0;\n"
        "};\n";
    char output_path[64];
    char stdout_buffer[128];
    char *run_argv[] = { output_path, NULL };
    int exit_code = -1;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build native executable with hetero array formatting");
    REQUIRE_TRUE(run_process_capture_stdout(output_path,
                                            run_argv,
                                            stdout_buffer,
                                            sizeof(stdout_buffer),
                                            &exit_code),
                 "capture hetero-array-formatting stdout");
    ASSERT_EQ_INT(0, exit_code, "hetero-array-formatting executable exits successfully");
    ASSERT_EQ_STR("[1, true, hello]\n",
                  stdout_buffer,
                  "native hetero arrays format through runtime");
    unlink(output_path);
}

void test_build_native_indexes_hetero_arrays(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true, \"hello\"];\n"
        "    stdlib.print(string(mixed[2]));\n"
        "    return int32(mixed[0]);\n"
        "};\n";
    char output_path[64];
    char stdout_buffer[128];
    char *run_argv[] = { output_path, NULL };
    int exit_code = -1;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build native executable with hetero array indexing");
    REQUIRE_TRUE(run_process_capture_stdout(output_path,
                                            run_argv,
                                            stdout_buffer,
                                            sizeof(stdout_buffer),
                                            &exit_code),
                 "capture hetero-array-indexing stdout");
    ASSERT_EQ_INT(1, exit_code, "hetero-array-indexing executable returns the indexed integer");
    ASSERT_EQ_STR("hello\n",
                  stdout_buffer,
                  "hetero array indexing returns external values that can be cast and printed");
    unlink(output_path);
}

void test_build_native_formats_union_variants(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> value = Option.Some(42);\n"
        "    stdlib.print(value);\n"
        "    return 0;\n"
        "};\n";
    char output_path[64];
    char stdout_buffer[128];
    char *run_argv[] = { output_path, NULL };
    int exit_code = -1;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build native executable with union formatting");
    REQUIRE_TRUE(run_process_capture_stdout(output_path,
                                            run_argv,
                                            stdout_buffer,
                                            sizeof(stdout_buffer),
                                            &exit_code),
                 "capture union-formatting stdout");
    ASSERT_EQ_INT(0, exit_code, "union-formatting executable exits successfully");
    ASSERT_EQ_STR("<Option::Some>\n",
                  stdout_buffer,
                  "native unions format through emitted type descriptors");
    unlink(output_path);
}
