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


void test_hir_dump_lowers_typed_program_to_normalized_blocks(void) {
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


void test_hir_dump_normalizes_exit_and_keeps_external_callable_metadata(void) {
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

