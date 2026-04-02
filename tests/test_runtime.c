#define _POSIX_C_SOURCE 200809L

#include "../src/runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;
static size_t startup_argument_count = 0;
static const char *startup_first_argument = NULL;

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

#define ASSERT_EQ_WORD(expected, actual, msg) do {                          \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %lld, got %lld\n",    \
                __FILE__, __LINE__, (msg),                                  \
                (long long)(expected), (long long)(actual));                \
    }                                                                       \
} while (0)

#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

static CalyndaRtWord sum_with_capture(const CalyndaRtWord *captures,
                                      size_t capture_count,
                                      const CalyndaRtWord *arguments,
                                      size_t argument_count) {
    (void)capture_count;
    (void)argument_count;
    return captures[0] + arguments[0] + arguments[1];
}

static CalyndaRtWord capture_start_arguments(CalyndaRtWord arguments) {
    startup_argument_count = calynda_rt_array_length(arguments);
    startup_first_argument = startup_argument_count > 0
        ? calynda_rt_string_bytes(__calynda_rt_index_load(arguments, 0))
        : NULL;
    return 23;
}

static void test_runtime_layout_dump_defines_object_model(void) {
    static const char expected[] =
        "RuntimeLayout word=uint64 raw-scalar-or-object-handle\n"
        "  ObjectHeader size=8 magic=0x434C5944 fields=[magic:uint32, kind:uint32]\n"
        "  String size=24 payload=[length:size_t, bytes:char*]\n"
        "  Array size=24 payload=[count:size_t, elements:uint64*]\n"
        "  Closure size=32 payload=[entry:void*, capture_count:size_t, captures:uint64*]\n"
        "  Package size=16 payload=[name:char*]\n"
        "  ExternCallable size=24 payload=[kind:uint32, name:char*]\n"
        "  TemplatePart size=16 payload=[tag:uint64, payload:uint64]\n"
        "  TemplateTags text=0 value=1\n"
        "  Builtins package=stdlib member=print\n";
    char *dump = calynda_rt_dump_layout_to_string();

    REQUIRE_TRUE(dump != NULL, "render runtime layout dump");
    ASSERT_EQ_STR(expected, dump, "runtime layout dump string");
    free(dump);
}

static void test_runtime_array_index_and_store_helpers(void) {
    CalyndaRtWord elements[3] = { 1, 2, 3 };
    CalyndaRtWord array = __calynda_rt_array_literal(3, elements);

    ASSERT_TRUE(calynda_rt_is_object(array), "array literal produces an object handle");
    ASSERT_EQ_WORD(3, (CalyndaRtWord)calynda_rt_array_length(array), "array length is preserved");
    ASSERT_EQ_WORD(2, __calynda_rt_index_load(array, 1), "index load reads the original element");

    __calynda_rt_store_index(array, 1, 42);
    ASSERT_EQ_WORD(42, __calynda_rt_index_load(array, 1), "index store writes the updated element");
}

static void test_runtime_closure_new_and_call_callable(void) {
    CalyndaRtWord captures[1] = { 5 };
    CalyndaRtWord arguments[2] = { 7, 9 };
    CalyndaRtWord closure = __calynda_rt_closure_new(sum_with_capture, 1, captures);

    ASSERT_TRUE(calynda_rt_is_object(closure), "closure helper produces an object handle");
    ASSERT_EQ_WORD(21,
                   __calynda_rt_call_callable(closure, 2, arguments),
                   "call callable invokes the stored closure entry with captures and arguments");
}

static void test_runtime_template_build_and_string_cast(void) {
    CalyndaRtTemplatePart parts[2];
    CalyndaRtWord built;
    CalyndaRtWord cast_string;

    parts[0].tag = CALYNDA_RT_TEMPLATE_TAG_TEXT;
    parts[0].payload = (CalyndaRtWord)(uintptr_t)"value ";
    parts[1].tag = CALYNDA_RT_TEMPLATE_TAG_VALUE;
    parts[1].payload = 42;

    built = __calynda_rt_template_build(2, parts);
    REQUIRE_TRUE(calynda_rt_string_bytes(built) != NULL, "template build returns a runtime string");
    ASSERT_EQ_STR("value 42",
                  calynda_rt_string_bytes(built),
                  "template helper concatenates text and formatted values");

    cast_string = __calynda_rt_cast_value(7, CALYNDA_RT_TYPE_STRING);
    REQUIRE_TRUE(calynda_rt_string_bytes(cast_string) != NULL, "string cast returns a runtime string");
    ASSERT_EQ_STR("7",
                  calynda_rt_string_bytes(cast_string),
                  "cast-to-string stringifies the source word");
}

static void test_runtime_member_load_returns_callable_builtin(void) {
    CalyndaRtWord callable = __calynda_rt_member_load((CalyndaRtWord)(uintptr_t)&__calynda_pkg_stdlib,
                                                      "print");
    FILE *capture;
    int saved_stdout;
    char buffer[64] = {0};
    CalyndaRtWord argument = calynda_rt_make_string_copy("hello");

    REQUIRE_TRUE(calynda_rt_is_object(callable), "member load on stdlib returns an object handle");

    fflush(stdout);
    saved_stdout = dup(fileno(stdout));
    REQUIRE_TRUE(saved_stdout >= 0, "duplicate stdout before capture");
    capture = tmpfile();
    REQUIRE_TRUE(capture != NULL, "open capture stream for builtin print");
    REQUIRE_TRUE(dup2(fileno(capture), fileno(stdout)) >= 0, "redirect stdout to capture stream");

    ASSERT_EQ_WORD(0,
                   __calynda_rt_call_callable(callable, 1, &argument),
                   "builtin print returns zero");
    fflush(stdout);

    REQUIRE_TRUE(dup2(saved_stdout, fileno(stdout)) >= 0, "restore stdout after capture");
    close(saved_stdout);
    rewind(capture);
    REQUIRE_TRUE(fgets(buffer, sizeof(buffer), capture) != NULL, "read captured builtin print output");
    {
        const char *captured = buffer;
        ASSERT_EQ_STR("hello\n", captured, "builtin print writes the formatted value to stdout");
    }
    fclose(capture);
}

static void test_runtime_start_process_boxes_cli_arguments(void) {
    char *argv[] = { (char *)"program", (char *)"hello", (char *)"world", NULL };
    int exit_code;

    startup_argument_count = 0;
    startup_first_argument = NULL;
    exit_code = calynda_rt_start_process(capture_start_arguments, 3, argv);

    ASSERT_EQ_WORD(23, (CalyndaRtWord)exit_code, "start process returns the start unit result as a process exit code");
    ASSERT_EQ_WORD(2,
                   (CalyndaRtWord)startup_argument_count,
                   "start process excludes argv[0] when boxing Calynda args");
    ASSERT_EQ_STR("hello",
                  startup_first_argument,
                  "start process preserves the first user argument as a runtime string");
}

int main(void) {
    printf("Running runtime tests...\n\n");

    RUN_TEST(test_runtime_layout_dump_defines_object_model);
    RUN_TEST(test_runtime_array_index_and_store_helpers);
    RUN_TEST(test_runtime_closure_new_and_call_callable);
    RUN_TEST(test_runtime_template_build_and_string_cast);
    RUN_TEST(test_runtime_member_load_returns_callable_builtin);
    RUN_TEST(test_runtime_start_process_boxes_cli_arguments);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}