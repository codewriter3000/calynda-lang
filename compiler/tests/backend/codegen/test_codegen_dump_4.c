#include "codegen.h"
#include "hir.h"
#include "lir.h"
#include "mir.h"
#include "parser.h"
#include "target.h"

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

void test_codegen_builder_preserves_upstream_error_spans(void) {
    LirProgram lir_program;
    CodegenProgram codegen_program;
    char expected[256];
    char actual[256];

    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    REQUIRE_TRUE(build_invalid_lir_program(&lir_program, expected, sizeof(expected)),
                 "build structured LIR error for codegen precondition");
    ASSERT_TRUE(!codegen_build_program(&codegen_program, &lir_program, target_get_default()),
                "codegen rejects LIR programs with structured errors");
    REQUIRE_TRUE(codegen_get_error(&codegen_program) != NULL,
                 "codegen exposes structured build error");
    REQUIRE_TRUE(codegen_format_error(codegen_get_error(&codegen_program), actual, sizeof(actual)),
                 "format propagated codegen build error");
    ASSERT_TRUE(strcmp(expected, actual) == 0,
                "codegen build error preserves upstream diagnostic text");
    ASSERT_TRUE(codegen_get_error(&codegen_program)->primary_span.start_line ==
                    lir_get_error(&lir_program)->primary_span.start_line &&
                codegen_get_error(&codegen_program)->primary_span.start_column ==
                    lir_get_error(&lir_program)->primary_span.start_column,
                "codegen build error preserves the primary diagnostic span");

    codegen_program_free(&codegen_program);
    lir_program_free(&lir_program);
}
