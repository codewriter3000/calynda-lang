#include "parser.h"
#include "type_resolution.h"

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


void test_type_resolver_resolves_arr_wildcard(void) {
    const char *source =
        "arr<?> record = [1, \"hello\"];\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    TypeResolver resolver;
    const AstBindingDecl *binding;
    const ResolvedType *resolved;

    type_resolver_init(&resolver);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse arr wildcard program");
    ASSERT_TRUE(type_resolver_resolve_program(&resolver, &program),
                "arr<?> passes type resolution");

    binding = &program.top_level_decls[0]->as.binding_decl;
    resolved = type_resolver_get_type(&resolver, &binding->declared_type);
    REQUIRE_TRUE(resolved != NULL, "resolved arr type exists");
    ASSERT_EQ_INT(RESOLVED_TYPE_NAMED, resolved->kind, "resolved type is named");
    ASSERT_EQ_STR("arr", resolved->name, "resolved name is arr");
    ASSERT_EQ_INT(1, (int)resolved->generic_arg_count, "one generic arg");

    type_resolver_free(&resolver);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_resolver_resolves_union_variant_payload(void) {
    const char *source =
        "union Result<T, E> { Ok(T), Err(E) };\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    TypeResolver resolver;

    type_resolver_init(&resolver);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse variant payload program");
    ASSERT_TRUE(type_resolver_resolve_program(&resolver, &program),
                "union variant payloads pass type resolution");

    type_resolver_free(&resolver);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_type_resolver_rejects_void_array_element(void) {
    const char *source =
        "int32[0] bad = [];\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    TypeResolver resolver;
    const TypeResolutionError *error;
    char buffer[256];

    type_resolver_init(&resolver);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse zero-dim array program");
    ASSERT_TRUE(!type_resolver_resolve_program(&resolver, &program),
                "int32[0] is rejected by type resolution");
    ASSERT_TRUE(resolver.has_error, "resolver records an error for zero-dim array");
    error = type_resolver_get_error(&resolver);
    REQUIRE_TRUE(error != NULL, "resolver error is retrievable");
    ASSERT_TRUE(error->primary_span.start_line > 0,
                "zero-dim error has a valid source line");
    ASSERT_TRUE(type_resolver_format_error(error, buffer, sizeof(buffer)),
                "resolver error formats without crash");

    type_resolver_free(&resolver);
    ast_program_free(&program);
    parser_free(&parser);
}

