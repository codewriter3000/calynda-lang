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


/* ------------------------------------------------------------------ */
/* V2 feature: tagged union declaration                               */
/* ------------------------------------------------------------------ */
void test_parse_union_declaration(void) {
    const char *source = "union Option<T> { Some(int32), None };\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse union decl");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for union");
    ASSERT_EQ_INT(1, program.top_level_count, "one top-level decl");
    ASSERT_EQ_INT(AST_TOP_LEVEL_UNION, program.top_level_decls[0]->kind,
                  "union kind");
    ASSERT_EQ_STR("Option", program.top_level_decls[0]->as.union_decl.name,
                  "union name");
    ASSERT_EQ_INT(1, program.top_level_decls[0]->as.union_decl.generic_param_count,
                  "union generic param count");
    ASSERT_EQ_INT(2, program.top_level_decls[0]->as.union_decl.variant_count,
                  "union variant count");
    ASSERT_EQ_STR("Some",
                  program.top_level_decls[0]->as.union_decl.variants[0].name,
                  "first variant name");
    ASSERT_TRUE(program.top_level_decls[0]->as.union_decl.variants[0].payload_type != NULL,
                "first variant has payload");
    ASSERT_EQ_STR("None",
                  program.top_level_decls[0]->as.union_decl.variants[1].name,
                  "second variant name");
    ASSERT_TRUE(program.top_level_decls[0]->as.union_decl.variants[1].payload_type == NULL,
                "second variant has no payload");
    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_union_with_modifiers(void) {
    const char *source =
        "export union Result<T, E> {\n"
        "    Ok(int32),\n"
        "    Err(string)\n"
        "};\n";
    Parser parser;
    AstProgram program;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse export union");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for export union");
    ASSERT_EQ_INT(1, program.top_level_count, "one top-level decl");
    ASSERT_EQ_INT(AST_TOP_LEVEL_UNION, program.top_level_decls[0]->kind,
                  "union kind");
    ASSERT_EQ_STR("Result", program.top_level_decls[0]->as.union_decl.name,
                  "union name");
    ASSERT_EQ_INT(1, program.top_level_decls[0]->as.union_decl.modifier_count,
                  "union modifier count");
    ASSERT_EQ_INT(AST_MODIFIER_EXPORT,
                  program.top_level_decls[0]->as.union_decl.modifiers[0],
                  "union export modifier");
    ASSERT_EQ_INT(2, program.top_level_decls[0]->as.union_decl.generic_param_count,
                  "union generic param count");
    ASSERT_EQ_INT(2, program.top_level_decls[0]->as.union_decl.variant_count,
                  "union variant count");

    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_internal_local_binding(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    internal void doBreak = () -> { return; };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstStartDecl *start_decl;
    const AstStatement *stmt;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse internal local binding");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for internal local");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    REQUIRE_TRUE(start_decl->body.kind == AST_LAMBDA_BODY_BLOCK,
                 "start has block body");
    REQUIRE_TRUE(start_decl->body.as.block->statement_count >= 1,
                 "start block has statements");

    stmt = start_decl->body.as.block->statements[0];
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, stmt->kind, "first stmt is local binding");
    ASSERT_TRUE(stmt->as.local_binding.is_internal, "local binding is_internal");
    ASSERT_EQ_STR("doBreak", stmt->as.local_binding.name, "local binding name");

    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_named_type_in_binding(void) {
    const char *source =
        "union Option<T> { Some(T), None };\n"
        "Option<int32> val = 42;\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    const AstBindingDecl *binding;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse named type binding");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for named type");

    ASSERT_EQ_INT(3, (int)program.top_level_count, "three top-level decls");

    binding = &program.top_level_decls[1]->as.binding_decl;
    ASSERT_EQ_INT(AST_TYPE_NAMED, binding->declared_type.kind, "binding has named type");
    ASSERT_EQ_STR("Option", binding->declared_type.name, "named type is Option");
    ASSERT_EQ_INT(1, (int)binding->declared_type.generic_args.count, "one generic arg");
    ASSERT_EQ_INT(AST_GENERIC_ARG_TYPE,
                  binding->declared_type.generic_args.items[0].kind,
                  "generic arg is a type");
    ASSERT_EQ_INT(AST_TYPE_PRIMITIVE,
                  binding->declared_type.generic_args.items[0].type->kind,
                  "generic arg type is primitive");

    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_arr_wildcard_type(void) {
    const char *source =
        "arr<?> record = [1, \"hello\"];\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    const AstBindingDecl *binding;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse arr wildcard");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for arr<?>") ;

    binding = &program.top_level_decls[0]->as.binding_decl;
    ASSERT_EQ_INT(AST_TYPE_ARR, binding->declared_type.kind, "binding has arr type");
    ASSERT_EQ_INT(1, (int)binding->declared_type.generic_args.count, "one generic arg");
    ASSERT_EQ_INT(AST_GENERIC_ARG_WILDCARD,
                  binding->declared_type.generic_args.items[0].kind,
                  "generic arg is wildcard");

    ast_program_free(&program);
    parser_free(&parser);
}

