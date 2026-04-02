#include "../src/parser.h"
#include "../src/type_resolution.h"

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

static void test_type_resolver_resolves_declared_types_and_casts(void) {
    const char *source =
        "int32 add = (int32 left) -> left;\n"
        "start(string[] args) -> {\n"
        "    int32[4] values = [1];\n"
        "    return int32(values[0]);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    TypeResolver resolver;
    const ResolvedType *binding_type;
    const ResolvedType *lambda_param_type;
    const ResolvedType *start_param_type;
    const ResolvedType *local_type;
    const ResolvedType *cast_target_type;
    const AstExpression *lambda_expression;
    const AstBlock *start_block;
    const AstExpression *cast_expression;
    char buffer[64];
    char *buffer_text = buffer;

    type_resolver_init(&resolver);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse program for type resolution test");
    REQUIRE_TRUE(type_resolver_resolve_program(&resolver, &program),
                 "resolve declared types in program");

    lambda_expression = program.top_level_decls[0]->as.binding_decl.initializer;
    start_block = program.top_level_decls[1]->as.start_decl.body.as.block;
    cast_expression = start_block->statements[1]->as.return_expression;

    binding_type = type_resolver_get_type(&resolver,
                                          &program.top_level_decls[0]->as.binding_decl.declared_type);
    lambda_param_type = type_resolver_get_type(&resolver,
                                               &lambda_expression->as.lambda.parameters.items[0].type);
    start_param_type = type_resolver_get_type(&resolver,
                                              &program.top_level_decls[1]->as.start_decl.parameters.items[0].type);
    local_type = type_resolver_get_type(&resolver,
                                        &start_block->statements[0]->as.local_binding.declared_type);
    cast_target_type = type_resolver_get_cast_target_type(&resolver, cast_expression);

    REQUIRE_TRUE(binding_type != NULL, "binding type is resolved");
    REQUIRE_TRUE(lambda_param_type != NULL, "lambda parameter type is resolved");
    REQUIRE_TRUE(start_param_type != NULL, "start parameter type is resolved");
    REQUIRE_TRUE(local_type != NULL, "local array type is resolved");
    REQUIRE_TRUE(cast_target_type != NULL, "cast target type is resolved");

    ASSERT_EQ_INT(RESOLVED_TYPE_VALUE, binding_type->kind, "binding type kind");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, binding_type->primitive, "binding primitive");
    ASSERT_EQ_INT(0, binding_type->array_depth, "binding array depth");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, lambda_param_type->primitive,
                  "lambda parameter primitive");
    ASSERT_EQ_INT(AST_PRIMITIVE_STRING, start_param_type->primitive,
                  "start parameter primitive");
    ASSERT_EQ_INT(1, start_param_type->array_depth, "start parameter array depth");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, local_type->primitive, "local primitive");
    ASSERT_EQ_INT(1, local_type->array_depth, "local array depth");
    ASSERT_EQ_INT(AST_PRIMITIVE_INT32, cast_target_type->primitive,
                  "cast target primitive");
    ASSERT_TRUE(resolved_type_to_string(*local_type, buffer, sizeof(buffer)),
                "format resolved local type");
    ASSERT_EQ_STR("int32[]", buffer_text, "formatted resolved local type");

    type_resolver_free(&resolver);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_resolver_rejects_void_parameter(void) {
    const char *source = "start(void args) -> 0;\n";
    Parser parser;
    AstProgram program;
    TypeResolver resolver;
    const TypeResolutionError *error;
    char diagnostic[256];
    char *diagnostic_text = diagnostic;

    type_resolver_init(&resolver);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse void parameter program");
    ASSERT_TRUE(!type_resolver_resolve_program(&resolver, &program),
                "void parameter fails type resolution");

    error = type_resolver_get_error(&resolver);
    REQUIRE_TRUE(error != NULL, "void parameter error exists");
    REQUIRE_TRUE(type_resolver_format_error(error, diagnostic, sizeof(diagnostic)),
                 "format void parameter error");
    ASSERT_EQ_STR("1:12: Parameter 'args' cannot have type void.",
                  diagnostic_text,
                  "formatted void parameter diagnostic");

    type_resolver_free(&resolver);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_type_resolver_rejects_zero_sized_array(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    int32[0] values = [1];\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    TypeResolver resolver;
    const TypeResolutionError *error;
    char diagnostic[256];
    char *diagnostic_text = diagnostic;

    type_resolver_init(&resolver);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse zero-sized array program");
    ASSERT_TRUE(!type_resolver_resolve_program(&resolver, &program),
                "zero-sized array fails type resolution");

    error = type_resolver_get_error(&resolver);
    REQUIRE_TRUE(error != NULL, "zero-sized array error exists");
    REQUIRE_TRUE(type_resolver_format_error(error, diagnostic, sizeof(diagnostic)),
                 "format zero-sized array error");
    ASSERT_EQ_STR("2:14: Local 'values' has invalid array dimension '0'; sizes must be positive integers.",
                  diagnostic_text,
                  "formatted zero-sized array diagnostic");

    type_resolver_free(&resolver);
    ast_program_free(&program);
    parser_free(&parser);
}

int main(void) {
    printf("Running type resolution tests...\n\n");

    RUN_TEST(test_type_resolver_resolves_declared_types_and_casts);
    RUN_TEST(test_type_resolver_rejects_void_parameter);
    RUN_TEST(test_type_resolver_rejects_zero_sized_array);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}