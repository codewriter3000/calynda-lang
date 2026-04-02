#include "../src/ast.h"

#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

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

static AstExpression *make_identifier_expr(const char *name) {
    AstExpression *expression = ast_expression_new(AST_EXPR_IDENTIFIER);

    if (!expression) {
        return NULL;
    }

    expression->as.identifier = ast_copy_text(name);
    if (!expression->as.identifier) {
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

static AstExpression *make_text_literal_expr(AstLiteralKind kind, const char *text) {
    AstExpression *expression = ast_expression_new(AST_EXPR_LITERAL);

    if (!expression) {
        return NULL;
    }

    expression->as.literal.kind = kind;
    expression->as.literal.as.text = ast_copy_text(text);
    if (!expression->as.literal.as.text) {
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

static AstExpression *make_binary_expr(AstBinaryOperator operator,
                                       AstExpression *left,
                                       AstExpression *right) {
    AstExpression *expression = ast_expression_new(AST_EXPR_BINARY);

    if (!expression) {
        ast_expression_free(left);
        ast_expression_free(right);
        return NULL;
    }

    expression->as.binary.operator = operator;
    expression->as.binary.left = left;
    expression->as.binary.right = right;
    return expression;
}

static AstParameter make_parameter(AstPrimitiveType primitive, const char *name) {
    AstParameter parameter;

    memset(&parameter, 0, sizeof(parameter));
    ast_type_init_primitive(&parameter.type, primitive);
    parameter.name = ast_copy_text(name);
    return parameter;
}

static void test_program_structure(void) {
    AstProgram program;
    AstQualifiedName package_name;
    AstQualifiedName import_name;
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

    ast_qualified_name_init(&import_name);
    REQUIRE_TRUE(ast_qualified_name_append(&import_name, "io"),
                 "append import segment io");
    REQUIRE_TRUE(ast_qualified_name_append(&import_name, "stdlib"),
                 "append import segment stdlib");
    REQUIRE_TRUE(ast_program_add_import(&program, &import_name),
                 "add io.stdlib import");

    ast_qualified_name_init(&import_name);
    REQUIRE_TRUE(ast_qualified_name_append(&import_name, "math"),
                 "append import segment math");
    REQUIRE_TRUE(ast_qualified_name_append(&import_name, "core"),
                 "append import segment core");
    REQUIRE_TRUE(ast_program_add_import(&program, &import_name),
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
    ASSERT_EQ_STR("stdlib", program.imports[0].segments[1], "first import tail");
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

static void test_expression_forms(void) {
    AstExpression *lambda_expr;
    AstParameter parameter;
    AstExpression *assignment_expr;
    AstExpression *then_branch;
    AstExpression *member_expr;
    AstExpression *call_expr;

    lambda_expr = ast_expression_new(AST_EXPR_LAMBDA);
    REQUIRE_TRUE(lambda_expr != NULL, "allocate lambda expression");
    parameter = make_parameter(AST_PRIMITIVE_INT32, "value");
    REQUIRE_TRUE(parameter.name != NULL, "copy lambda parameter name");
    REQUIRE_TRUE(ast_parameter_list_append(&lambda_expr->as.lambda.parameters, &parameter),
                 "append lambda parameter");
    lambda_expr->as.lambda.body.kind = AST_LAMBDA_BODY_EXPRESSION;
    lambda_expr->as.lambda.body.as.expression =
        make_binary_expr(AST_BINARY_OP_ADD,
                         make_identifier_expr("value"),
                         make_text_literal_expr(AST_LITERAL_INTEGER, "1"));
    REQUIRE_TRUE(lambda_expr->as.lambda.body.as.expression != NULL,
                 "build lambda body expression");

    ASSERT_EQ_INT(AST_EXPR_LAMBDA, lambda_expr->kind, "lambda expression kind");
    ASSERT_EQ_INT(1, lambda_expr->as.lambda.parameters.count,
                  "lambda parameter count");
    ASSERT_EQ_INT(AST_EXPR_BINARY,
                  lambda_expr->as.lambda.body.as.expression->kind,
                  "lambda body expression kind");
    ASSERT_EQ_INT(AST_BINARY_OP_ADD,
                  lambda_expr->as.lambda.body.as.expression->as.binary.operator,
                  "lambda binary operator");

    assignment_expr = ast_expression_new(AST_EXPR_ASSIGNMENT);
    REQUIRE_TRUE(assignment_expr != NULL, "allocate assignment expression");
    assignment_expr->as.assignment.operator = AST_ASSIGN_OP_ADD;
    assignment_expr->as.assignment.target = make_identifier_expr("result");
    REQUIRE_TRUE(assignment_expr->as.assignment.target != NULL,
                 "create assignment target");
    assignment_expr->as.assignment.value = ast_expression_new(AST_EXPR_TERNARY);
    REQUIRE_TRUE(assignment_expr->as.assignment.value != NULL,
                 "allocate ternary expression");
    assignment_expr->as.assignment.value->as.ternary.condition =
        make_identifier_expr("ready");
    REQUIRE_TRUE(assignment_expr->as.assignment.value->as.ternary.condition != NULL,
                 "create ternary condition");

    then_branch = ast_expression_new(AST_EXPR_CAST);
    REQUIRE_TRUE(then_branch != NULL, "allocate cast expression");
    then_branch->as.cast.target_type = AST_PRIMITIVE_INT32;

    member_expr = ast_expression_new(AST_EXPR_MEMBER);
    REQUIRE_TRUE(member_expr != NULL, "allocate member expression");
    member_expr->as.member.target = ast_expression_new(AST_EXPR_INDEX);
    REQUIRE_TRUE(member_expr->as.member.target != NULL,
                 "allocate index expression");
    member_expr->as.member.target->as.index.target = make_identifier_expr("items");
    REQUIRE_TRUE(member_expr->as.member.target->as.index.target != NULL,
                 "create index target");
    member_expr->as.member.target->as.index.index =
        make_text_literal_expr(AST_LITERAL_INTEGER, "0");
    REQUIRE_TRUE(member_expr->as.member.target->as.index.index != NULL,
                 "create index operand");
    member_expr->as.member.member = ast_copy_text("count");
    REQUIRE_TRUE(member_expr->as.member.member != NULL, "copy member name");
    then_branch->as.cast.expression = member_expr;
    assignment_expr->as.assignment.value->as.ternary.then_branch = then_branch;

    assignment_expr->as.assignment.value->as.ternary.else_branch =
        ast_expression_new(AST_EXPR_ARRAY_LITERAL);
    REQUIRE_TRUE(assignment_expr->as.assignment.value->as.ternary.else_branch != NULL,
                 "allocate array literal expression");
    REQUIRE_TRUE(ast_expression_list_append(
                     &assignment_expr->as.assignment.value->as.ternary.else_branch
                          ->as.array_literal.elements,
                     make_text_literal_expr(AST_LITERAL_INTEGER, "1")),
                 "append first array element");
    REQUIRE_TRUE(ast_expression_list_append(
                     &assignment_expr->as.assignment.value->as.ternary.else_branch
                          ->as.array_literal.elements,
                     make_text_literal_expr(AST_LITERAL_INTEGER, "2")),
                 "append second array element");

    ASSERT_EQ_INT(AST_EXPR_ASSIGNMENT, assignment_expr->kind,
                  "assignment expression kind");
    ASSERT_EQ_INT(AST_ASSIGN_OP_ADD, assignment_expr->as.assignment.operator,
                  "assignment operator");
    ASSERT_EQ_INT(AST_EXPR_TERNARY, assignment_expr->as.assignment.value->kind,
                  "assignment value kind");
    ASSERT_EQ_INT(AST_EXPR_CAST,
                  assignment_expr->as.assignment.value->as.ternary.then_branch->kind,
                  "ternary then-branch kind");
    ASSERT_EQ_INT(AST_EXPR_MEMBER,
                  assignment_expr->as.assignment.value->as.ternary.then_branch
                      ->as.cast.expression->kind,
                  "cast operand kind");
    ASSERT_EQ_INT(AST_EXPR_INDEX,
                  assignment_expr->as.assignment.value->as.ternary.then_branch
                      ->as.cast.expression->as.member.target->kind,
                  "member target kind");
    ASSERT_EQ_INT(AST_EXPR_ARRAY_LITERAL,
                  assignment_expr->as.assignment.value->as.ternary.else_branch->kind,
                  "ternary else-branch kind");
    ASSERT_EQ_INT(2,
                  assignment_expr->as.assignment.value->as.ternary.else_branch
                      ->as.array_literal.elements.count,
                  "array literal element count");

    call_expr = ast_expression_new(AST_EXPR_CALL);
    REQUIRE_TRUE(call_expr != NULL, "allocate call expression");
    call_expr->as.call.callee = make_identifier_expr("emit");
    REQUIRE_TRUE(call_expr->as.call.callee != NULL, "create emit callee");
    call_expr->as.call.arguments.items = NULL;
    call_expr->as.call.arguments.count = 0;
    call_expr->as.call.arguments.capacity = 0;
    call_expr->as.call.arguments.count = 0;
    call_expr->as.call.arguments.capacity = 0;
    call_expr->as.call.arguments.items = NULL;
    call_expr->as.call.arguments.count = 0;

    {
        AstExpression *grouping_expr = ast_expression_new(AST_EXPR_GROUPING);
        AstExpression *unary_expr = ast_expression_new(AST_EXPR_UNARY);

        REQUIRE_TRUE(grouping_expr != NULL, "allocate grouping expression");
        REQUIRE_TRUE(unary_expr != NULL, "allocate unary expression");
        unary_expr->as.unary.operator = AST_UNARY_OP_NEGATE;
        unary_expr->as.unary.operand = make_identifier_expr("delta");
        REQUIRE_TRUE(unary_expr->as.unary.operand != NULL,
                     "create unary operand");
        grouping_expr->as.grouping.inner = unary_expr;
        REQUIRE_TRUE(ast_expression_list_append(&call_expr->as.call.arguments,
                                                grouping_expr),
                     "append grouped unary argument");
    }

    ASSERT_EQ_INT(AST_EXPR_CALL, call_expr->kind, "call expression kind");
    ASSERT_EQ_INT(1, call_expr->as.call.arguments.count, "call argument count");
    ASSERT_EQ_INT(AST_EXPR_GROUPING, call_expr->as.call.arguments.items[0]->kind,
                  "call argument grouping kind");
    ASSERT_EQ_INT(AST_EXPR_UNARY,
                  call_expr->as.call.arguments.items[0]->as.grouping.inner->kind,
                  "grouping inner kind");
    ASSERT_EQ_INT(AST_UNARY_OP_NEGATE,
                  call_expr->as.call.arguments.items[0]->as.grouping.inner->as.unary.operator,
                  "unary operator kind");

    ast_expression_free(lambda_expr);
    ast_expression_free(assignment_expr);
    ast_expression_free(call_expr);
}

static void test_template_literal(void) {
    AstExpression *template_expr = ast_expression_new(AST_EXPR_LITERAL);

    REQUIRE_TRUE(template_expr != NULL, "allocate template literal expression");
    template_expr->as.literal.kind = AST_LITERAL_TEMPLATE;

    REQUIRE_TRUE(ast_template_literal_append_text(&template_expr->as.literal,
                                                  "Hello "),
                 "append first template text part");
    REQUIRE_TRUE(ast_template_literal_append_expression(&template_expr->as.literal,
                                                        make_identifier_expr("name")),
                 "append template identifier expression");
    REQUIRE_TRUE(ast_template_literal_append_text(&template_expr->as.literal,
                                                  ", count="),
                 "append second template text part");
    REQUIRE_TRUE(ast_template_literal_append_expression(&template_expr->as.literal,
                                                        make_text_literal_expr(AST_LITERAL_INTEGER,
                                                                               "3")),
                 "append template integer expression");

    ASSERT_EQ_INT(AST_LITERAL_TEMPLATE, template_expr->as.literal.kind,
                  "template literal kind");
    ASSERT_EQ_INT(4, template_expr->as.literal.as.template_parts.count,
                  "template part count");
    ASSERT_EQ_INT(AST_TEMPLATE_PART_TEXT,
                  template_expr->as.literal.as.template_parts.items[0].kind,
                  "first template part kind");
    ASSERT_EQ_STR("Hello ",
                  template_expr->as.literal.as.template_parts.items[0].as.text,
                  "first template part text");
    ASSERT_EQ_INT(AST_TEMPLATE_PART_EXPRESSION,
                  template_expr->as.literal.as.template_parts.items[1].kind,
                  "second template part kind");
    ASSERT_EQ_INT(AST_EXPR_IDENTIFIER,
                  template_expr->as.literal.as.template_parts.items[1].as.expression->kind,
                  "template identifier expression kind");
    ASSERT_EQ_INT(AST_TEMPLATE_PART_TEXT,
                  template_expr->as.literal.as.template_parts.items[2].kind,
                  "third template part kind");
    ASSERT_EQ_STR(", count=",
                  template_expr->as.literal.as.template_parts.items[2].as.text,
                  "second template part text");
    ASSERT_EQ_INT(AST_EXPR_LITERAL,
                  template_expr->as.literal.as.template_parts.items[3].as.expression->kind,
                  "template literal expression kind");

    ast_expression_free(template_expr);
}

int main(void) {
    printf("Running AST tests...\n\n");

    RUN_TEST(test_program_structure);
    RUN_TEST(test_expression_forms);
    RUN_TEST(test_template_literal);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}