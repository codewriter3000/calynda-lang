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


/* ------------------------------------------------------------------ */
/* V2: import alias binds the alias name as the import symbol         */
/* ------------------------------------------------------------------ */
void test_symbol_table_import_alias(void) {
    const char *source =
        "import io.stdlib as std;\n"
        "int32 x = 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse import alias");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbol table with alias");
    ASSERT_TRUE(symbol_table_get_error(&table) == NULL, "no symbol table error");

    {
        const Symbol *import_sym = symbol_table_find_import(&table, "std");
        REQUIRE_TRUE(import_sym != NULL, "import alias 'std' found");
        ASSERT_EQ_STR("std", import_sym->name, "import alias local name");
        ASSERT_EQ_STR("io.stdlib", import_sym->qualified_name, "import alias qualified name");
    }

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2: selective import binds each selected name as an import symbol   */
/* ------------------------------------------------------------------ */
void test_symbol_table_import_selective(void) {
    const char *source =
        "import io.stdlib.{print, readLine};\n"
        "int32 x = 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse selective import");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbol table with selective");
    ASSERT_TRUE(symbol_table_get_error(&table) == NULL, "no symbol table error");

    {
        const Symbol *print_sym = symbol_table_find_import(&table, "print");
        const Symbol *readline_sym = symbol_table_find_import(&table, "readLine");
        REQUIRE_TRUE(print_sym != NULL, "selective import 'print' found");
        ASSERT_EQ_STR("io.stdlib.print", print_sym->qualified_name,
                       "selective import print qualified");
        REQUIRE_TRUE(readline_sym != NULL, "selective import 'readLine' found");
        ASSERT_EQ_STR("io.stdlib.readLine", readline_sym->qualified_name,
                       "selective import readLine qualified");
    }

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2: ambiguous selective import is diagnosed                        */
/* ------------------------------------------------------------------ */
void test_symbol_table_ambiguous_selective_import(void) {
    const char *source =
        "import io.stdlib.{print};\n"
        "import io.other.{print};\n"
        "int32 x = 0;\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse ambiguous imports");

    symbol_table_init(&table);
    ASSERT_TRUE(!symbol_table_build(&table, &program),
                "ambiguous import build fails");
    {
        const SymbolTableError *error = symbol_table_get_error(&table);
        REQUIRE_TRUE(error != NULL, "ambiguous import error exists");
        ASSERT_CONTAINS("print", error->message, "error mentions 'print'");
    }

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}


/* ------------------------------------------------------------------ */
/* V2: export and static flags propagated to symbols                  */
/* ------------------------------------------------------------------ */
void test_symbol_table_export_static_flags(void) {
    const char *source =
        "export int32 visible = 1;\n"
        "static int32 counter = 0;\n"
        "int32 plain = 2;\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const Scope *root;
    const Symbol *sym;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse export/static flags");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols export/static");

    root = symbol_table_root_scope(&table);
    REQUIRE_TRUE(root != NULL, "root scope exists");

    sym = scope_lookup_local(root, "visible");
    REQUIRE_TRUE(sym != NULL, "visible symbol found");
    ASSERT_TRUE(sym->is_exported, "visible is_exported");
    ASSERT_TRUE(!sym->is_static, "visible not is_static");

    sym = scope_lookup_local(root, "counter");
    REQUIRE_TRUE(sym != NULL, "counter symbol found");
    ASSERT_TRUE(!sym->is_exported, "counter not is_exported");
    ASSERT_TRUE(sym->is_static, "counter is_static");

    sym = scope_lookup_local(root, "plain");
    REQUIRE_TRUE(sym != NULL, "plain symbol found");
    ASSERT_TRUE(!sym->is_exported, "plain not is_exported");
    ASSERT_TRUE(!sym->is_static, "plain not is_static");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_symbol_table_thread_local_flag(void) {
    const char *source =
        "thread_local int32 tlsCounter = 0;\n"
        "int32 plain = 2;\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;
    const Scope *root;
    const Symbol *sym;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse thread_local flag");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols thread_local");

    root = symbol_table_root_scope(&table);
    REQUIRE_TRUE(root != NULL, "root scope exists");

    sym = scope_lookup_local(root, "tlsCounter");
    REQUIRE_TRUE(sym != NULL, "tlsCounter symbol found");
    ASSERT_TRUE(sym->is_thread_local, "tlsCounter is_thread_local");
    ASSERT_TRUE(!sym->is_static, "tlsCounter not is_static");

    sym = scope_lookup_local(root, "plain");
    REQUIRE_TRUE(sym != NULL, "plain symbol found");
    ASSERT_TRUE(!sym->is_thread_local, "plain not is_thread_local");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_symbol_table_allows_future_atomic_builtin_identifiers(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    Future<int32> futureValue = Future.new();\n"
        "    Atomic<int32> atomicValue = Atomic.new();\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse Future/Atomic builtin identifiers");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols Future/Atomic builtin identifiers");
    ASSERT_EQ_INT(0, (int)table.unresolved_count,
                  "Future/Atomic builtin identifiers do not become unresolved");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_symbol_table_allows_car_cdr_builtin_identifiers(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    int32 head = car(values);\n"
        "    int32[] tail = cdr(values);\n"
        "    return head + int32(tail.length);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse car/cdr builtin identifiers");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols car/cdr builtin identifiers");
    ASSERT_EQ_INT(0, (int)table.unresolved_count,
                  "car/cdr builtin identifiers do not become unresolved");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_symbol_table_allows_type_query_builtin_identifiers(void) {
    const char *source =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    arr<?> mixed = [1, true];\n"
        "    string typeName = typeof(values);\n"
        "    bool ok = isint(1) && isfloat(float64(1)) && isbool(true) &&\n"
        "              isstring(typeName) && isarray(values) && isarray(mixed) &&\n"
        "              issametype(values, cdr(values));\n"
        "    return ok ? 0 : 1;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable table;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse typeof/is* builtin identifiers");

    symbol_table_init(&table);
    REQUIRE_TRUE(symbol_table_build(&table, &program), "build symbols typeof/is* builtin identifiers");
    ASSERT_EQ_INT(0, (int)table.unresolved_count,
                  "typeof/is* builtin identifiers do not become unresolved");

    symbol_table_free(&table);
    ast_program_free(&program);
    parser_free(&parser);
}
