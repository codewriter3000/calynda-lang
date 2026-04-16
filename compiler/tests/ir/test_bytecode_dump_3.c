#include "bytecode.h"
#include "hir.h"
#include "mir.h"
#include "parser.h"

#include <stdio.h>
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

int main(void) {
    printf("Running bytecode dump tests (part 3)...\n\n");

    RUN_TEST(test_bytecode_build_rejects_programs_with_mir_errors);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
