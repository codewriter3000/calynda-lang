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


void test_parse_nested_generic_rshift(void) {
    const char *source =
        "union Box<T> { Val(T), Empty };\n"
        "union Option<T> { Some(T), None };\n"
        "Option<Box<int32>> nested = 42;\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    const AstBindingDecl *binding;
    const AstGenericArg *outer_arg;
    const AstGenericArg *inner_arg;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse nested>>generic");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for nested>>generic");

    binding = &program.top_level_decls[2]->as.binding_decl;
    ASSERT_EQ_INT(AST_TYPE_NAMED, binding->declared_type.kind, "outer named type");
    ASSERT_EQ_STR("Option", binding->declared_type.name, "outer is Option");
    ASSERT_EQ_INT(1, (int)binding->declared_type.generic_args.count, "one outer generic arg");

    outer_arg = &binding->declared_type.generic_args.items[0];
    ASSERT_EQ_INT(AST_GENERIC_ARG_TYPE, outer_arg->kind, "outer arg is type");
    ASSERT_EQ_INT(AST_TYPE_NAMED, outer_arg->type->kind, "inner is named type");
    ASSERT_EQ_STR("Box", outer_arg->type->name, "inner named type is Box");
    ASSERT_EQ_INT(1, (int)outer_arg->type->generic_args.count, "one inner generic arg");

    inner_arg = &outer_arg->type->generic_args.items[0];
    ASSERT_EQ_INT(AST_GENERIC_ARG_TYPE, inner_arg->kind, "inner arg is type");
    ASSERT_EQ_INT(AST_TYPE_PRIMITIVE, inner_arg->type->kind, "innermost is primitive");

    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_named_type_in_parameter(void) {
    const char *source =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<string> x = args;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstStatement *stmt;
    const AstStartDecl *start_decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse named type in local");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error");

    start_decl = &program.top_level_decls[1]->as.start_decl;
    stmt = start_decl->body.as.block->statements[0];
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, stmt->kind, "local binding stmt");
    ASSERT_EQ_INT(AST_TYPE_NAMED, stmt->as.local_binding.declared_type.kind,
                  "local has named type");
    ASSERT_EQ_STR("Option", stmt->as.local_binding.declared_type.name,
                  "local named type is Option");

    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_asm_decl(void) {
    const char *source =
        "int32 my_add = asm(int32 a, int32 b) -> {\n"
        "    mov eax, edi\n"
        "    add eax, esi\n"
        "    ret\n"
        "};\n"
        "start(string[] args) -> my_add(1, 2);\n";
    Parser parser;
    AstProgram program;
    const AstTopLevelDecl *decl;
    const AstAsmDecl *asm_decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse asm decl");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for asm decl");
    ASSERT_EQ_INT(2, (int)program.top_level_count, "two top-level decls (asm + start)");

    decl = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_ASM, (int)decl->kind, "first decl is asm");
    asm_decl = &decl->as.asm_decl;
    ASSERT_EQ_STR("my_add", asm_decl->name, "asm decl name is my_add");
    ASSERT_EQ_INT(AST_TYPE_PRIMITIVE, (int)asm_decl->return_type.kind, "asm return type is primitive");
    ASSERT_EQ_INT(2, (int)asm_decl->parameters.count, "asm has 2 parameters");
    ASSERT_EQ_STR("a", asm_decl->parameters.items[0].name, "asm param 1 name");
    ASSERT_EQ_STR("b", asm_decl->parameters.items[1].name, "asm param 2 name");
    ASSERT_TRUE(asm_decl->body != NULL, "asm body is not null");
    ASSERT_TRUE(asm_decl->body_length > 0, "asm body has non-zero length");
    ASSERT_TRUE(strstr(asm_decl->body, "mov eax, edi") != NULL, "asm body contains mov instruction");
    ASSERT_TRUE(strstr(asm_decl->body, "add eax, esi") != NULL, "asm body contains add instruction");

    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_asm_decl_with_modifiers(void) {
    const char *source =
        "export int32 my_fn = asm() -> {\n"
        "    xor eax, eax\n"
        "    ret\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const AstTopLevelDecl *decl;
    const AstAsmDecl *asm_decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse asm decl with modifiers");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for asm decl with modifiers");
    ASSERT_EQ_INT(1, (int)program.top_level_count, "one top-level decl");

    decl = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_ASM, (int)decl->kind, "decl is asm");
    asm_decl = &decl->as.asm_decl;
    ASSERT_EQ_STR("my_fn", asm_decl->name, "asm decl name is my_fn");
    ASSERT_EQ_INT(1, (int)asm_decl->modifier_count, "asm has 1 modifier");
    ASSERT_EQ_INT(AST_MODIFIER_EXPORT, (int)asm_decl->modifiers[0], "modifier is export");
    ASSERT_EQ_INT(0, (int)asm_decl->parameters.count, "asm has 0 parameters");

    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/*  Test: boot declaration                                            */
/* ------------------------------------------------------------------ */

void test_parse_boot_decl(void) {
    const char *source =
        "boot() -> {\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    AstTopLevelDecl *decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse boot program");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error for boot decl");
    ASSERT_EQ_INT(1, (int)program.top_level_count, "one top-level decl");

    decl = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_START, (int)decl->kind, "boot decl uses AST_TOP_LEVEL_START");
    ASSERT_TRUE(decl->as.start_decl.is_boot, "is_boot flag is set");
    ASSERT_EQ_INT(0, (int)decl->as.start_decl.parameters.count, "boot has zero parameters");
    ASSERT_EQ_INT(AST_LAMBDA_BODY_BLOCK, (int)decl->as.start_decl.body.kind, "boot body is block");

    ast_program_free(&program);
    parser_free(&parser);
}


void test_parse_boot_decl_expression_body(void) {
    const char *source = "boot() -> 42;\n";
    Parser parser;
    AstProgram program;
    AstTopLevelDecl *decl;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse boot expression body");
    ASSERT_TRUE(parser_get_error(&parser) == NULL, "no parse error");

    decl = program.top_level_decls[0];
    ASSERT_EQ_INT(AST_TOP_LEVEL_START, (int)decl->kind, "boot decl kind");
    ASSERT_TRUE(decl->as.start_decl.is_boot, "is_boot set for expression body");
    ASSERT_EQ_INT(AST_LAMBDA_BODY_EXPRESSION, (int)decl->as.start_decl.body.kind,
                  "boot body is expression");

    ast_program_free(&program);
    parser_free(&parser);
}

