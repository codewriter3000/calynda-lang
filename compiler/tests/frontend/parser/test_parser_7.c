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
        "        int64 ptr = malloc(1024);\n"
        "        ptr = realloc(ptr, 2048);\n"
        "        free(ptr);\n"
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

    /* First statement: int64 ptr = malloc(1024) */
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, body->statements[0]->kind,
                  "manual body stmt 0 is local binding");
    ASSERT_EQ_INT(AST_EXPR_MEMORY_OP,
                  body->statements[0]->as.local_binding.initializer->kind,
                  "malloc is a memory op expression");
    ASSERT_EQ_INT(AST_MEMORY_MALLOC,
                  body->statements[0]->as.local_binding.initializer->as.memory_op.kind,
                  "memory op kind is malloc");

    /* Third statement: free(ptr) */
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
        "        int64 ptr = calloc(10, 8);\n"
        "        free(ptr);\n"
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

