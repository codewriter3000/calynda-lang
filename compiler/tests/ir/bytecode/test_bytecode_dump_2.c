#include "bytecode.h"
#include "hir.h"
#include "mir.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

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

static bool build_bytecode_from_source(const char *source, char **dump_out) {
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    BytecodeProgram bytecode_program;
    bool ok = false;

    if (!source || !dump_out) {
        return false;
    }

    *dump_out = NULL;
    memset(&ast_program, 0, sizeof(ast_program));
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    bytecode_program_init(&bytecode_program);
    parser_init(&parser, source);

    if (!parser_parse_program(&parser, &ast_program) ||
        !symbol_table_build(&symbols, &ast_program) ||
        !type_checker_check_program(&checker, &ast_program, &symbols) ||
        !hir_build_program(&hir_program, &ast_program, &symbols, &checker) ||
        !mir_build_program(&mir_program, &hir_program, false) ||
        !bytecode_build_program(&bytecode_program, &mir_program)) {
        goto cleanup;
    }

    *dump_out = bytecode_dump_program_to_string(&bytecode_program);
    ok = *dump_out != NULL;

cleanup:
    bytecode_program_free(&bytecode_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
    return ok;
}

static void test_bytecode_dump_lowers_binary_unary_store_index_store_member(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "start(string[] args) -> {\n"
        "    var x = 3 + 4;\n"
        "    var y = -x;\n"
        "    var nums = [1, 2, 3];\n"
        "    nums[0] = 10;\n"
        "    stdlib.x = 5;\n"
        "    return y;\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump),
                 "build binary/unary/store bytecode dump");
    ASSERT_CONTAINS("BC_BINARY", dump,
                    "binary addition lowers to BC_BINARY");
    ASSERT_CONTAINS("BC_UNARY", dump,
                    "unary negation lowers to BC_UNARY");
    ASSERT_CONTAINS("BC_STORE_INDEX", dump,
                    "array index assignment lowers to BC_STORE_INDEX");
    ASSERT_CONTAINS("BC_STORE_MEMBER", dump,
                    "package member assignment lowers to BC_STORE_MEMBER");
    free(dump);
}


/* ------------------------------------------------------------------ */
/*  G-BC-2: Duplicate string constants are deduplicated in the pool   */
/* ------------------------------------------------------------------ */
static void test_bytecode_dump_deduplicates_string_constants(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    string a = \"hello\";\n"
        "    string b = \"hello\";\n"
        "    return 0;\n"
        "};\n";
    char *dump;
    const char *first;
    const char *second;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump),
                 "build constant pool dedup dump");
    first = strstr(dump, "literal kind=string text=\"hello\"");
    REQUIRE_TRUE(first != NULL,
                 "string constant appears at least once in constant pool");
    second = strstr(first + 1, "literal kind=string text=\"hello\"");
    ASSERT_TRUE(second == NULL,
                "duplicate string constant is deduplicated in bytecode constant pool");
    free(dump);
}

static void test_bytecode_dump_lowers_checked_manual_helpers(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual checked {\n"
        "        int64 p = malloc(64);\n"
        "        free(p);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump), "build checked manual bytecode dump");
    ASSERT_CONTAINS("__calynda_bc_malloc", dump, "manual checked malloc lowers to bounds-checked helper");
    ASSERT_CONTAINS("__calynda_bc_free", dump, "manual checked free lowers to bounds-checked helper");
    free(dump);
}

static void test_bytecode_dump_lowers_checked_ptr_helpers(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        ptr<int64, checked> p = malloc(16);\n"
        "        ptr<int64, checked> q = offset(p, 1);\n"
        "        store(q, 5);\n"
        "        int64 v = deref(q);\n"
        "        free(p);\n"
        "        return int32(v);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump), "build checked ptr bytecode dump");
    ASSERT_CONTAINS("__calynda_bc_offset", dump, "checked ptr offset lowers to bounds-checked helper");
    ASSERT_CONTAINS("__calynda_bc_store", dump, "checked ptr store lowers to bounds-checked helper");
    ASSERT_CONTAINS("__calynda_bc_deref", dump, "checked ptr deref lowers to bounds-checked helper");
    free(dump);
}

static void test_bytecode_dump_lowers_stackalloc_cleanup(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 p = stackalloc(8);\n"
        "        return 1;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    const char *cleanup_call;
    const char *return_stmt;
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump), "build stackalloc bytecode dump");
    cleanup_call = strstr(dump, "BC_CALL void <- global(");
    return_stmt = strstr(dump, "BC_RETURN const(");
    ASSERT_CONTAINS("__calynda_stackalloc", dump, "stackalloc lowers to runtime helper call");
    ASSERT_CONTAINS("symbol \"free\"", dump, "stackalloc registers free as deferred cleanup");
    ASSERT_TRUE(cleanup_call != NULL && return_stmt != NULL && cleanup_call < return_stmt,
                "stackalloc cleanup call stays ahead of the enclosing return");
    free(dump);
}

int main(void) {
    printf("Running bytecode dump tests (part 2)...\n\n");

    RUN_TEST(test_bytecode_dump_lowers_binary_unary_store_index_store_member);
    RUN_TEST(test_bytecode_dump_deduplicates_string_constants);
    RUN_TEST(test_bytecode_dump_lowers_checked_manual_helpers);
    RUN_TEST(test_bytecode_dump_lowers_checked_ptr_helpers);
    RUN_TEST(test_bytecode_dump_lowers_stackalloc_cleanup);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
