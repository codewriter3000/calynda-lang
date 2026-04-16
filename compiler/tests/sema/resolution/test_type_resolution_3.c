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

void test_type_resolver_resolves_type_alias_and_threading_types(void) {
    const char *source =
        "type Grid = float64[][];\n"
        "start(string[] args) -> {\n"
        "    Grid g = [];\n"
        "    Thread t = spawn () -> { };\n"
        "    Mutex m = Mutex.new();\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    TypeResolver resolver;
    const ResolvedType *resolved;

    type_resolver_init(&resolver);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse alias + threading resolution");
    REQUIRE_TRUE(type_resolver_resolve_program(&resolver, &program),
                 "resolve alias + threading types");

    resolved = type_resolver_get_type(&resolver,
                                      &program.top_level_decls[1]
                                           ->as.start_decl.body.as.block->statements[0]
                                           ->as.local_binding.declared_type);
    REQUIRE_TRUE(resolved != NULL, "resolved alias usage exists");
    ASSERT_EQ_INT(RESOLVED_TYPE_VALUE, resolved->kind, "alias resolves transparently");
    ASSERT_EQ_INT(2, (int)resolved->array_depth, "alias preserves array depth");

    type_resolver_free(&resolver);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_type_resolver_rejects_circular_type_alias(void) {
    const char *source =
        "type A = A;\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    TypeResolver resolver;
    const TypeResolutionError *error;

    type_resolver_init(&resolver);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse circular type alias");
    ASSERT_TRUE(!type_resolver_resolve_program(&resolver, &program),
                "circular type alias is rejected");
    error = type_resolver_get_error(&resolver);
    REQUIRE_TRUE(error != NULL, "circular type alias error exists");
    ASSERT_TRUE(strstr(error->message, "circular") != NULL,
                "circular type alias error mentions circular");

    type_resolver_free(&resolver);
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/*  G-TRES-4: ptr<T> generic type resolves to named type "ptr"        */
/* ------------------------------------------------------------------ */
void test_type_resolver_resolves_ptr_generic_type(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    ptr<int32> p = malloc(8);\n"
        "    free(p);\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    TypeResolver resolver;
    const AstStartDecl *start_decl;
    const AstStatement *binding_stmt;
    const ResolvedType *resolved;

    type_resolver_init(&resolver);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse ptr<int32> local binding");
    REQUIRE_TRUE(type_resolver_resolve_program(&resolver, &program),
                 "ptr<int32> passes type resolution");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    binding_stmt = start_decl->body.as.block->statements[0];
    resolved = type_resolver_get_type(&resolver,
                                      &binding_stmt->as.local_binding.declared_type);
    REQUIRE_TRUE(resolved != NULL, "ptr<int32> resolved type exists");
    ASSERT_EQ_INT(RESOLVED_TYPE_NAMED, resolved->kind,
                  "ptr<int32> resolves as RESOLVED_TYPE_NAMED");
    ASSERT_TRUE(resolved->name != NULL && strcmp(resolved->name, "ptr") == 0,
                "ptr<int32> resolved name is ptr");
    ASSERT_EQ_INT(1, (int)resolved->generic_arg_count,
                  "ptr<int32> has exactly one generic argument");

    type_resolver_free(&resolver);
    ast_program_free(&program);
    parser_free(&parser);
}
