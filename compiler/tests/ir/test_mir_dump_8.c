#include "hir.h"
#include "mir.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

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

static char *build_mir_dump(const char *source) {
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    char *dump = NULL;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    parser_init(&parser, source);

    if (parser_parse_program(&parser, &ast_program) &&
        symbol_table_build(&symbols, &ast_program) &&
        type_checker_check_program(&checker, &ast_program, &symbols) &&
        hir_build_program(&hir_program, &ast_program, &symbols, &checker) &&
        mir_build_program(&mir_program, &hir_program, false)) {
        dump = mir_dump_program_to_string(&mir_program);
    }

    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
    return dump;
}

void test_mir_dump_lowers_union_tag_and_payload_access(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> value = Option.Some(7);\n"
        "    int32 tag = value.tag;\n"
        "    int32 payload_value = int32(value.payload);\n"
        "    return tag + payload_value;\n"
        "};\n";
    char *dump = build_mir_dump(source);

    REQUIRE_TRUE(dump != NULL, "build union access MIR dump");
    ASSERT_TRUE(strstr(dump, "union_get_tag local(1:value)") != NULL,
                "union .tag lowers to a dedicated MIR instruction");
    ASSERT_TRUE(strstr(dump, "union_get_payload local(1:value)") != NULL,
                "union .payload lowers to a dedicated MIR instruction");
    ASSERT_TRUE(strstr(dump, "member local(1:value).tag") == NULL &&
                strstr(dump, "member local(1:value).payload") == NULL,
                "union metadata access no longer falls back to generic MIR members");

    free(dump);
}