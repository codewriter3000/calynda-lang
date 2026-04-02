#include "../src/hir.h"
#include "../src/hir_dump.h"
#include "../src/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void test_hir_dump_lowers_typed_program_to_normalized_blocks(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> add((1), 2);\n";
    static const char expected[] =
        "HirProgram\n"
        "  Package: <none>\n"
        "  Imports: []\n"
        "  TopLevel:\n"
        "    Binding name=add type=int32 final=false callable=(int32, int32) -> int32 span=1:7\n"
        "      Init:\n"
        "        Lambda type=int32 callable=(int32, int32) -> int32 span=1:13\n"
        "          Parameters:\n"
        "            Param name=left type=int32 span=1:20\n"
        "            Param name=right type=int32 span=1:32\n"
        "          Body:\n"
        "            Block statements=1\n"
        "              Return span=1:42\n"
        "                Binary op=+ type=int32 span=1:42\n"
        "                  Left:\n"
        "                    Symbol name=left kind=parameter type=int32 span=1:42\n"
        "                  Right:\n"
        "                    Symbol name=right kind=parameter type=int32 span=1:49\n"
        "    Start span=2:1\n"
        "      Parameters:\n"
        "        Param name=args type=string[] span=2:16\n"
        "      Body:\n"
        "        Block statements=1\n"
        "          Return span=2:25\n"
        "            Call type=int32 span=2:25\n"
        "              Callee:\n"
        "                Symbol name=add kind=top-level binding type=int32 callable=(int32, int32) -> int32 span=2:25\n"
        "              Arg 1:\n"
        "                Literal kind=0 type=int32 text=1 span=2:30\n"
        "              Arg 2:\n"
        "                Literal kind=0 type=int32 text=2 span=2:34\n";
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse program for HIR dump test");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for HIR dump test");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check program for HIR dump test");
    REQUIRE_TRUE(hir_build_program(&hir_program, &program, &symbols, &checker),
                 "lower program to HIR");

    dump = hir_dump_program_to_string(&hir_program);
    REQUIRE_TRUE(dump != NULL, "render HIR dump to string");
    ASSERT_EQ_STR(expected, dump, "HIR dump string");

    free(dump);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_hir_dump_normalizes_exit_and_keeps_external_callable_metadata(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "void printer = stdlib.print;\n"
        "void cleanup = () -> { exit; };\n"
        "start(string[] args) -> { cleanup(); return 0; };\n";
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
                 "parse HIR expansion program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for HIR expansion program");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check HIR expansion program");
    REQUIRE_TRUE(hir_build_program(&hir_program, &program, &symbols, &checker),
                 "lower HIR expansion program");

    dump = hir_dump_program_to_string(&hir_program);
    REQUIRE_TRUE(dump != NULL, "render HIR expansion dump to string");
    ASSERT_CONTAINS("Binding name=printer type=void final=false callable=(...) -> void",
                    dump,
                    "external callable binding keeps callable signature");
    ASSERT_CONTAINS("Member name=print type=<external> callable=(...) -> <external>",
                    dump,
                    "external member expression keeps callable signature");
    ASSERT_CONTAINS("Binding name=cleanup type=void final=false callable=() -> void",
                    dump,
                    "zero-arg lambda binding keeps exact callable signature");
    ASSERT_CONTAINS("Return span=3:24\n",
                    dump,
                    "exit lowers to return in HIR");
    ASSERT_TRUE(strstr(dump, "Exit span=") == NULL,
                "exit syntax does not survive into HIR dump");
    ASSERT_CONTAINS("Symbol name=cleanup kind=top-level binding type=void callable=() -> void",
                    dump,
                    "callable symbol references retain signature metadata");

    free(dump);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_hir_dump_covers_templates_casts_arrays_assignments_and_nested_lambdas(void) {
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

static void test_hir_builder_rejects_programs_with_type_errors(void) {
    static const char source[] = "start(string[] args) -> missing + 1;\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    char diagnostic[256];
    char *diagnostic_text = diagnostic;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse invalid HIR input program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for invalid HIR input");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "type check fails before HIR lowering");
    ASSERT_TRUE(!hir_build_program(&hir_program, &program, &symbols, &checker),
                "HIR lowering rejects type-invalid program");
    REQUIRE_TRUE(hir_format_error(hir_get_error(&hir_program), diagnostic, sizeof(diagnostic)),
                 "format HIR build error");
    ASSERT_EQ_STR("Cannot lower program to HIR while the type checker reports errors.",
                  diagnostic_text,
                  "formatted HIR build precondition error");

    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

int main(void) {
    printf("Running HIR dump tests...\n\n");

    RUN_TEST(test_hir_dump_lowers_typed_program_to_normalized_blocks);
    RUN_TEST(test_hir_dump_normalizes_exit_and_keeps_external_callable_metadata);
    RUN_TEST(test_hir_dump_covers_templates_casts_arrays_assignments_and_nested_lambdas);
    RUN_TEST(test_hir_builder_rejects_programs_with_type_errors);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}