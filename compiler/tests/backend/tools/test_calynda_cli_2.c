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

bool write_temp_source(const char *source, char *path_buffer, size_t buffer_size);
bool make_temp_output_path(char *path_buffer, size_t buffer_size);
bool run_capture(const char *path,
                        char *const argv[],
                        char *buffer,
                        size_t buffer_size,
                        int *exit_code);

static bool write_text_file(const char *path, const char *contents) {
    FILE *file;

    if (!path || !contents) {
        return false;
    }

    file = fopen(path, "wb");
    if (!file) {
        return false;
    }

    if (fputs(contents, file) == EOF) {
        fclose(file);
        return false;
    }

    return fclose(file) == 0;
}


void test_calynda_cli_help_and_emitters(void) {
    static const char source[] =
        "start(string[] args) -> 7;\n";
    char source_path[64];
    char output[4096];
    const char *captured_output;
    char *help_argv[] = { (char *)"./build/calynda", (char *)"help", NULL };
    char *version_argv[] = { (char *)"./build/calynda", (char *)"--version", NULL };
    char *asm_argv[] = { (char *)"./build/calynda", (char *)"asm", source_path, NULL };
    char *strict_asm_argv[] = {
        (char *)"./build/calynda",
        (char *)"asm",
        (char *)"--strict-race-check",
        source_path,
        NULL
    };
    char *bytecode_argv[] = { (char *)"./build/calynda", (char *)"bytecode", source_path, NULL };
    int exit_code;

    REQUIRE_TRUE(write_temp_source(source, source_path, sizeof(source_path)),
                 "write CLI test source file");

    REQUIRE_TRUE(run_capture("./build/calynda", help_argv, output, sizeof(output), &exit_code),
                 "run calynda help");
    captured_output = output;
    ASSERT_EQ_INT(0, exit_code, "calynda help exits successfully");
    ASSERT_CONTAINS("build", captured_output, "help text lists build command");
    ASSERT_CONTAINS("--version", captured_output, "help text lists version flag");
    ASSERT_CONTAINS("--strict-race-check", captured_output, "help text lists strict race flag");
    ASSERT_CONTAINS("Compiler options", captured_output, "help text includes compiler options");

    REQUIRE_TRUE(run_capture("./build/calynda", version_argv, output, sizeof(output), &exit_code),
                 "run calynda version");
    captured_output = output;
    ASSERT_EQ_INT(0, exit_code, "calynda version exits successfully");
    ASSERT_CONTAINS("1.0.0-alpha.5", captured_output, "version text prints alpha.5 metadata");

    REQUIRE_TRUE(run_capture("./build/calynda", asm_argv, output, sizeof(output), &exit_code),
                 "run calynda asm");
    captured_output = output;
    ASSERT_EQ_INT(0, exit_code, "calynda asm exits successfully");
    ASSERT_CONTAINS(".globl main", captured_output, "calynda asm emits executable assembly");

    REQUIRE_TRUE(run_capture("./build/calynda", strict_asm_argv, output, sizeof(output), &exit_code),
                 "run calynda asm with strict race flag");
    captured_output = output;
    ASSERT_EQ_INT(0, exit_code, "calynda asm with strict race flag exits successfully");
    ASSERT_CONTAINS(".globl main", captured_output, "strict race flag preserves asm emission");

    REQUIRE_TRUE(run_capture("./build/calynda", bytecode_argv, output, sizeof(output), &exit_code),
                 "run calynda bytecode");
    captured_output = output;
    ASSERT_EQ_INT(0, exit_code, "calynda bytecode exits successfully");
    ASSERT_CONTAINS("BytecodeProgram target=portable-v1", captured_output, "calynda bytecode emits bytecode text");

    unlink(source_path);
}


void test_calynda_cli_builds_native_executable(void) {
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


void test_calynda_cli_asm_accepts_car_archive(void) {
    static const char main_source[] =
        "start(string[] args) -> add(40, 2);\n";
    static const char math_source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n";
    char dir_path[] = "/tmp/calynda-cli-car-XXXXXX";
    char main_path[128];
    char math_path[128];
    char car_path[128];
    char output[4096];
    const char *captured_output;
    char *pack_argv[] = {
        (char *)"./build/calynda",
        (char *)"pack",
        dir_path,
        (char *)"-o",
        car_path,
        NULL
    };
    char *asm_argv[] = {
        (char *)"./build/calynda",
        (char *)"asm",
        car_path,
        NULL
    };
    int exit_code;

    REQUIRE_TRUE(mkdtemp(dir_path) != NULL, "create temp CAR directory");
    snprintf(main_path, sizeof(main_path), "%s/main.cal", dir_path);
    snprintf(math_path, sizeof(math_path), "%s/math.cal", dir_path);
    snprintf(car_path, sizeof(car_path), "%s/project.car", dir_path);

    REQUIRE_TRUE(write_text_file(main_path, main_source),
                 "write CAR main source file");
    REQUIRE_TRUE(write_text_file(math_path, math_source),
                 "write CAR math source file");

    REQUIRE_TRUE(run_capture("./build/calynda", pack_argv, output, sizeof(output),
                             &exit_code),
                 "pack CAR archive for asm CLI test");
    ASSERT_EQ_INT(0, exit_code, "calynda pack exits successfully for asm CAR test");

    REQUIRE_TRUE(run_capture("./build/calynda", asm_argv, output, sizeof(output),
                             &exit_code),
                 "run calynda asm on CAR archive");
    captured_output = output;
    ASSERT_EQ_INT(0, exit_code, "calynda asm accepts CAR archives");
    ASSERT_CONTAINS(".globl main", captured_output,
                    "calynda asm emits executable assembly from CAR archive");
    ASSERT_CONTAINS("calynda_unit_add", captured_output,
                    "CAR asm output includes lowered helper unit from second file");

    unlink(car_path);
    unlink(main_path);
    unlink(math_path);
    rmdir(dir_path);
}
