#include "hir.h"
#include "hir_dump.h"
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
#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)


void test_hir_dump_covers_templates_casts_arrays_assignments_and_nested_lambdas(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    var values = [1, 2, 3];\n"
        "    var chooser = (bool flag) -> flag ? values[1] : int32(3.5);\n"
        "    var render = (int32 x) -> `value ${chooser(true) + ((int32 y) -> y + x)(1)}`;\n"
        "    values[0] = chooser(false);\n"
        "    return chooser(true);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse broad HIR coverage program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for broad HIR coverage program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check broad HIR coverage program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &program, &symbols, &checker),
                 "lower broad HIR coverage program");

    dump = hir_dump_program_to_string(&hir_program);
    REQUIRE_TRUE(dump != NULL, "render broad HIR coverage dump to string");
    ASSERT_CONTAINS("Local name=values type=int32[] final=false span=2:9",
                    dump,
                    "array local appears in HIR dump");
    ASSERT_CONTAINS("ArrayLiteral type=int32[] span=2:18",
                    dump,
                    "array literal lowering is covered");
    ASSERT_CONTAINS("Local name=chooser type=int32 callable=(bool) -> int32 final=false span=3:9",
                    dump,
                    "callable local binding keeps signature metadata");
    ASSERT_CONTAINS("Ternary type=int32 span=3:34",
                    dump,
                    "ternary lowering is covered");
    ASSERT_CONTAINS("Cast target=int32 span=3:53",
                    dump,
                    "cast lowering is covered");
    ASSERT_CONTAINS("Local name=render type=string callable=(int32) -> string final=false span=4:9",
                    dump,
                    "template-producing callable local keeps signature metadata");
    ASSERT_CONTAINS("Template type=string span=4:31",
                    dump,
                    "template lowering is covered");
    ASSERT_CONTAINS("ExpressionPart",
                    dump,
                    "template interpolation lowering is covered");
    ASSERT_CONTAINS("Lambda type=int32 callable=(int32) -> int32 span=4:57",
                    dump,
                    "nested lambda lowering is covered");
    ASSERT_CONTAINS("Assignment op=0 type=int32 span=5:5",
                    dump,
                    "assignment lowering is covered");
    ASSERT_CONTAINS("Symbol name=chooser kind=local type=int32 callable=(bool) -> int32",
                    dump,
                    "callable local references keep signature metadata");

    free(dump);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_hir_builder_rejects_programs_with_type_errors(void) {
    static const char source[] = "start(string[] args) -> missing + 1;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    const TypeCheckError *type_error;
    char diagnostic[256];
    char expected[256];

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse invalid HIR input program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for invalid HIR input");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "type check fails before HIR lowering");
    type_error = type_checker_get_error(&checker);
    REQUIRE_TRUE(type_error != NULL, "type checker exposes structured error for invalid HIR input");
    REQUIRE_TRUE(type_checker_format_error(type_error, expected, sizeof(expected)),
                 "format type checker error for invalid HIR input");
    ASSERT_TRUE(!hir_build_program(&hir_program, &program, &symbols, &checker),
                "HIR lowering rejects type-invalid program");
    REQUIRE_TRUE(hir_format_error(hir_get_error(&hir_program), diagnostic, sizeof(diagnostic)),
                 "format HIR build error");
    ASSERT_TRUE(strcmp(expected, diagnostic) == 0,
                "HIR build error preserves the type checker diagnostic text");
    ASSERT_TRUE(hir_get_error(&hir_program)->primary_span.start_line ==
                    type_error->primary_span.start_line &&
                hir_get_error(&hir_program)->primary_span.start_column ==
                    type_error->primary_span.start_column,
                "HIR build error preserves the primary diagnostic span");

    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2: post ++/-- , discard, varargs in HIR dump                       */
/* ------------------------------------------------------------------ */

void test_hir_dump_lowers_v2_expressions(void) {
    static const char source[] =
        "int32 counter = () -> 0;\n"
        "int32 inc = (int32... nums) -> 0;\n"
        "start(string[] args) -> {\n"
        "    var c = counter();\n"
        "    c++;\n"
        "    c--;\n"
        "    _ = inc(1, 2, 3);\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse V2 HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols V2 HIR");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check V2 HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for V2 expressions");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "HIR dump string is not NULL");

    ASSERT_CONTAINS("PostIncrement", dump, "HIR dump contains PostIncrement");
    ASSERT_CONTAINS("PostDecrement", dump, "HIR dump contains PostDecrement");
    ASSERT_CONTAINS("Discard", dump, "HIR dump contains Discard");
    ASSERT_CONTAINS("varargs", dump, "HIR dump contains varargs");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
