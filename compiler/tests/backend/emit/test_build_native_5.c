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
/*  B5: manual checked out-of-bounds triggers abort (non-zero exit)   */
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
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build manual checked abort program");
    char *argv[] = { output_path, NULL };
    exit_code = run_process(output_path, argv);
    ASSERT_TRUE(exit_code != 0, "manual checked out-of-bounds triggers non-zero exit");
    unlink(output_path);
}


/* ------------------------------------------------------------------ */
/*  B6: CAR multi-file E2E native build                               */
/* ------------------------------------------------------------------ */

static bool resolve_calynda_binary(char *buffer, size_t buffer_size) {
    extern const char *test_binary_path;
    if (test_binary_path) {
        char *slash = strrchr(test_binary_path, '/');
        if (slash) {
            size_t dir_len = (size_t)(slash - test_binary_path + 1);
            if (dir_len + sizeof("calynda") > buffer_size) return false;
            memcpy(buffer, test_binary_path, dir_len);
            memcpy(buffer + dir_len, "calynda", sizeof("calynda"));
            return true;
        }
    }
    if (sizeof("./build/calynda") > buffer_size) return false;
    strcpy(buffer, "./build/calynda");
    return true;
}

void test_build_native_car_multifile(void) {
    static const char main_cal[] =
        "start(string[] args) -> {\n"
        "    return add(30, 12);\n"
        "};\n";
    static const char math_cal[] =
        "int32 add = (int32 a, int32 b) -> a + b;\n";
    char car_path[128];
    char out_path[] = "/tmp/calynda-car-exe-XXXXXX";
    char calynda_bin[128];
    int fd_out, exit_code;
    FILE *f;

    REQUIRE_TRUE(resolve_calynda_binary(calynda_bin, sizeof(calynda_bin)),
                 "resolve calynda binary for CAR test");

    /* Write two source files into a temp directory and pack via CAR API */
    char dir_path[] = "/tmp/calynda-car-dir-XXXXXX";
    REQUIRE_TRUE(mkdtemp(dir_path) != NULL, "create temp dir for CAR test");

    /* Build the .car path inside the temp directory */
    snprintf(car_path, sizeof(car_path), "%s/project.car", dir_path);

    char main_path[128], math_path[128];
    snprintf(main_path, sizeof(main_path), "%s/main.cal", dir_path);
    snprintf(math_path, sizeof(math_path), "%s/math.cal", dir_path);

    f = fopen(main_path, "w");
    REQUIRE_TRUE(f != NULL, "write main.cal");
    fputs(main_cal, f);
    fclose(f);

    f = fopen(math_path, "w");
    REQUIRE_TRUE(f != NULL, "write math.cal");
    fputs(math_cal, f);
    fclose(f);

    /* Pack with calynda pack */
    char *pack_argv[] = { calynda_bin, "pack", dir_path, "-o", car_path, NULL };
    exit_code = run_process(calynda_bin, pack_argv);
    ASSERT_EQ_INT(0, exit_code, "calynda pack succeeds");

    /* Build from .car */
    fd_out = mkstemp(out_path);
    REQUIRE_TRUE(fd_out >= 0, "create temp exe for CAR test");
    close(fd_out);

    char *build_argv[] = { calynda_bin, "build", car_path, "-o", out_path, NULL };
    exit_code = run_process(calynda_bin, build_argv);
    ASSERT_EQ_INT(0, exit_code, "calynda build from .car succeeds");

    /* Run the built executable */
    char *run_argv[] = { out_path, NULL };
    exit_code = run_process(out_path, run_argv);
    ASSERT_EQ_INT(42, exit_code, "CAR multi-file program returns 42");

    unlink(main_path);
    unlink(math_path);
    unlink(car_path);
    rmdir(dir_path);
    unlink(out_path);
}

void test_build_native_threading_helpers(void) {
    static const char source[] =
        "type ExitCode = int32;\n"
        "start(string[] args) -> {\n"
        "    Thread t = spawn () -> {\n"
        "    };\n"
        "    Mutex m = Mutex.new();\n"
        "    m.lock();\n"
        "    m.unlock();\n"
        "    t.join();\n"
        "    ExitCode code = 0;\n"
        "    return code;\n"
        "};\n";
    char output_path[64];
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build threading helper program");
    {
        char *argv[] = { output_path, NULL };
        exit_code = run_process(output_path, argv);
    }
    ASSERT_EQ_INT(0, exit_code, "threading helper program returns 0");
    unlink(output_path);
}
