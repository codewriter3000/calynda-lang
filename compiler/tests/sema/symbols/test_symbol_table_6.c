#include "car.h"
#include "parser.h"
#include "symbol_table.h"

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

static bool add_archive_source(CarArchive *archive,
                               const char *path,
                               const char *source) {
    return archive &&
           path &&
           source &&
           car_archive_add_file(archive, path, source, strlen(source));
}


/* ------------------------------------------------------------------ */
/* V2: internal flag propagated to local symbols                      */
/* ------------------------------------------------------------------ */
void test_symbol_table_internal_flag(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    internal void helper = () -> { return; };\n"
        "    int32 plain = 0;\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const AstStartDecl *start_decl;
    const Scope *start_scope;
    const Scope *block_scope;
    const Symbol *sym;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse internal local");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols internal");

    start_decl = &program.top_level_decls[0]->as.start_decl;
    start_scope = symbol_table_find_scope(&table, start_decl, SCOPE_KIND_START);
    REQUIRE_TRUE(start_scope != NULL, "start scope exists");
    REQUIRE_TRUE(start_scope->child_count > 0, "start scope has children");
    block_scope = start_scope->children[0];

    sym = scope_lookup_local(block_scope, "helper");
    REQUIRE_TRUE(sym != NULL, "helper symbol found");
    ASSERT_TRUE(sym->is_internal, "helper is_internal");

    sym = scope_lookup_local(block_scope, "plain");
    REQUIRE_TRUE(sym != NULL, "plain symbol found");
    ASSERT_TRUE(!sym->is_internal, "plain not is_internal");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_symbol_table_union_and_type_params(void) {
    const char *source =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const Symbol *sym;
    const Scope *root;
    const AstUnionDecl *union_decl;
    const Scope *union_scope;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse union program");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols union");

    root = symbol_table_root_scope(&table);
    REQUIRE_TRUE(root != NULL, "root scope exists");

    sym = scope_lookup_local(root, "Option");
    REQUIRE_TRUE(sym != NULL, "Option symbol found");
    ASSERT_EQ_INT(SYMBOL_KIND_UNION, sym->kind, "Option is union");
    ASSERT_EQ_INT(1, (int)sym->generic_param_count, "Option has 1 generic param");

    union_decl = &program.top_level_decls[0]->as.union_decl;
    union_scope = symbol_table_find_scope(&table, union_decl, SCOPE_KIND_UNION);
    REQUIRE_TRUE(union_scope != NULL, "union scope exists");
    ASSERT_EQ_INT(3, (int)union_scope->symbol_count, "union scope has 3 symbols");

    /* Type parameter T */
    sym = scope_lookup_local(union_scope, "T");
    REQUIRE_TRUE(sym != NULL, "type param T found");
    ASSERT_EQ_INT(SYMBOL_KIND_TYPE_PARAMETER, sym->kind, "T is type param");

    /* Variant Some(T) */
    sym = scope_lookup_local(union_scope, "Some");
    REQUIRE_TRUE(sym != NULL, "variant Some found");
    ASSERT_EQ_INT(SYMBOL_KIND_VARIANT, sym->kind, "Some is variant");
    ASSERT_EQ_INT(0, (int)sym->variant_index, "Some is variant 0");
    ASSERT_TRUE(sym->variant_has_payload, "Some has payload");

    /* Variant None */
    sym = scope_lookup_local(union_scope, "None");
    REQUIRE_TRUE(sym != NULL, "variant None found");
    ASSERT_EQ_INT(SYMBOL_KIND_VARIANT, sym->kind, "None is variant");
    ASSERT_EQ_INT(1, (int)sym->variant_index, "None is variant 1");
    ASSERT_TRUE(!sym->variant_has_payload, "None has no payload");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_symbol_table_union_no_generics(void) {
    const char *source =
        "union Direction { North, South, East, West };\n"
        "start(string[] args) -> 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const Symbol *sym;
    const Scope *root;
    const AstUnionDecl *union_decl;
    const Scope *union_scope;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse simple union");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols simple union");

    root = symbol_table_root_scope(&table);
    sym = scope_lookup_local(root, "Direction");
    REQUIRE_TRUE(sym != NULL, "Direction symbol found");
    ASSERT_EQ_INT(SYMBOL_KIND_UNION, sym->kind, "Direction is union");
    ASSERT_EQ_INT(0, (int)sym->generic_param_count, "Direction has 0 generic params");

    union_decl = &program.top_level_decls[0]->as.union_decl;
    union_scope = symbol_table_find_scope(&table, union_decl, SCOPE_KIND_UNION);
    REQUIRE_TRUE(union_scope != NULL, "Direction union scope exists");
    ASSERT_EQ_INT(4, (int)union_scope->symbol_count, "Direction scope has 4 variants");

    sym = scope_lookup_local(union_scope, "North");
    REQUIRE_TRUE(sym != NULL, "North variant found");
    ASSERT_EQ_INT(SYMBOL_KIND_VARIANT, sym->kind, "North is variant");
    ASSERT_EQ_INT(0, (int)sym->variant_index, "North is variant 0");
    ASSERT_TRUE(!sym->variant_has_payload, "North has no payload");

    sym = scope_lookup_local(union_scope, "West");
    REQUIRE_TRUE(sym != NULL, "West variant found");
    ASSERT_EQ_INT(SYMBOL_KIND_VARIANT, sym->kind, "West is variant");
    ASSERT_EQ_INT(3, (int)sym->variant_index, "West is variant 3");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_symbol_table_injects_wildcard_dep_archive_imports(void) {
    static const char lib_source[] =
        "package lib.math;\n"
        "export int32 squarePower = (int32 value) -> value * value;\n";
    static const char main_source[] =
        "import lib.math.*;\n"
        "start(string[] args) -> {\n"
        "    return squarePower(5);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    CarArchive archive;
    const Symbol *symbol;

    car_archive_init(&archive);
    parser_init(&parser, main_source);
    symbol_table_init(&table);
    REQUIRE_TRUE(add_archive_source(&archive, "lib/math.cal", lib_source),
                 "build wildcard dependency archive");
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse wildcard dependency archive consumer");
    REQUIRE_TRUE(symbol_table_build_with_archive_deps(&table, &program,
                                                      &archive, 1),
                 "build symbols with wildcard dependency archive");

    symbol = symbol_table_find_import(&table, "squarePower");
    REQUIRE_TRUE(symbol != NULL, "wildcard import injects squarePower");
    ASSERT_EQ_INT(SYMBOL_KIND_ASM_BINDING, symbol->kind,
                  "wildcard import preserves callable symbol kind");
    ASSERT_EQ_STR("lib.math.squarePower", symbol->qualified_name,
                  "wildcard import preserves qualified name");

    car_archive_free(&archive);
    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_symbol_table_stores_top_level_overload_sets(void) {
    static const char source[] =
        "int32 pick = (int32 value) -> value;\n"
        "int32 pick = (string value) -> 0;\n"
        "start(string[] args) -> {\n"
        "    return pick(1);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const Scope *root;
    const OverloadSet *overload_set;
    const AstExpression *call_expr;
    const SymbolResolution *resolution;

    parser_init(&parser, source);
    symbol_table_init(&table);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse overload-set source");
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build overload-set symbols");

    root = symbol_table_root_scope(&table);
    REQUIRE_TRUE(root != NULL, "root scope exists for overload-set source");
    overload_set = scope_lookup_local_overload_set(root, "pick");
    REQUIRE_TRUE(overload_set != NULL, "top-level overload set recorded");
    ASSERT_EQ_INT(2, (int)overload_set->symbol_count, "overload set stores both callables");

    call_expr = program.top_level_decls[2]
                    ->as.start_decl.body.as.block->statements[0]
                    ->as.return_expression;
    REQUIRE_TRUE(call_expr != NULL && call_expr->kind == AST_EXPR_CALL,
                 "return expression is call");
    resolution = symbol_table_find_resolution(&table, call_expr->as.call.callee);
    REQUIRE_TRUE(resolution != NULL, "call callee resolution exists");
    ASSERT_TRUE(resolution->symbol == NULL,
                "callee keeps unresolved symbol until overload selection");
    ASSERT_TRUE(resolution->overload_set == overload_set,
                "callee resolution points at overload set");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}
