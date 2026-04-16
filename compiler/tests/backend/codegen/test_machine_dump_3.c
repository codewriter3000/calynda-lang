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

static bool build_invalid_lir_program(LirProgram *lir_program,
                                      char *expected,
                                      size_t expected_size) {
    static const char source[] = "start(string[] args) -> missing + 1;\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    bool ok = false;

    memset(&ast_program, 0, sizeof(ast_program));
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    parser_init(&parser, source);
    if (parser_parse_program(&parser, &ast_program) &&
        symbol_table_build(&symbols, &ast_program) &&
        !type_checker_check_program(&checker, &ast_program, &symbols) &&
        !hir_build_program(&hir_program, &ast_program, &symbols, &checker) &&
        hir_format_error(hir_get_error(&hir_program), expected, expected_size)) {
        mir_program.has_error = true;
        mir_program.error.primary_span = hir_get_error(&hir_program)->primary_span;
        mir_program.error.related_span = hir_get_error(&hir_program)->related_span;
        mir_program.error.has_related_span = hir_get_error(&hir_program)->has_related_span;
        strncpy(mir_program.error.message,
                hir_get_error(&hir_program)->message,
                sizeof(mir_program.error.message) - 1);
        mir_program.error.message[sizeof(mir_program.error.message) - 1] = '\0';
        ok = !lir_build_program(lir_program, &mir_program);
    }

    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
    return ok;
}

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

void test_machine_builder_preserves_upstream_error_spans(void) {
    LirProgram lir_program;
    CodegenProgram codegen_program;
    MachineProgram machine_program;
    char expected[256];
    char actual[256];

    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    machine_program_init(&machine_program);
    REQUIRE_TRUE(build_invalid_lir_program(&lir_program, expected, sizeof(expected)),
                 "build structured LIR error for machine precondition");
    ASSERT_TRUE(!codegen_build_program(&codegen_program, &lir_program, target_get_default()),
                "codegen rejects invalid machine precondition");
    ASSERT_TRUE(!machine_build_program(&machine_program, &lir_program, &codegen_program),
                "machine builder rejects structured upstream errors");
    REQUIRE_TRUE(machine_get_error(&machine_program) != NULL,
                 "machine builder exposes structured build error");
    REQUIRE_TRUE(machine_format_error(machine_get_error(&machine_program), actual, sizeof(actual)),
                 "format propagated machine build error");
    ASSERT_TRUE(strcmp(expected, actual) == 0,
                "machine build error preserves upstream diagnostic text");
    ASSERT_TRUE(machine_get_error(&machine_program)->primary_span.start_line ==
                    lir_get_error(&lir_program)->primary_span.start_line &&
                machine_get_error(&machine_program)->primary_span.start_column ==
                    lir_get_error(&lir_program)->primary_span.start_column,
                "machine build error preserves the primary diagnostic span");

    machine_program_free(&machine_program);
    codegen_program_free(&codegen_program);
    lir_program_free(&lir_program);
}
