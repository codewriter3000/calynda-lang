#include "ast.h"
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
/*  G-AST-2: Source-span fidelity on parsed nodes                     */
/* ------------------------------------------------------------------ */

void test_ast_source_spans(void) {
    static const char source[] =
        "int32 add = (int32 a, int32 b) -> a + b;\n"
        "start(string[] args) -> {\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstTopLevelDecl *binding;
    const AstTopLevelDecl *start;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse program for span test");

    /* binding on line 1 */
    binding = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_BINDING, binding->kind, "first decl is binding");
    ASSERT_EQ_INT(1, binding->as.binding_decl.name_span.start_line,
                  "binding name span starts on line 1");
    ASSERT_TRUE(binding->as.binding_decl.name_span.start_column > 0,
                "binding name span has valid column");

    /* start on line 2 */
    start = program.top_level_decls[1];
    ASSERT_EQ_INT(AST_TOP_LEVEL_START, start->kind, "second decl is start");
    ASSERT_EQ_INT(2, start->as.start_decl.start_span.start_line,
                  "start span starts on line 2");

    /* return statement on line 3 */
    REQUIRE_TRUE(start->as.start_decl.body.as.block != NULL,
                 "start has block body");
    ASSERT_EQ_INT(3, start->as.start_decl.body.as.block->statements[0]->source_span.start_line,
                  "return statement span on line 3");

    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/*  G-AST-3: Modifier flags survive parse-to-AST                     */
/* ------------------------------------------------------------------ */

void test_ast_modifier_flags(void) {
    static const char source[] =
        "export int32 pub_val = 42;\n"
        "static int32 stat_val = 7;\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    const AstBindingDecl *export_binding;
    const AstBindingDecl *static_binding;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse program with modifiers");
    REQUIRE_TRUE(program.top_level_count == 3,
                 "modifier program has 3 top-level decls");

    export_binding = &program.top_level_decls[0]->as.binding_decl;
    ASSERT_TRUE(export_binding->modifier_count >= 1,
                "export binding has at least one modifier");
    ASSERT_EQ_INT(AST_MODIFIER_EXPORT, export_binding->modifiers[0],
                  "export binding first modifier is EXPORT");

    static_binding = &program.top_level_decls[1]->as.binding_decl;
    ASSERT_TRUE(static_binding->modifier_count >= 1,
                "static binding has at least one modifier");
    ASSERT_EQ_INT(AST_MODIFIER_STATIC, static_binding->modifiers[0],
                  "static binding first modifier is STATIC");

    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/*  G-AST-4: Union AST node                                          */
/* ------------------------------------------------------------------ */

void test_ast_union_node(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    const AstUnionDecl *union_decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse program with union");
    ASSERT_EQ_INT(AST_TOP_LEVEL_UNION, program.top_level_decls[0]->kind,
                  "first decl is union");
    union_decl = &program.top_level_decls[0]->as.union_decl;
    ASSERT_TRUE(strcmp(union_decl->name, "Option") == 0, "union name is Option");
    ASSERT_EQ_INT(1, (int)union_decl->generic_param_count, "union has 1 type param");
    ASSERT_EQ_INT(2, (int)union_decl->variant_count, "union has 2 variants");

    ast_program_free(&program);
    parser_free(&parser);
}
