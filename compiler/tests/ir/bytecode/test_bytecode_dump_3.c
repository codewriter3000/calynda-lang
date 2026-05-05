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

#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                              \
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

static size_t count_substring(const char *haystack, const char *needle) {
    size_t count = 0;
    const char *cursor = haystack;

    while (cursor && needle && *needle) {
        cursor = strstr(cursor, needle);
        if (!cursor) {
            break;
        }
        count++;
        cursor += strlen(needle);
    }

    return count;
}

static void test_bytecode_build_rejects_programs_with_mir_errors(void) {
    static const char source[] = "start(string[] args) -> missing + 1;\n";
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    BytecodeProgram bytecode_program;
    const MirBuildError *mir_error;
    char expected[256];
    char diagnostic[256];

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    bytecode_program_init(&bytecode_program);
    parser_init(&parser, source);

    REQUIRE_TRUE(parser_parse_program(&parser, &ast_program), "parse invalid bytecode input");
    REQUIRE_TRUE(symbol_table_build(&symbols, &ast_program), "build symbols for invalid bytecode input");
    ASSERT_TRUE(!type_checker_check_program(&checker, &ast_program, &symbols),
                "type check fails before bytecode lowering");
    ASSERT_TRUE(!hir_build_program(&hir_program, &ast_program, &symbols, &checker),
                "HIR lowering rejects type-invalid bytecode input");
    ASSERT_TRUE(!mir_build_program(&mir_program, &hir_program, false),
                "MIR lowering rejects HIR-invalid bytecode input");
    mir_error = mir_get_error(&mir_program);
    REQUIRE_TRUE(mir_error != NULL, "MIR exposes structured error for invalid bytecode input");
    REQUIRE_TRUE(mir_format_error(mir_error, expected, sizeof(expected)),
                 "format MIR error for bytecode propagation test");
    ASSERT_TRUE(!bytecode_build_program(&bytecode_program, &mir_program),
                "bytecode lowering rejects MIR-invalid program");
    REQUIRE_TRUE(bytecode_format_error(bytecode_get_error(&bytecode_program),
                                       diagnostic,
                                       sizeof(diagnostic)),
                 "format bytecode build error");
    ASSERT_TRUE(strcmp(expected, diagnostic) == 0,
                "bytecode build error preserves the MIR diagnostic text");
    ASSERT_TRUE(bytecode_get_error(&bytecode_program)->primary_span.start_line ==
                    mir_error->primary_span.start_line &&
                bytecode_get_error(&bytecode_program)->primary_span.start_column ==
                    mir_error->primary_span.start_column,
                "bytecode build error preserves the primary diagnostic span");

    bytecode_program_free(&bytecode_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
}

static void test_bytecode_dump_lowers_branch_and_goto_terminators(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    bool flag = true;\n"
        "    int32 value = flag ? 1 : 2;\n"
        "    return value;\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_bytecode_from_source(source, &dump),
                 "build branching bytecode dump text");
    ASSERT_TRUE(strstr(dump, "BC_BRANCH local(1:flag) -> bb1, bb2") != NULL,
                "ternary branch lowering emits BC_BRANCH with both successor blocks");
    ASSERT_TRUE(count_substring(dump, "BC_JUMP bb3") == 2,
                "both MIR goto edges lower into BC_JUMP terminators that rejoin the merge block");
    free(dump);
}

int main(void) {
    printf("Running bytecode dump tests (part 3)...\n\n");

    RUN_TEST(test_bytecode_build_rejects_programs_with_mir_errors);
    RUN_TEST(test_bytecode_dump_lowers_branch_and_goto_terminators);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
