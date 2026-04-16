#include "parser.h"

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


void test_parse_manual_statement_with_memory_ops(void) {
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
    const AstStartDecl *start_decl;
    const AstStatement *manual_stmt;
    const AstBlock *body;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse manual statement");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for manual");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    manual_stmt = start_decl->body.as.block->statements[0];
    ASSERT_EQ_INT(AST_STMT_MANUAL, manual_stmt->kind, "first stmt is manual");

    body = manual_stmt->as.manual.body;
    REQUIRE_TRUE(body != NULL, "manual body is not null");
    ASSERT_EQ_INT(3, (int)body->statement_count, "manual body has 3 statements");

    /* First statement: int64 mem = malloc(1024) */
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, body->statements[0]->kind,
                  "manual body stmt 0 is local binding");
    ASSERT_EQ_INT(AST_EXPR_MEMORY_OP,
                  body->statements[0]->as.local_binding.initializer->kind,
                  "malloc is a memory op expression");
    ASSERT_EQ_INT(AST_MEMORY_MALLOC,
                  body->statements[0]->as.local_binding.initializer->as.memory_op.kind,
                  "memory op kind is malloc");

    /* Third statement: free(mem) */
    ASSERT_EQ_INT(AST_STMT_EXPRESSION, body->statements[2]->kind,
                  "manual body stmt 2 is expression");
    ASSERT_EQ_INT(AST_EXPR_MEMORY_OP,
                  body->statements[2]->as.expression->kind,
                  "free is a memory op expression");
    ASSERT_EQ_INT(AST_MEMORY_FREE,
                  body->statements[2]->as.expression->as.memory_op.kind,
                  "memory op kind is free");

    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_calloc_memory_op(void) {
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
    const AstStartDecl *start_decl;
    const AstStatement *manual_stmt;
    const AstBlock *body;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse calloc manual");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for calloc");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    manual_stmt = start_decl->body.as.block->statements[0];
    body = manual_stmt->as.manual.body;
    REQUIRE_TRUE(body != NULL, "calloc manual body is not null");

    ASSERT_EQ_INT(AST_EXPR_MEMORY_OP,
                  body->statements[0]->as.local_binding.initializer->kind,
                  "calloc is a memory op expression");
    ASSERT_EQ_INT(AST_MEMORY_CALLOC,
                  body->statements[0]->as.local_binding.initializer->as.memory_op.kind,
                  "memory op kind is calloc");

    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_ptr_type_in_manual(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        ptr<int32> p = malloc(4);\n"
        "        free(p);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstStartDecl *start_decl;
    const AstStatement *manual_stmt;
    const AstBlock *body;
    const AstType *decl_type;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse ptr type in manual");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for ptr type");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    manual_stmt = start_decl->body.as.block->statements[0];
    body = manual_stmt->as.manual.body;
    REQUIRE_TRUE(body != NULL, "ptr manual body is not null");
    ASSERT_EQ_INT(2, (int)body->statement_count, "ptr manual body has 2 statements");

    decl_type = &body->statements[0]->as.local_binding.declared_type;
    ASSERT_EQ_INT(AST_TYPE_PTR, decl_type->kind, "declared type is ptr");
    ASSERT_EQ_INT(1, (int)decl_type->generic_args.count, "ptr has 1 generic arg");
    ASSERT_EQ_INT(AST_GENERIC_ARG_TYPE, decl_type->generic_args.items[0].kind,
                  "ptr generic arg is a type");
    ASSERT_EQ_INT(AST_TYPE_PRIMITIVE, decl_type->generic_args.items[0].type->kind,
                  "ptr generic arg is primitive");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32,
                  decl_type->generic_args.items[0].type->primitive,
                  "ptr generic arg is int32");

    ast_program_free(&program);
    parser_free(&parser);
}


