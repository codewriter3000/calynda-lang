#include "parser.h"
#include "symbol_table.h"
#include "type_checker.h"

#include <stdio.h>
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


void test_type_checker_accepts_manual_block_with_memory_ops(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 mem = malloc(1024);\n"
        "        mem = realloc(mem, 2048);\n"
        "        free(mem);\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse manual block program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for manual block");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "manual block with memory ops passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_calloc_memory_op(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 mem = calloc(10, 8);\n"
        "        free(mem);\n"
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
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse calloc program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for calloc");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "calloc memory op passes type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_manual_pointer_ops(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 p = malloc(64);\n"
        "        store(p, 42);\n"
        "        int64 val = deref(p);\n"
        "        int64 q = offset(p, 2);\n"
        "        int64 a = addr(p);\n"
        "        free(p);\n"
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
                 "parse manual pointer ops program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for manual pointer ops");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "manual pointer ops pass type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_typed_ptr_deref_offset_store(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        ptr<int32> p = malloc(16);\n"
        "        store(p, 42);\n"
        "        int64 val = deref(p);\n"
        "        ptr<int32> q = offset(p, 2);\n"
        "        int64 a = addr(p);\n"
        "        free(p);\n"
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
                 "parse typed ptr ops program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for typed ptr ops");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "typed ptr<int32> deref/offset/store/addr pass type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_typed_ptr_int8_ops(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        ptr<int8> p = malloc(4);\n"
        "        store(p, 1);\n"
        "        int64 v = deref(p);\n"
        "        ptr<int8> q = offset(p, 3);\n"
        "        free(p);\n"
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
                 "parse typed ptr<int8> ops program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for typed ptr<int8> ops");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "typed ptr<int8> ops pass type checking");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

