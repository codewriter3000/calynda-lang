#include "hir.h"
#include "lir.h"
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

void test_lir_dump_lowers_union_tag_and_payload_access(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> value = Option.Some(7);\n"
        "    int32 tag = value.tag;\n"
        "    int32 payload_value = int32(value.payload);\n"
        "    return tag + payload_value;\n"
        "};\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse union access LIR program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for union access LIR program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols),
                 "type check union access LIR program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                 "lower HIR for union access LIR program");
    REQUIRE_TRUE(mir_build_program(&mir_program, &hir_program, false),
                 "lower MIR for union access LIR program");
    REQUIRE_TRUE(lir_build_program(&lir_program, &mir_program),
                 "lower LIR for union tag/payload access");

    dump = lir_dump_program_to_string(&lir_program);
    REQUIRE_TRUE(dump != NULL, "render union access LIR dump to string");
    ASSERT_TRUE(strstr(dump, "union_get_tag slot(1:value)") != NULL,
                "union .tag lowers to a dedicated LIR instruction");
    ASSERT_TRUE(strstr(dump, "union_get_payload slot(1:value)") != NULL,
                "union .payload lowers to a dedicated LIR instruction");
    ASSERT_TRUE(strstr(dump, "member slot(1:value).tag") == NULL &&
                strstr(dump, "member slot(1:value).payload") == NULL,
                "union metadata access no longer falls back to generic LIR members");

    free(dump);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}