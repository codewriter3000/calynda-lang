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


/* ------------------------------------------------------------------ */
/*  G-PAR-1: Malformed input produces error, no crash                 */
/* ------------------------------------------------------------------ */

void test_parse_malformed_unclosed_brace(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    return 0;\n";
    Parser parser;
    AstProgram program;
    bool result;

    parser_init(&parser, source);
    result = parser_parse_program(&parser, &program);
    ASSERT_TRUE(!result, "unclosed brace rejects program");
    ASSERT_TRUE(parser_get_error(&parser) != NULL,
                "unclosed brace reports parser error");
    ASSERT_TRUE(parser_get_error(&parser)->token.line > 0,
                "unclosed brace error has valid line number");
    if (result) ast_program_free(&program);
    parser_free(&parser);
}

void test_parse_malformed_missing_semicolon(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    var x = 1\n"
        "    return x;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    bool result;

    parser_init(&parser, source);
    result = parser_parse_program(&parser, &program);
    ASSERT_TRUE(!result, "missing semicolon rejects program");
    ASSERT_TRUE(parser_get_error(&parser) != NULL,
                "missing semicolon reports parser error");
    if (result) ast_program_free(&program);
    parser_free(&parser);
}

void test_parse_malformed_stray_token(void) {
    static const char source[] =
        "@@@ start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    bool result;

    parser_init(&parser, source);
    result = parser_parse_program(&parser, &program);
    ASSERT_TRUE(!result, "stray tokens reject program");
    ASSERT_TRUE(parser_get_error(&parser) != NULL,
                "stray tokens report parser error");
    if (result) ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/*  G-PAR-7: manual checked block parses distinctly                   */
/* ------------------------------------------------------------------ */

void test_parse_manual_checked_block(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual checked {\n"
        "        int64 p = malloc(64);\n"
        "        free(p);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstStartDecl *start_decl;
    const AstStatement *manual_stmt;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse manual checked block");
    ASSERT_TRUE(parser_get_error(&parser) == NULL,
                "no parse error for manual checked");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    manual_stmt = start_decl->body.as.block->statements[0];
    ASSERT_EQ_INT(AST_STMT_MANUAL, manual_stmt->kind,
                  "manual checked is AST_STMT_MANUAL");
    ASSERT_TRUE(manual_stmt->as.manual.is_checked,
                "manual checked block has is_checked true");

    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/*  G-PAR-3: Union with multiple type params and variants             */
/* ------------------------------------------------------------------ */

void test_parse_union_multi_generic(void) {
    static const char source[] =
        "union Result<T, E> { Ok(T), Err(E), Unknown };\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    const AstUnionDecl *union_decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse union with multiple generics");
    ASSERT_TRUE(parser_get_error(&parser) == NULL,
                "no parse error for multi-generic union");
    REQUIRE_TRUE(program.top_level_count >= 2,
                 "multi-generic union has 2+ top-level decls");

    ASSERT_EQ_INT(AST_TOP_LEVEL_UNION,
                  program.top_level_decls[0]->kind,
                  "first decl is union");
    union_decl = &program.top_level_decls[0]->as.union_decl;
    ASSERT_TRUE(strcmp(union_decl->name, "Result") == 0,
                "union name is Result");
    ASSERT_EQ_INT(2, (int)union_decl->generic_param_count,
                  "union has 2 generic params");
    ASSERT_EQ_INT(3, (int)union_decl->variant_count,
                  "union has 3 variants");

    ast_program_free(&program);
    parser_free(&parser);
}

void test_parse_type_alias_and_spawn_threading(void) {
    static const char source[] =
        "export type Grid = float64[][];\n"
        "start(string[] args) -> {\n"
        "    Thread t = spawn () -> {\n"
        "    };\n"
        "    Mutex m = Mutex.new();\n"
        "    t.join();\n"
        "    m.lock();\n"
        "    m.unlock();\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstTopLevelDecl *alias_decl;
    const AstStatement *thread_stmt;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse type alias + threading");
    ASSERT_EQ_INT(AST_TOP_LEVEL_TYPE_ALIAS, program.top_level_decls[0]->kind,
                  "first decl is type alias");
    alias_decl = program.top_level_decls[0];
    ASSERT_TRUE(strcmp(alias_decl->as.type_alias_decl.name, "Grid") == 0,
                "type alias name is Grid");
    ASSERT_EQ_INT(AST_TYPE_PRIMITIVE, alias_decl->as.type_alias_decl.target_type.kind,
                  "alias target root is primitive array");
    ASSERT_EQ_INT(2, (int)alias_decl->as.type_alias_decl.target_type.dimension_count,
                  "alias target preserves array depth");

    thread_stmt = program.top_level_decls[1]->as.start_decl.body.as.block->statements[0];
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, thread_stmt->kind, "thread binding parses");
    ASSERT_EQ_INT(AST_TYPE_THREAD, thread_stmt->as.local_binding.declared_type.kind,
                  "Thread parses as built-in type");
    ASSERT_EQ_INT(AST_EXPR_SPAWN, thread_stmt->as.local_binding.initializer->kind,
                  "spawn parses as dedicated expression");

    ast_program_free(&program);
    parser_free(&parser);
}
