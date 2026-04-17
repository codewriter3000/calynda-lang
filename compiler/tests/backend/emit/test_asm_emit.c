#define _POSIX_C_SOURCE 200809L

#include "asm_emit.h"
#include "codegen.h"
#include "hir.h"
#include "lir.h"
#include "machine.h"
#include "mir.h"
#include "parser.h"
#include "target.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

bool build_assembly_from_source_with_target(const char *source,
                                                   char **assembly_out,
                                                   const TargetDescriptor *target);

bool build_assembly_from_source(const char *source, char **assembly_out) {
    return build_assembly_from_source_with_target(source, assembly_out, target_get_default());
}

bool build_assembly_from_source_with_target(const char *source,
                                                   char **assembly_out,
                                                   const TargetDescriptor *target) {
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

    if (!source || !assembly_out) {
        return false;
    }

    *assembly_out = NULL;
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
        !codegen_build_program(&codegen_program, &lir_program, target) ||
        !machine_build_program(&machine_program, &lir_program, &codegen_program)) {
        goto cleanup;
    }

    *assembly_out = asm_emit_program_to_string(&machine_program);
    ok = *assembly_out != NULL;

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

bool compile_assembly_text(const char *assembly) {
    char source_template[] = "/tmp/calynda-asm-XXXXXX";
    char object_path[64];
    char command[256];
    FILE *file;
    int fd;
    int exit_code;

    fd = mkstemp(source_template);
    if (fd < 0) {
        return false;
    }

    file = fdopen(fd, "wb");
    if (!file) {
        close(fd);
        unlink(source_template);
        return false;
    }
    if (fputs(assembly, file) == EOF || fclose(file) != 0) {
        unlink(source_template);
        return false;
    }

    snprintf(object_path, sizeof(object_path), "%s.o", source_template);
    snprintf(command,
             sizeof(command),
             "gcc -x assembler -c %s -o %s > /dev/null 2>&1",
             source_template,
             object_path);
    exit_code = system(command);
    unlink(source_template);
    unlink(object_path);
    return exit_code == 0;
}

void test_asm_emit_compiles_minimal_direct_program(void);
void test_asm_emit_compiles_runtime_backed_program(void);
void test_asm_emit_lowers_string_literals_into_runtime_objects(void);
void test_asm_emit_aarch64_minimal_program(void);
void test_asm_emit_aarch64_runtime_backed_program(void);
void test_asm_emit_aarch64_register_usage(void);
void test_asm_emit_asm_decl_emits_raw_body(void);
void test_asm_emit_asm_decl_no_params(void);
void test_asm_emit_boot_emits_start_label(void);
void test_asm_emit_boot_aarch64_emits_start_label(void);
void test_asm_emit_boot_rejects_unknown_imported_member(void);
void test_asm_emit_boot_rejects_multi_arg_imported_print(void);
void test_asm_emit_riscv64_minimal_program(void);
void test_asm_emit_riscv64_runtime_backed_program(void);
void test_asm_emit_boot_riscv64_emits_start_label(void);
void test_asm_emit_boot_riscv64_statically_lowers_imported_print(void);
void test_asm_emit_preserves_upstream_error_spans(void);
void test_cross_asm_riscv64_assembles(void);
void test_cross_asm_aarch64_assembles(void);
void test_cross_build_riscv64_boot_links(void);
void test_cross_run_riscv64_boot_qemu(void);


int main(void) {
    printf("Running asm emit tests...\n\n");

    RUN_TEST(test_asm_emit_compiles_minimal_direct_program);
    RUN_TEST(test_asm_emit_compiles_runtime_backed_program);
    RUN_TEST(test_asm_emit_lowers_string_literals_into_runtime_objects);

    printf("\n  ARM64 tests...\n");
    RUN_TEST(test_asm_emit_aarch64_minimal_program);
    RUN_TEST(test_asm_emit_aarch64_runtime_backed_program);
    RUN_TEST(test_asm_emit_aarch64_register_usage);

    printf("\n  Asm decl tests...\n");
    RUN_TEST(test_asm_emit_asm_decl_emits_raw_body);
    RUN_TEST(test_asm_emit_asm_decl_no_params);

    printf("\n  Boot entry tests...\n");
    RUN_TEST(test_asm_emit_boot_emits_start_label);
    RUN_TEST(test_asm_emit_boot_aarch64_emits_start_label);
    RUN_TEST(test_asm_emit_boot_rejects_unknown_imported_member);
    RUN_TEST(test_asm_emit_boot_rejects_multi_arg_imported_print);
    RUN_TEST(test_asm_emit_boot_riscv64_emits_start_label);
    RUN_TEST(test_asm_emit_boot_riscv64_statically_lowers_imported_print);
    RUN_TEST(test_asm_emit_preserves_upstream_error_spans);

    printf("\n  RV64 tests...\n");
    RUN_TEST(test_asm_emit_riscv64_minimal_program);
    RUN_TEST(test_asm_emit_riscv64_runtime_backed_program);

    printf("\n  Cross-compilation tests (skipped if toolchain absent)...\n");
    RUN_TEST(test_cross_asm_riscv64_assembles);
    RUN_TEST(test_cross_asm_aarch64_assembles);
    RUN_TEST(test_cross_build_riscv64_boot_links);
    RUN_TEST(test_cross_run_riscv64_boot_qemu);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
