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

static void test_bytecode_dump_lowers_init_start_and_lambda_units(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "int32 seed = 1;\n"
        "start(string[] args) -> {\n"
        "    var values = [seed, 2, 3];\n"
        "    var render = (int32 x) -> `value ${x}`;\n"
        "    stdlib.print(render(values[1]));\n"
        "    return values[0];\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump), "build bytecode dump text");
    ASSERT_CONTAINS("BytecodeProgram target=portable-v1", dump, "bytecode backend reports portable target name");
    ASSERT_CONTAINS("Unit name=__mir$module_init kind=init", dump, "top-level initializers lower into a bytecode init unit");
    ASSERT_CONTAINS("Unit name=start kind=start", dump, "start lowers into a bytecode start unit");
    ASSERT_CONTAINS("kind=lambda", dump, "lambda bodies lower into sibling bytecode units");
    ASSERT_CONTAINS("BC_STORE_GLOBAL", dump, "module initialization keeps explicit global stores");
    ASSERT_CONTAINS("BC_ARRAY_LITERAL", dump, "array literals lower into bytecode opcodes");
    ASSERT_CONTAINS("BC_CLOSURE", dump, "closure construction lowers into bytecode opcodes");
    ASSERT_CONTAINS("BC_MEMBER", dump, "member loads lower into bytecode opcodes");
    ASSERT_CONTAINS("BC_TEMPLATE", dump, "templates lower into bytecode opcodes");
    ASSERT_CONTAINS("BC_CALL", dump, "calls lower into bytecode opcodes");
    free(dump);
}

static void test_bytecode_dump_records_throw_literals(void) {
    static const char source[] =
        "int32 fail = () -> {\n"
        "    throw \"boom\";\n"
        "    return 0;\n"
        "};\n"
        "start(string[] args) -> fail();\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump), "build throw bytecode dump text");
    ASSERT_CONTAINS("literal kind=string text=\"boom\"", dump, "string throw literals are interned in the constant pool");
    ASSERT_CONTAINS("BC_THROW const(", dump, "throw terminators lower into bytecode throw opcodes");
    free(dump);
}

static void test_bytecode_dump_lowers_union_new_instructions(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "Option<int32> x = Option.Some(42);\n"
        "Option<int32> y = Option.None;\n"
        "start(string[] args) -> 0;\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump), "build union bytecode dump text");
    ASSERT_CONTAINS("type_desc name=Option generic_params=[int32] variants=[Some:int32, None:void]",
                    dump,
                    "union descriptor constants capture variant names and payload tags");
    ASSERT_CONTAINS("BC_UNION_NEW t0 <- type_desc(0) tag=0 payload=", dump,
                    "payload variant lowers to BC_UNION_NEW with tag 0");
    ASSERT_CONTAINS("BC_UNION_NEW t1 <- type_desc(0) tag=1 payload=", dump,
                    "non-payload variant lowers to BC_UNION_NEW with tag 1");
    ASSERT_CONTAINS("BC_STORE_GLOBAL", dump,
                    "union values stored to globals via BC_STORE_GLOBAL");
    free(dump);
}

static void test_bytecode_dump_lowers_hetero_array_literals(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true, \"hello\"];\n"
        "    return 0;\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump), "build hetero array bytecode dump text");
    ASSERT_CONTAINS("literal kind=bool value=true", dump,
                    "hetero array bytecode interns bool constants");
    ASSERT_CONTAINS("type_desc name=arr generic_params=[raw_word] variants=[int32, bool, string]", dump,
                    "hetero arrays intern shared descriptor constants with element runtime tags");
    ASSERT_CONTAINS("BC_HETERO_ARRAY_NEW t0 <- type_desc(3) [const(c0:integer(\"1\")), const(c1:bool(true)), const(c2:string(\"hello\"))]",
                    dump,
                    "arr<?> literals lower to BC_HETERO_ARRAY_NEW with descriptor and element constants");
    ASSERT_CONTAINS("BC_STORE_LOCAL local(1:mixed) <- t0", dump,
                    "hetero array temporaries store into the target local");
    free(dump);
}

static void test_bytecode_dump_lowers_hetero_array_index_reads(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true, \"hello\"];\n"
        "    return int32(mixed[0]);\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump),
                 "build hetero array indexing bytecode dump text");
    ASSERT_CONTAINS("BC_HETERO_ARRAY_NEW", dump,
                    "hetero array indexing still lowers through descriptor-backed construction");
    ASSERT_CONTAINS("BC_INDEX_LOAD", dump,
                    "hetero array reads reuse the existing bytecode index-load path");
    ASSERT_CONTAINS("BC_CAST", dump,
                    "hetero array reads can feed runtime casts from external values");
    free(dump);
}

static void test_bytecode_dump_lowers_union_tag_and_payload_access(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> value = Option.Some(7);\n"
        "    int32 tag = value.tag;\n"
        "    int32 payload_value = int32(value.payload);\n"
        "    return tag + payload_value;\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump),
                 "build union access bytecode dump text");
    ASSERT_CONTAINS("BC_UNION_GET_TAG", dump,
                    "union .tag lowers to a dedicated bytecode opcode");
    ASSERT_CONTAINS("BC_UNION_GET_PAYLOAD", dump,
                    "union .payload lowers to a dedicated bytecode opcode");
    ASSERT_TRUE(strstr(dump, "BC_MEMBER") == NULL,
                "union metadata access no longer falls back to generic member bytecode");
    free(dump);
}

static void test_bytecode_dump_lowers_type_query_builtins(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    arr<?> mixed = [1, true];\n"
        "    string typeName = typeof(values);\n"
        "    bool same = issametype(values, cdr(values));\n"
        "    return (isarray(mixed) && same) ? int32(typeName.length) : 0;\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump),
                 "build type-query bytecode dump text");
    ASSERT_CONTAINS("BC_TYPEOF", dump,
                    "typeof lowers to a dedicated bytecode opcode");
    ASSERT_CONTAINS("BC_ISARRAY", dump,
                    "isarray lowers to a dedicated bytecode opcode");
    ASSERT_CONTAINS("BC_ISSAMETYPE", dump,
                    "issametype lowers to a dedicated bytecode opcode");
    ASSERT_CONTAINS("literal kind=string text=\"int32[]\"", dump,
                    "type query helpers intern canonical static type metadata");
    free(dump);
}

int main(void) {
    printf("Running bytecode dump tests...\n\n");

    RUN_TEST(test_bytecode_dump_lowers_init_start_and_lambda_units);
    RUN_TEST(test_bytecode_dump_records_throw_literals);
    RUN_TEST(test_bytecode_dump_lowers_union_new_instructions);
    RUN_TEST(test_bytecode_dump_lowers_hetero_array_literals);
    RUN_TEST(test_bytecode_dump_lowers_hetero_array_index_reads);
    RUN_TEST(test_bytecode_dump_lowers_union_tag_and_payload_access);
    RUN_TEST(test_bytecode_dump_lowers_type_query_builtins);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
