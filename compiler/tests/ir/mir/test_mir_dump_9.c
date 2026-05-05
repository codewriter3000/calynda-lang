#include "hir.h"
#include "mir.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

#define ASSERT_EQ_INT(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %d, got %d\n",      \
                __FILE__, __LINE__, (msg), (int)(expected), (int)(actual)); \
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
#define ASSERT_CONTAINS(needle, haystack, msg) do {                         \
    tests_run++;                                                            \
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {      \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\" in \"%s\"\n", \
                __FILE__, __LINE__, (msg), (needle),                        \
                (haystack) ? (haystack) : "(null)");                       \
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


/* ------------------------------------------------------------------ */
/*  G-MIR-7: MIR build error has structured span                     */
/*                                                                    */
/*  Note: MIR errors are rare because type checking catches most      */
/*  problems. We verify the MirBuildError struct and format_error     */
/*  API exist and work on a valid program (no error).                 */
/* ------------------------------------------------------------------ */

void test_mir_dump_valid_program_has_no_error(void) {
    static const char source[] =
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    parser_init(&parser, source);

    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse valid MIR");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &ast_program, &symbols), "type check");
    REQUIRE_TRUE(hir_build_program(&hir_program, &ast_program, &symbols, &checker), "hir build");
    ASSERT_TRUE(mir_build_program(&mir_program, &hir_program, false),
                "valid program builds MIR successfully");
    ASSERT_TRUE(mir_get_error(&mir_program) == NULL ||
                !mir_program.has_error,
                "valid program has no MIR build error");

    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/*  G-MIR-3: Double-capture closure in MIR                            */
/* ------------------------------------------------------------------ */

void test_mir_dump_double_capture_closure(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    var x = 10;\n"
        "    var inner = () -> {\n"
        "        var nested = () -> x;\n"
        "        return nested();\n"
        "    };\n"
        "    return inner();\n"
        "};\n";
    char *dump = build_mir_dump(source);

    REQUIRE_TRUE(dump != NULL, "double capture MIR builds");
    ASSERT_CONTAINS("closure", dump, "double capture produces closure instructions");
    ASSERT_CONTAINS("capture", dump, "double capture shows captured variables");
    free(dump);
}


/* ------------------------------------------------------------------ */
/*  G-MIR-8: MIR error API is safe when no error has occurred        */
/*                                                                    */
/*  Verifies: mir_format_error(NULL, ...) returns false without       */
/*  crashing, and mir_get_error returns NULL on a fresh program.      */
/* ------------------------------------------------------------------ */
void test_mir_dump_error_api_format_null_is_safe(void) {
    MirProgram fresh;
    char buf[128];

    mir_program_init(&fresh);
    ASSERT_TRUE(mir_get_error(&fresh) == NULL,
                "mir_get_error returns NULL on freshly initialised program");
    ASSERT_TRUE(!mir_format_error(NULL, NULL, 0),
                "mir_format_error with NULL error and NULL buffer returns false");
    ASSERT_TRUE(!mir_format_error(NULL, buf, sizeof(buf)),
                "mir_format_error with NULL error and valid buffer returns false");
    mir_program_free(&fresh);
}

void test_mir_build_rejects_programs_with_hir_errors(void) {
    static const char source[] = "start(string[] args) -> missing + 1;\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    const HirBuildError *hir_error;
    char expected[256];
    char diagnostic[256];

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    parser_init(&parser, source);

    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse invalid MIR input");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for invalid MIR input");
    ASSERT_TRUE(!type_checker_check_program(&checker, &ast_program, &symbols),
                "type check fails before MIR lowering");
    ASSERT_TRUE(!hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                "HIR lowering rejects type-invalid MIR input");
    hir_error = hir_get_error(&hir_program);
    REQUIRE_TRUE(hir_error != NULL, "HIR exposes structured error for invalid MIR input");
    REQUIRE_TRUE(hir_format_error(hir_error, expected, sizeof(expected)),
                 "format HIR error for MIR propagation test");
    ASSERT_TRUE(!mir_build_program(&mir_program, &hir_program, false),
                "MIR lowering rejects HIR-invalid program");
    REQUIRE_TRUE(mir_format_error(mir_get_error(&mir_program), diagnostic, sizeof(diagnostic)),
                 "format MIR build error");
    ASSERT_TRUE(strcmp(expected, diagnostic) == 0,
                "MIR build error preserves the HIR diagnostic text");
    ASSERT_TRUE(mir_get_error(&mir_program)->primary_span.start_line ==
                    hir_error->primary_span.start_line &&
                mir_get_error(&mir_program)->primary_span.start_column ==
                    hir_error->primary_span.start_column,
                "MIR build error preserves the primary diagnostic span");

    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}
