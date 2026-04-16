#include "parser.h"
#include "symbol_table.h"
#include "type_checker.h"

#include <stdio.h>
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
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {      \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n", __FILE__, __LINE__, (msg)); \
    }                                                                       \
} while (0)


void test_type_checker_accepts_stackalloc_op(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 buf = stackalloc(128);\n"
        "        store(buf, 99);\n"
        "        int64 v = deref(buf);\n"
        "        free(buf);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse stackalloc program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for stackalloc");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "stackalloc passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_stackalloc_non_integral_arg(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 buf = stackalloc(true);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse stackalloc bad-arg program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for stackalloc bad-arg");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "stackalloc with bool arg rejected by type checker");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_layout_declaration(void) {
    static const char source[] =
        "layout Point { int32 x; int32 y; };\n"
        "start(string[] args) -> { return 0; };\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse layout declaration program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for layout");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "layout declaration accepted by type checker");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_non_primitive_layout_field(void) {
    static const char source[] =
        "layout Node { int32 value; Node next; };\n"
        "start(string[] args) -> { return 0; };\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse layout with named field type");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for layout with named field type");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "layout fields reject non-primitive types");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format non-primitive layout field diagnostic");
    ASSERT_CONTAINS("must use a scalar primitive type in 0.4.0",
                    diagnostic,
                    "layout restriction diagnostic is explicit");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_accepts_hetero_array_index_reads(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true, \"hello\"];\n"
        "    stdlib.print(string(mixed[2]));\n"
        "    return int32(mixed[0]);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse hetero array indexing program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for hetero array indexing");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "hetero array indexing reads type check as external values");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_checker_rejects_hetero_array_index_writes(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true, \"hello\"];\n"
        "    mixed[0] = 7;\n"
        "    return 0;\n"
        "};\n";
    char diagnostic[256];
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse hetero array write program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for hetero array write");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "hetero array indexing stays read-only in type checking");
    REQUIRE_TRUE(type_checker_format_error(type_checker_get_error(&checker),
                                           diagnostic,
                                           sizeof(diagnostic)),
                 "format hetero array write diagnostic");
    ASSERT_CONTAINS("shape-locked after literal construction",
                    diagnostic,
                    "hetero array writes report locked-shape semantics");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

