#define _POSIX_C_SOURCE 200809L

#include "car.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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
 * Test: round-trip in-memory write and read
 * ================================================================ */

static void test_car_round_trip_memory(void) {
    CarArchive writer, reader;
    FILE *tmpf;
    long fsize;
    uint8_t *buf;

    car_archive_init(&writer);

    REQUIRE_TRUE(
        car_archive_add_file(&writer, "main.cal",
                             "start() -> 0;", 14),
        "add main.cal");
    REQUIRE_TRUE(
        car_archive_add_file(&writer, "lib/math.cal",
                             "int32 add = (int32 a, int32 b) -> a + b;", 40),
        "add lib/math.cal");
    ASSERT_EQ_SIZE(2, writer.file_count, "writer has 2 files");

    /* Write to a temp file, then read the bytes back */
    tmpf = tmpfile();
    REQUIRE_TRUE(tmpf != NULL, "tmpfile created");
    REQUIRE_TRUE(car_archive_write_to_stream(&writer, tmpf),
                 "write to stream");

    fsize = ftell(tmpf);
    REQUIRE_TRUE(fsize > 0, "file has content");

    buf = malloc((size_t)fsize);
    REQUIRE_TRUE(buf != NULL, "malloc buf");
    fseek(tmpf, 0, SEEK_SET);
    REQUIRE_TRUE(fread(buf, 1, (size_t)fsize, tmpf) == (size_t)fsize,
                 "read back bytes");
    fclose(tmpf);

    /* Read from memory */
    car_archive_init(&reader);
    REQUIRE_TRUE(car_archive_read_from_memory(&reader, buf, (size_t)fsize),
                 "read from memory");
    free(buf);

    ASSERT_EQ_SIZE(2, reader.file_count, "reader has 2 files");

    {
        const CarFile *f0 = &reader.files[0];
        ASSERT_EQ_STR("main.cal", f0->path, "file 0 path");
        ASSERT_EQ_SIZE(14, f0->content_length, "file 0 content length");
        ASSERT_EQ_STR("start() -> 0;", f0->content, "file 0 content");
    }
    {
        const CarFile *f1 = &reader.files[1];
        ASSERT_EQ_STR("lib/math.cal", f1->path, "file 1 path");
        ASSERT_EQ_SIZE(40, f1->content_length, "file 1 content length");
        ASSERT_EQ_STR("int32 add = (int32 a, int32 b) -> a + b;",
                       f1->content, "file 1 content");
    }

    car_archive_free(&reader);
    car_archive_free(&writer);
}

/* ================================================================
 * Test: find_file lookup
 * ================================================================ */

static void test_car_find_file(void) {
    CarArchive archive;
    const CarFile *found;

    car_archive_init(&archive);
    car_archive_add_file(&archive, "a.cal", "aaa", 3);
    car_archive_add_file(&archive, "pkg/b.cal", "bbb", 3);

    found = car_archive_find_file(&archive, "a.cal");
    REQUIRE_TRUE(found != NULL, "find a.cal");
    ASSERT_EQ_STR("aaa", found->content, "a.cal content");

    found = car_archive_find_file(&archive, "pkg/b.cal");
    REQUIRE_TRUE(found != NULL, "find pkg/b.cal");
    ASSERT_EQ_STR("bbb", found->content, "pkg/b.cal content");

    found = car_archive_find_file(&archive, "nonexistent.cal");
    ASSERT_TRUE(found == NULL, "nonexistent returns NULL");

    car_archive_free(&archive);
}

/* ================================================================
 * Test: write to file / read from file
 * ================================================================ */

static void test_car_write_and_read_file(void) {
    CarArchive writer, reader;
    const char *tmppath = "/tmp/calynda_test.car";

    car_archive_init(&writer);
    car_archive_add_file(&writer, "hello.cal",
                         "start() -> println(\"hello\");", 28);

    REQUIRE_TRUE(car_archive_write(&writer, tmppath),
                 "write to file");

    car_archive_init(&reader);
    REQUIRE_TRUE(car_archive_read(&reader, tmppath),
                 "read from file");

    ASSERT_EQ_SIZE(1, reader.file_count, "reader has 1 file");
    ASSERT_EQ_STR("hello.cal", reader.files[0].path, "path matches");
    ASSERT_EQ_SIZE(28, reader.files[0].content_length, "content length");
    ASSERT_EQ_STR("start() -> println(\"hello\");",
                   reader.files[0].content, "content matches");

    car_archive_free(&reader);
    car_archive_free(&writer);
    unlink(tmppath);
}

/* ================================================================
 * Test: read from invalid data
 * ================================================================ */

static void test_car_read_invalid_magic(void) {
    CarArchive archive;
    uint8_t bad_magic[] = { 'X', 'A', 'R', 0x01, 0, 0, 0, 0 };

    car_archive_init(&archive);
    ASSERT_TRUE(!car_archive_read_from_memory(&archive, bad_magic, 8),
                "bad magic rejected");
    ASSERT_TRUE(archive.has_error, "error flag set");

    car_archive_free(&archive);
}

static void test_car_read_truncated(void) {
    CarArchive archive;
    uint8_t truncated[] = { 'C', 'A', 'R', 0x01 };

    car_archive_init(&archive);
    ASSERT_TRUE(!car_archive_read_from_memory(&archive, truncated, 4),
                "truncated data rejected");
    ASSERT_TRUE(archive.has_error, "error flag set for truncated");

    car_archive_free(&archive);
}

static void test_car_read_bad_version(void) {
    CarArchive archive;
    uint8_t bad_ver[] = { 'C', 'A', 'R', 0xFF, 0, 0, 0, 0 };

    car_archive_init(&archive);
    ASSERT_TRUE(!car_archive_read_from_memory(&archive, bad_ver, 8),
                "bad version rejected");
    ASSERT_TRUE(archive.has_error, "error flag set for bad version");

    car_archive_free(&archive);
}

/* ================================================================
 * Test: empty archive round-trip
 * ================================================================ */

static void test_car_empty_archive(void) {
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

/* ================================================================
 * Test: add_directory from a temp tree
 * ================================================================ */

static void write_temp_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
}

static void test_car_add_directory(void) {
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

static void test_car_many_files(void) {
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

/* ================================================================
 * Main
 * ================================================================ */

int main(void) {
    printf("Running CAR archive tests...\n\n");

    RUN_TEST(test_car_round_trip_memory);
    RUN_TEST(test_car_find_file);
    RUN_TEST(test_car_write_and_read_file);
    RUN_TEST(test_car_read_invalid_magic);
    RUN_TEST(test_car_read_truncated);
    RUN_TEST(test_car_read_bad_version);
    RUN_TEST(test_car_empty_archive);
    RUN_TEST(test_car_add_directory);
    RUN_TEST(test_car_many_files);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
