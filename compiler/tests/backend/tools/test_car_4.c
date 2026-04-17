#define _POSIX_C_SOURCE 200809L

#include "car.h"

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

static bool write_test_archive(const char *archive_path, const char *source) {
    CarArchive archive;
    FILE *file = NULL;
    bool ok = false;
    car_archive_init(&archive);
    if (car_archive_add_file(&archive, "main.cal", source, strlen(source))) {
        file = fopen(archive_path, "wb");
        ok = file != NULL && car_archive_write_to_stream(&archive, file);
    }
    if (file) fclose(file);
    car_archive_free(&archive);
    return ok;
}

void test_car_build_reports_parse_span(void) {
    static const char source[] = "start(string[] args) -> {\n    @\n};\n";
    char car_path[128];
    char output_path[128];
    char output[4096];
    int exit_code = 0;
    char *argv[] = { (char *) "./build/calynda", (char *) "build", car_path, (char *) "-o", output_path, NULL };

    snprintf(car_path, sizeof(car_path), "build/test-car-malformed-%ld.car", (long) getpid());
    snprintf(output_path, sizeof(output_path), "build/test-car-malformed-%ld.out", (long) getpid());
    REQUIRE_TRUE(write_test_archive(car_path, source), "write malformed CAR archive");
    REQUIRE_TRUE(run_capture_combined("./build/calynda", argv, output, sizeof(output), &exit_code),
                 "run calynda build on malformed CAR archive");
    ASSERT_TRUE(exit_code != 0, "malformed CAR build exits non-zero");
    ASSERT_CONTAINS("main.cal:2:5: parse error:", output, "CAR diagnostic includes archive member span");
    ASSERT_CONTAINS("Unexpected character", output, "CAR diagnostic includes parse message");
    unlink(car_path);
    unlink(output_path);
}

void test_car_collects_exported_symbols_by_package(void) {
    static const char source[] =
        "package lib.math;\n"
        "export int32 square = (int32 x) -> x * x;\n"
        "export type Count = int32;\n"
        "export union Maybe<T> { Some(T), None };\n"
        "start(string[] args) -> 0;\n";
    CarArchive archive;
    CarExportedSymbol *symbols = NULL;
    size_t symbol_count = 0;
    bool found_square = false;
    bool found_count = false;
    bool found_maybe = false;
    size_t i;

    car_archive_init(&archive);
    REQUIRE_TRUE(car_archive_add_file(&archive, "lib/math.cal",
                                      source, strlen(source)),
                 "build export enumeration archive");
    REQUIRE_TRUE(car_archive_collect_exported_symbols(&archive, "lib.math",
                                                      &symbols, &symbol_count),
                 "collect exported symbols from package");
    ASSERT_TRUE(symbol_count == 3, "collects 3 exported symbols from package");

    for (i = 0; i < symbol_count; i++) {
        if (strcmp(symbols[i].name, "square") == 0) {
            found_square = true;
            ASSERT_TRUE(symbols[i].kind == CAR_EXPORTED_SYMBOL_CALLABLE,
                        "square exported as callable");
            ASSERT_TRUE(symbols[i].parameters.count == 1,
                        "square exposes one parameter");
        } else if (strcmp(symbols[i].name, "Count") == 0) {
            found_count = true;
            ASSERT_TRUE(symbols[i].kind == CAR_EXPORTED_SYMBOL_TYPE_ALIAS,
                        "Count exported as type alias");
        } else if (strcmp(symbols[i].name, "Maybe") == 0) {
            found_maybe = true;
            ASSERT_TRUE(symbols[i].kind == CAR_EXPORTED_SYMBOL_UNION,
                        "Maybe exported as union");
            ASSERT_TRUE(symbols[i].generic_param_count == 1,
                        "Maybe preserves generic parameter count");
        }
    }

    ASSERT_TRUE(found_square, "square export found");
    ASSERT_TRUE(found_count, "Count export found");
    ASSERT_TRUE(found_maybe, "Maybe export found");
    car_exported_symbol_list_free(symbols, symbol_count);
    car_archive_free(&archive);
}
