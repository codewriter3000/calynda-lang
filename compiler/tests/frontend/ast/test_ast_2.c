#include "ast.h"

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
#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

AstExpression *make_identifier_expr(const char *name);
AstExpression *make_text_literal_expr(AstLiteralKind kind, const char *text);
AstExpression *make_binary_expr(AstBinaryOperator operator,
                                       AstExpression *left,
                                       AstExpression *right);
AstParameter make_parameter(AstPrimitiveType primitive, const char *name);


void test_program_structure(void) {
    AstProgram program;
    AstQualifiedName package_name;
    AstImportDecl import_decl;
    AstTopLevelDecl *binding_decl;
    AstTopLevelDecl *start_decl;
    AstParameter args_parameter;
    AstBlock *block;
    AstStatement *local_binding;
    AstStatement *return_statement;
    AstExpression *call_expr;

    ast_program_init(&program);

    ast_qualified_name_init(&package_name);
    REQUIRE_TRUE(ast_qualified_name_append(&package_name, "com"),
                 "append package segment com");
    REQUIRE_TRUE(ast_qualified_name_append(&package_name, "example"),
                 "append package segment example");
    REQUIRE_TRUE(ast_qualified_name_append(&package_name, "app"),
                 "append package segment app");
    REQUIRE_TRUE(ast_program_set_package(&program, &package_name),
                 "set package name");

    ast_import_decl_init(&import_decl);
    import_decl.kind = AST_IMPORT_PLAIN;
    REQUIRE_TRUE(ast_qualified_name_append(&import_decl.module_name, "io"),
                 "append import segment io");
    REQUIRE_TRUE(ast_qualified_name_append(&import_decl.module_name, "stdlib"),
                 "append import segment stdlib");
    REQUIRE_TRUE(ast_program_add_import(&program, &import_decl),
                 "add io.stdlib import");

    ast_import_decl_init(&import_decl);
    import_decl.kind = AST_IMPORT_PLAIN;
    REQUIRE_TRUE(ast_qualified_name_append(&import_decl.module_name, "math"),
                 "append import segment math");
    REQUIRE_TRUE(ast_qualified_name_append(&import_decl.module_name, "core"),
                 "append import segment core");
    REQUIRE_TRUE(ast_program_add_import(&program, &import_decl),
                 "add math.core import");

    binding_decl = ast_top_level_decl_new(AST_TOP_LEVEL_BINDING);
    REQUIRE_TRUE(binding_decl != NULL, "allocate top-level binding");
    REQUIRE_TRUE(ast_binding_decl_add_modifier(&binding_decl->as.binding_decl,
                                               AST_MODIFIER_PUBLIC),
                 "add public modifier");
    REQUIRE_TRUE(ast_binding_decl_add_modifier(&binding_decl->as.binding_decl,
                                               AST_MODIFIER_FINAL),
                 "add final modifier");
    ast_type_init_primitive(&binding_decl->as.binding_decl.declared_type,
                            AST_PRIMITIVE_INT32);
    binding_decl->as.binding_decl.name = ast_copy_text("answer");
    REQUIRE_TRUE(binding_decl->as.binding_decl.name != NULL,
                 "copy binding name");
    binding_decl->as.binding_decl.initializer =
        make_text_literal_expr(AST_LITERAL_INTEGER, "42");
    REQUIRE_TRUE(binding_decl->as.binding_decl.initializer != NULL,
                 "create binding initializer");
    REQUIRE_TRUE(ast_program_add_top_level_decl(&program, binding_decl),
                 "append binding declaration");

    start_decl = ast_top_level_decl_new(AST_TOP_LEVEL_START);
    REQUIRE_TRUE(start_decl != NULL, "allocate start declaration");

    args_parameter = make_parameter(AST_PRIMITIVE_STRING, "args");
    REQUIRE_TRUE(args_parameter.name != NULL, "copy start parameter name");
    REQUIRE_TRUE(ast_type_add_dimension(&args_parameter.type, false, NULL),
                 "add unsized array dimension to string parameter");
    REQUIRE_TRUE(ast_parameter_list_append(&start_decl->as.start_decl.parameters,
                                           &args_parameter),
                 "append start parameter");

    block = ast_block_new();
    REQUIRE_TRUE(block != NULL, "allocate start block");
    start_decl->as.start_decl.body.kind = AST_LAMBDA_BODY_BLOCK;
    start_decl->as.start_decl.body.as.block = block;

    local_binding = ast_statement_new(AST_STMT_LOCAL_BINDING);
    REQUIRE_TRUE(local_binding != NULL, "allocate local binding statement");
    local_binding->as.local_binding.is_inferred_type = true;
    local_binding->as.local_binding.name = ast_copy_text("result");
    REQUIRE_TRUE(local_binding->as.local_binding.name != NULL,
                 "copy local binding name");

    call_expr = ast_expression_new(AST_EXPR_CALL);
    REQUIRE_TRUE(call_expr != NULL, "allocate call expression");
    call_expr->as.call.callee = make_identifier_expr("add");
    REQUIRE_TRUE(call_expr->as.call.callee != NULL, "create call callee");
    REQUIRE_TRUE(ast_expression_list_append(&call_expr->as.call.arguments,
                                            make_text_literal_expr(AST_LITERAL_INTEGER, "1")),
                 "append first call argument");
    REQUIRE_TRUE(ast_expression_list_append(&call_expr->as.call.arguments,
                                            make_text_literal_expr(AST_LITERAL_INTEGER, "2")),
                 "append second call argument");
    local_binding->as.local_binding.initializer = call_expr;
    REQUIRE_TRUE(ast_block_append_statement(block, local_binding),
                 "append local binding statement");

    return_statement = ast_statement_new(AST_STMT_RETURN);
    REQUIRE_TRUE(return_statement != NULL, "allocate return statement");
    return_statement->as.return_expression =
        make_text_literal_expr(AST_LITERAL_INTEGER, "0");
    REQUIRE_TRUE(return_statement->as.return_expression != NULL,
                 "create return expression");
    REQUIRE_TRUE(ast_block_append_statement(block, return_statement),
                 "append return statement");
    REQUIRE_TRUE(ast_program_add_top_level_decl(&program, start_decl),
                 "append start declaration");

    ASSERT_TRUE(program.has_package, "program has package declaration");
    ASSERT_EQ_INT(3, program.package_name.count, "package segment count");
    ASSERT_EQ_STR("example", program.package_name.segments[1],
                  "package middle segment");
    ASSERT_EQ_INT(2, program.import_count, "import count");
    ASSERT_EQ_STR("stdlib", program.imports[0].module_name.segments[1], "first import tail");
    ASSERT_EQ_INT(2, program.top_level_count, "top-level declaration count");
    ASSERT_EQ_INT(AST_TOP_LEVEL_BINDING, program.top_level_decls[0]->kind,
                  "first top-level declaration kind");
    ASSERT_EQ_INT(2, program.top_level_decls[0]->as.binding_decl.modifier_count,
                  "binding modifier count");
    ASSERT_EQ_STR("answer", program.top_level_decls[0]->as.binding_decl.name,
                  "binding name");
    ASSERT_EQ_INT(AST_EXPR_LITERAL,
                  program.top_level_decls[0]->as.binding_decl.initializer->kind,
                  "binding initializer kind");
    ASSERT_EQ_INT(AST_TOP_LEVEL_START, program.top_level_decls[1]->kind,
                  "second top-level declaration kind");
    ASSERT_EQ_INT(1, program.top_level_decls[1]->as.start_decl.parameters.count,
                  "start parameter count");
    ASSERT_EQ_INT(AST_PRIMITIVE_STRING,
                  program.top_level_decls[1]->as.start_decl.parameters.items[0].type.primitive,
                  "start parameter primitive type");
    ASSERT_EQ_INT(1,
                  program.top_level_decls[1]->as.start_decl.parameters.items[0].type.dimension_count,
                  "start parameter dimension count");
    ASSERT_TRUE(!program.top_level_decls[1]->as.start_decl.parameters.items[0]
                     .type.dimensions[0].has_size,
                "start parameter array dimension is unsized");
    ASSERT_EQ_INT(2, block->statement_count, "start block statement count");
    ASSERT_EQ_INT(AST_STMT_LOCAL_BINDING, block->statements[0]->kind,
                  "first block statement kind");
    ASSERT_EQ_INT(AST_STMT_RETURN, block->statements[1]->kind,
                  "second block statement kind");

    ast_program_free(&program);
}

