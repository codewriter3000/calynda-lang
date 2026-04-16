#include "codegen.h"
#include "hir.h"
#include "lir.h"
#include "machine.h"
#include "mir.h"
#include "parser.h"
#include "runtime_abi.h"
#include "target.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

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

bool build_machine_dump_from_source(const char *source, char **dump_out) {
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    CodegenProgram codegen_program;
    MachineProgram machine_program;
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
    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    machine_program_init(&machine_program);
    parser_init(&parser, source);

    if (!parser_parse_program(&parser, &ast_program) ||
        !symbol_table_build(&symbols, &ast_program) ||
        !type_checker_check_program(&checker, &ast_program, &symbols) ||
        !hir_build_program(&hir_program, &ast_program, &symbols, &checker) ||
        !mir_build_program(&mir_program, &hir_program, false) ||
        !lir_build_program(&lir_program, &mir_program) ||
        !codegen_build_program(&codegen_program, &lir_program, target_get_default()) ||
        !machine_build_program(&machine_program, &lir_program, &codegen_program)) {
        goto cleanup;
    }

    *dump_out = machine_dump_program_to_string(&machine_program);
    ok = *dump_out != NULL;

cleanup:
    machine_program_free(&machine_program);
    codegen_program_free(&codegen_program);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
    return ok;
}

void test_runtime_abi_dump_defines_helper_surface(void);
void test_machine_dump_emits_minimal_direct_instruction_stream(void);
void test_machine_dump_routes_runtime_backed_ops_through_helpers(void);
void test_machine_dump_includes_static_hetero_array_type_descriptors(void);
void test_machine_dump_includes_static_union_type_descriptors(void);
void test_machine_dump_routes_throw_terminator_to_runtime_helper(void);
void test_machine_dump_routes_union_tag_and_payload_helpers(void);

void test_machine_dump_includes_static_hetero_array_type_descriptors(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    arr<?> mixed = [1, true, \"hello\"];\n"
        "    return 0;\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_machine_dump_from_source(source, &dump), "emit hetero array machine program");
    ASSERT_CONTAINS("mov rdi, typedesc(arr|1|g0:raw_word|0:int32|1:bool|2:string)", dump,
                    "hetero array construction passes a static type descriptor operand");
    ASSERT_CONTAINS("call __calynda_rt_hetero_array_new", dump,
                    "hetero array construction still lowers through the runtime ABI");
    free(dump);
}

void test_machine_dump_includes_static_union_type_descriptors(void) {
    static const char source[] =
        "union Option<T> { Some(T), None };\n"
        "start(string[] args) -> {\n"
        "    Option<int32> value = Option.Some(42);\n"
        "    return 0;\n"
        "};\n";
    char *dump;

    REQUIRE_TRUE(build_machine_dump_from_source(source, &dump), "emit union machine program");
    ASSERT_CONTAINS("mov rdi, typedesc(Option|1|g0:int32|Some:int32|None:void)", dump,
                    "union construction passes a static type descriptor operand");
    ASSERT_CONTAINS("call __calynda_rt_union_new", dump,
                    "union construction still lowers through the runtime ABI");
    free(dump);
}


int main(void) {
    printf("Running machine dump tests...\n\n");

    RUN_TEST(test_runtime_abi_dump_defines_helper_surface);
    RUN_TEST(test_machine_dump_emits_minimal_direct_instruction_stream);
    RUN_TEST(test_machine_dump_routes_runtime_backed_ops_through_helpers);
    RUN_TEST(test_machine_dump_includes_static_hetero_array_type_descriptors);
    RUN_TEST(test_machine_dump_includes_static_union_type_descriptors);
    RUN_TEST(test_machine_dump_routes_throw_terminator_to_runtime_helper);
    RUN_TEST(test_machine_dump_routes_union_tag_and_payload_helpers);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
