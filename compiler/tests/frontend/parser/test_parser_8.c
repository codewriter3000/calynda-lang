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


void test_parse_deref_store_offset_addr_ops(void) {
    static const char source[] =
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
    const AstStartDecl *start_decl;
    const AstStatement *manual_stmt;
    const AstBlock *body;
    const AstExpression *expr;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse deref/store/offset/addr ops");
    ASSERT_TRUE(parser_get_error(&parser) == NULL,
                "no parse error for pointer ops");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    manual_stmt = start_decl->body.as.block->statements[0];
    body = manual_stmt->as.manual.body;
    REQUIRE_TRUE(body != NULL, "pointer ops manual body is not null");
    ASSERT_EQ_INT(6, (int)body->statement_count,
                  "pointer ops manual body has 6 statements");

    /* store(p, 42) is an expression statement */
    expr = body->statements[1]->as.expression;
    ASSERT_EQ_INT(AST_EXPR_MEMORY_OP, expr->kind, "store is memory op");
    ASSERT_EQ_INT(AST_MEMORY_STORE, expr->as.memory_op.kind,
                  "memory op kind is store");
    ASSERT_EQ_INT(2, (int)expr->as.memory_op.arguments.count,
                  "store has 2 arguments");

    /* deref(p) is the initializer of int64 val */
    expr = body->statements[2]->as.local_binding.initializer;
    ASSERT_EQ_INT(AST_EXPR_MEMORY_OP, expr->kind, "deref is memory op");
    ASSERT_EQ_INT(AST_MEMORY_DEREF, expr->as.memory_op.kind,
                  "memory op kind is deref");
    ASSERT_EQ_INT(1, (int)expr->as.memory_op.arguments.count,
                  "deref has 1 argument");

    ast_program_free(&program);
    parser_free(&parser);
}

void test_parse_stackalloc_op(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 buf = stackalloc(128);\n"
        "        store(buf, 7);\n"
        "        free(buf);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstStartDecl *start_decl;
    const AstStatement *manual_stmt;
    const AstBlock *body;
    const AstExpression *expr;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse stackalloc op");
    ASSERT_TRUE(parser_get_error(&parser) == NULL,
                "no parse error for stackalloc");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    manual_stmt = start_decl->body.as.block->statements[0];
    body = manual_stmt->as.manual.body;
    REQUIRE_TRUE(body != NULL, "stackalloc manual body is not null");
    ASSERT_EQ_INT(3, (int)body->statement_count,
                  "stackalloc manual body has 3 statements");

    expr = body->statements[0]->as.local_binding.initializer;
    ASSERT_EQ_INT(AST_EXPR_MEMORY_OP, expr->kind, "stackalloc is memory op");
    ASSERT_EQ_INT(AST_MEMORY_STACKALLOC, expr->as.memory_op.kind,
                  "memory op kind is stackalloc");
    ASSERT_EQ_INT(1, (int)expr->as.memory_op.arguments.count,
                  "stackalloc has 1 argument");

    ast_program_free(&program);
    parser_free(&parser);
}

void test_parse_layout_declaration(void) {
    static const char source[] =
        "layout Point { int32 x; int32 y; };\n"
        "start(string[] args) -> { return 0; };\n";
    Parser parser;
    AstProgram program;
    const AstLayoutDecl *layout;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse layout declaration");
    ASSERT_TRUE(parser_get_error(&parser) == NULL,
                "no parse error for layout");
    REQUIRE_TRUE(program.top_level_count == 2,
                 "layout program has 2 top-level decls");

    ASSERT_EQ_INT(AST_TOP_LEVEL_LAYOUT,
                  program.top_level_decls[0]->kind,
                  "first decl is layout");
    layout = &program.top_level_decls[0]->as.layout_decl;
    ASSERT_TRUE(strcmp(layout->name, "Point") == 0, "layout name is Point");
    ASSERT_EQ_INT(2, (int)layout->field_count, "layout has 2 fields");
    ASSERT_TRUE(strcmp(layout->fields[0].name, "x") == 0, "first field name");
    ASSERT_TRUE(strcmp(layout->fields[1].name, "y") == 0, "second field name");
    ASSERT_EQ_INT(AST_TYPE_PRIMITIVE, layout->fields[0].field_type.kind,
                  "field type is primitive");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, layout->fields[0].field_type.primitive,
                  "field primitive is int32");

    ast_program_free(&program);
    parser_free(&parser);
}
