#include "codegen.h"
#include "hir.h"
#include "lir.h"
#include "machine.h"
#include "mir.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
                (haystack) ? (haystack) : "(null)");                       \
    }                                                                       \
} while (0)

bool build_machine_dump_from_source(const char *source, char **dump_out);

void test_machine_dump_routes_union_tag_and_payload_helpers(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> value = Option.Some(7);\n"
        "    int32 tag = value.tag;\n"
        "    int32 payload_value = int32(value.payload);\n"
        "    return tag + payload_value;\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_machine_dump_from_source(source, &dump),
                 "emit union access machine program");
    ASSERT_CONTAINS("call __calynda_rt_union_get_tag", dump,
                    "union .tag lowers through the dedicated runtime helper");
    ASSERT_CONTAINS("call __calynda_rt_union_get_payload", dump,
                    "union .payload lowers through the dedicated runtime helper");
    ASSERT_TRUE(strstr(dump, "call __calynda_rt_member_load") == NULL,
                "union metadata access no longer lowers through generic member loads");

    free(dump);
}


void test_machine_error_api_returns_null_on_success(void) {
    static const char source[] =
        "start(string[] args) -> 0;\n";
    char *dump;
    const MachineBuildError *error;
    MachineProgram machine_program;

    machine_program_init(&machine_program);
    REQUIRE_TRUE(build_machine_dump_from_source(source, &dump),
                 "build machine program for error API test");
    free(dump);

    ASSERT_TRUE(!machine_program.has_error, "fresh machine program has no error flag");
    error = machine_get_error(&machine_program);
    ASSERT_TRUE(error == NULL, "machine_get_error returns NULL when no error");
    ASSERT_TRUE(!machine_format_error(NULL, NULL, 0),
                "machine_format_error returns false for NULL error");

    machine_program_free(&machine_program);
}