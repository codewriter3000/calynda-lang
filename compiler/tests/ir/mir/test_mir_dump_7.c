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

static void print_parser_diagnostic(const Parser *parser) {
    const ParserError *error = parser_get_error(parser);

    if (!error) {
        return;
    }

    fprintf(stderr,
            "    parser: %s at %d:%d near %.*s\n",
            error->message,
            error->token.line,
            error->token.column,
            (int)error->token.length,
            error->token.start ? error->token.start : "");
}

static void print_formatted_diagnostic(const char *stage,
                                       bool ok,
                                       const char *diagnostic) {
    if (!ok || !diagnostic) {
        return;
    }

    fprintf(stderr, "    %s: %s\n", stage, diagnostic);
}

static char *build_mir_dump(const char *source) {
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    char diagnostic[512];
    char *dump = NULL;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    parser_init(&parser, source);

    if (!parser_parse_program(&parser, &ast_program)) {
        print_parser_diagnostic(&parser);
        goto cleanup;
    }

    if (!symbol_table_build(&symbols, &ast_program)) {
        print_formatted_diagnostic("symbols",
                                   symbol_table_format_error(symbol_table_get_error(&symbols),
                                                             diagnostic,
                                                             sizeof(diagnostic)),
                                   diagnostic);
        goto cleanup;
    }

    if (!type_checker_check_program(&checker, &ast_program, &symbols)) {
        print_formatted_diagnostic("type",
                                   type_checker_format_error(type_checker_get_error(&checker),
                                                             diagnostic,
                                                             sizeof(diagnostic)),
                                   diagnostic);
        goto cleanup;
    }

    if (!hir_build_program(&hir_program, &ast_program, &symbols, &checker)) {
        print_formatted_diagnostic("hir",
                                   hir_format_error(hir_get_error(&hir_program),
                                                    diagnostic,
                                                    sizeof(diagnostic)),
                                   diagnostic);
        goto cleanup;
    }

    if (!mir_build_program(&mir_program, &hir_program, false)) {
        print_formatted_diagnostic("mir",
                                   mir_format_error(mir_get_error(&mir_program),
                                                    diagnostic,
                                                    sizeof(diagnostic)),
                                   diagnostic);
        goto cleanup;
    }

    dump = mir_dump_program_to_string(&mir_program);

cleanup:
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
    return dump;
}

void test_mir_dump_lowers_hetero_array_literal(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true, \"hello\"];\n"
        "    return 0;\n"
        "};\n";
    const char *hetero_array_new;
    const char *true_literal;
    char *dump = build_mir_dump(source);

    REQUIRE_TRUE(dump != NULL, "build hetero array MIR dump");
    hetero_array_new = strstr(dump,
                              "hetero_array_new typedesc(arr|1|g0:raw_word|0:int32|1:bool|2:string) [int32(1), bool(true)");
    true_literal = strstr(dump, "bool(true)");

    ASSERT_TRUE(hetero_array_new != NULL, "arr<?> literals lower with descriptor metadata in MIR");
    ASSERT_TRUE(true_literal != NULL, "hetero_array_new preserves boolean elements");

    free(dump);
}

void test_mir_dump_cleanup_runs_before_manual_return(void) {
    static const char source[] =
        "int64 release = (int64 value) -> {\n"
        "    free(value);\n"
        "    return value;\n"
        "};\n"
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 p = malloc(8);\n"
        "        cleanup(p, release);\n"
        "        return 7;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    const char *cleanup_store;
    const char *cleanup_call;
    const char *return_stmt;
    char *dump = build_mir_dump(source);

    REQUIRE_TRUE(dump != NULL, "build cleanup MIR dump");
    cleanup_store = strstr(dump, "__mir_cleanup_fn");
    cleanup_call = strstr(dump, "call global(free)(");
    return_stmt = strstr(dump, "return int32(7)");

    ASSERT_TRUE(cleanup_store != NULL, "cleanup() snapshots callable state");
    ASSERT_TRUE(cleanup_call != NULL, "cleanup() emits a deferred local call");
    ASSERT_TRUE(cleanup_call != NULL && return_stmt != NULL && cleanup_call < return_stmt,
                "cleanup() runs before the enclosing return");

    free(dump);
}

void test_mir_dump_stackalloc_auto_registers_free(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 p = stackalloc(8);\n"
        "        return 1;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    const char *stackalloc_call;
    const char *free_snapshot;
    const char *cleanup_call;
    const char *return_stmt;
    char *dump = build_mir_dump(source);

    REQUIRE_TRUE(dump != NULL, "build stackalloc cleanup MIR dump");
    stackalloc_call = strstr(dump, "__calynda_stackalloc");
    free_snapshot = strstr(dump, "global(free)");
    cleanup_call = strstr(dump, "call global(free)(");
    return_stmt = strstr(dump, "return int32(1)");

    ASSERT_TRUE(stackalloc_call != NULL, "stackalloc lowers to runtime helper");
    ASSERT_TRUE(free_snapshot != NULL, "stackalloc retains free as the deferred cleanup callee");
    ASSERT_TRUE(cleanup_call != NULL && return_stmt != NULL && cleanup_call < return_stmt,
                "stackalloc cleanup runs before returning from manual scope");

    free(dump);
}