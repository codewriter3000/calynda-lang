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

