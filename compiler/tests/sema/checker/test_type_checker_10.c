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


void test_type_checker_accepts_manual_lambda_shorthand(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    int32 adjust = manual(int32 value) -> {\n"
        "        return value + 1;\n"
        "    };\n"
        "    return adjust(41);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse manual lambda shorthand program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for manual lambda shorthand");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "manual lambda shorthand passes type checking");

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


void test_type_checker_accepts_mmio_deref_offset_store(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        mmio<uint32> reg = 0;\n"
        "        store(reg, 42);\n"
        "        uint32 val = deref(offset(reg, 2));\n"
        "        mmio<uint32> next = offset(reg, 2);\n"
        "        int64 a = addr(reg);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const AstBlock *body;
    const AstBlock *manual_body;
    const TypeCheckInfo *deref_info;
    const TypeCheckInfo *offset_info;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse mmio ops program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for mmio ops");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "mmio<uint32> deref/offset/store/addr pass type checking");

    body = program.top_level_decls[0]->as.start_decl.body.as.block;
    manual_body = body->statements[0]->as.manual.body;
    deref_info = type_checker_get_expression_info(
        &checker, manual_body->statements[2]->as.local_binding.initializer);
    offset_info = type_checker_get_expression_info(
        &checker, manual_body->statements[3]->as.local_binding.initializer);
    REQUIRE_TRUE(deref_info != NULL, "mmio deref expression has type info");
    REQUIRE_TRUE(offset_info != NULL, "mmio offset expression has type info");
    ASSERT_EQ_INT(CHECKED_TYPE_VALUE, deref_info->type.kind,
                  "mmio deref yields a value type");
    ASSERT_EQ_INT(AST_PRIMITIVE_UINT32, deref_info->type.primitive,
                  "mmio deref yields uint32");
    ASSERT_EQ_INT(CHECKED_TYPE_NAMED, offset_info->type.kind,
                  "mmio offset preserves named wrapper type");
    ASSERT_TRUE(offset_info->type.name != NULL && strcmp(offset_info->type.name, "mmio") == 0,
                "mmio offset preserves the mmio wrapper");
    ASSERT_TRUE(offset_info->has_first_generic_arg,
                "mmio offset preserves first generic argument metadata");
    ASSERT_EQ_INT(AST_PRIMITIVE_UINT32, offset_info->first_generic_arg_type.primitive,
                  "mmio offset preserves uint32 element type");

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_checker_accepts_mmio_value_member_rmw(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        mmio<uint32> reg = 0;\n"
        "        offset(reg, 2).value = 99;\n"
        "        offset(reg, 2).value += 1;\n"
        "        offset(reg, 2).value++;\n"
        "        uint32 v = offset(reg, 2).value;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    const AstBlock *body;
    const AstBlock *manual_body;
    const TypeCheckInfo *value_info;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse mmio<T>.value program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build symbols for mmio<T>.value program");
    ASSERT_TRUE(type_checker_check_program(&checker, &program, &symbols),
                "mmio<T>.value supports load/store/RMW");

    body = program.top_level_decls[0]->as.start_decl.body.as.block;
    manual_body = body->statements[0]->as.manual.body;
    value_info = type_checker_get_expression_info(
        &checker, manual_body->statements[4]->as.local_binding.initializer);
    REQUIRE_TRUE(value_info != NULL, "mmio<T>.value expression has type info");
    ASSERT_EQ_INT(CHECKED_TYPE_VALUE, value_info->type.kind,
                  "mmio<T>.value yields a scalar type");
    ASSERT_EQ_INT(AST_PRIMITIVE_UINT32, value_info->type.primitive,
                  "mmio<T>.value yields the wrapped element type");

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
    const AstBlock *body;
    const AstBlock *manual_body;
    const TypeCheckInfo *deref_info;
    const TypeCheckInfo *offset_info;

#include "test_type_checker_10_p2.inc"
