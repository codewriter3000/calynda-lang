#define _POSIX_C_SOURCE 200809L

#include "car.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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
#define ASSERT_EQ_SIZE(expected, actual, msg) do {                          \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %zu, got %zu\n",     \
                __FILE__, __LINE__, (msg),                                  \
                (size_t)(expected), (size_t)(actual));                      \
    }                                                                       \
} while (0)
#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)


/* ================================================================
 * Helper: write_temp_file
 * ================================================================ */

static void write_temp_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
}

/* ================================================================
 * Test: empty archive round-trip
 * ================================================================ */

void test_car_empty_archive(void) {
    CarArchive writer, reader;
    FILE *tmpf;
    long fsize;
    uint8_t *buf;

    car_archive_init(&writer);

    tmpf = tmpfile();
    REQUIRE_TRUE(tmpf != NULL, "tmpfile created");
    REQUIRE_TRUE(car_archive_write_to_stream(&writer, tmpf),
                 "write empty archive");

    fsize = ftell(tmpf);
    buf = malloc((size_t)fsize);
    fseek(tmpf, 0, SEEK_SET);
    fread(buf, 1, (size_t)fsize, tmpf);
    fclose(tmpf);

    car_archive_init(&reader);
    REQUIRE_TRUE(car_archive_read_from_memory(&reader, buf, (size_t)fsize),
                 "read empty archive");
    free(buf);

    ASSERT_EQ_SIZE(0, reader.file_count, "empty archive has 0 files");

    car_archive_free(&reader);
    car_archive_free(&writer);
}


void test_car_add_directory(void) {
    CarArchive archive;
    const char *tmpdir = "/tmp/calynda_car_test_dir";
    char subdir[512];
    char file1[512];
    char file2[512];
    char file3[512];

    /* Create temp directory tree */
    mkdir(tmpdir, 0755);
    snprintf(subdir, sizeof(subdir), "%s/pkg", tmpdir);
    mkdir(subdir, 0755);

    snprintf(file1, sizeof(file1), "%s/main.cal", tmpdir);
    write_temp_file(file1, "start() -> 0;");

    snprintf(file2, sizeof(file2), "%s/pkg/lib.cal", tmpdir);
    write_temp_file(file2, "int32 add = (int32 a, int32 b) -> a + b;");

    snprintf(file3, sizeof(file3), "%s/readme.txt", tmpdir);
    write_temp_file(file3, "this is not a .cal file");

    car_archive_init(&archive);
    REQUIRE_TRUE(car_archive_add_directory(&archive, tmpdir),
                 "add directory");

    /* Should have exactly 2 .cal files */
    ASSERT_EQ_SIZE(2, archive.file_count,
                   "directory scan finds 2 .cal files");

    /* Verify both files are present (order may vary) */
    {
        const CarFile *main_file = car_archive_find_file(&archive, "main.cal");
        const CarFile *lib_file = car_archive_find_file(&archive, "pkg/lib.cal");

        REQUIRE_TRUE(main_file != NULL, "found main.cal");
        ASSERT_EQ_STR("start() -> 0;", main_file->content,
                       "main.cal content");

        REQUIRE_TRUE(lib_file != NULL, "found pkg/lib.cal");
        ASSERT_EQ_STR("int32 add = (int32 a, int32 b) -> a + b;",
                       lib_file->content, "pkg/lib.cal content");
    }

    car_archive_free(&archive);

    /* Cleanup */
    unlink(file1);
    unlink(file2);
    unlink(file3);
    rmdir(subdir);
    rmdir(tmpdir);
}


/* ================================================================
 * Test: large file count (stress basic growth)
 * ================================================================ */

void test_car_many_files(void) {
    CarArchive writer, reader;
    FILE *tmpf;
    long fsize;
    uint8_t *buf;
    size_t i;
    char path[64];

    car_archive_init(&writer);

    for (i = 0; i < 50; i++) {
        snprintf(path, sizeof(path), "file_%03zu.cal", i);
        REQUIRE_TRUE(car_archive_add_file(&writer, path, "x", 1),
                     "add file in loop");
    }
    ASSERT_EQ_SIZE(50, writer.file_count, "writer has 50 files");

    tmpf = tmpfile();
    REQUIRE_TRUE(tmpf != NULL, "tmpfile for many_files test");
    REQUIRE_TRUE(car_archive_write_to_stream(&writer, tmpf),
                 "write 50 files");

    fsize = ftell(tmpf);
    buf = malloc((size_t)fsize);
    fseek(tmpf, 0, SEEK_SET);
    fread(buf, 1, (size_t)fsize, tmpf);
    fclose(tmpf);

    car_archive_init(&reader);
    REQUIRE_TRUE(car_archive_read_from_memory(&reader, buf, (size_t)fsize),
                 "read 50 files");
    free(buf);

    ASSERT_EQ_SIZE(50, reader.file_count, "reader has 50 files");

    {
        const CarFile *f25 = car_archive_find_file(&reader, "file_025.cal");
        REQUIRE_TRUE(f25 != NULL, "find file_025.cal");
        ASSERT_EQ_STR("x", f25->content, "file_025.cal content");
    }

    car_archive_free(&reader);
    car_archive_free(&writer);
}

