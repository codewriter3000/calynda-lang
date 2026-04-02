#define _POSIX_C_SOURCE 200809L

#include "../src/asm_emit.h"
#include "../src/codegen.h"
#include "../src/hir.h"
#include "../src/lir.h"
#include "../src/machine.h"
#include "../src/mir.h"
#include "../src/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static bool build_assembly_from_source(const char *source, char **assembly_out) {
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
        !mir_build_program(&mir_program, &hir_program) ||
        !lir_build_program(&lir_program, &mir_program) ||
        !codegen_build_program(&codegen_program, &lir_program) ||
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

static bool compile_assembly_text(const char *assembly) {
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

static void test_asm_emit_compiles_minimal_direct_program(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    int32 total = add(1, 2);\n"
        "    return total;\n"
        "};\n";
    char *assembly;

    REQUIRE_TRUE(build_assembly_from_source(source, &assembly), "emit minimal assembly text");
    ASSERT_CONTAINS(".intel_syntax noprefix", assembly, "assembly writer emits Intel syntax");
    ASSERT_CONTAINS(".globl calynda_unit_add", assembly, "assembly writer exports the lowered add unit symbol");
    ASSERT_CONTAINS(".globl calynda_program_start", assembly, "assembly writer emits a callable start wrapper");
    ASSERT_CONTAINS(".globl main", assembly, "assembly writer emits a process entry point");
    ASSERT_CONTAINS("call calynda_unit_add", assembly, "direct user-function calls lower to native call instructions");
    ASSERT_CONTAINS("QWORD PTR [rbp - 16]", assembly, "frame slots lower to concrete stack addresses");
    ASSERT_TRUE(compile_assembly_text(assembly), "emitted minimal assembly assembles with gcc -c");
    free(assembly);
}

static void test_asm_emit_compiles_runtime_backed_program(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "int32 seed = 1;\n"
        "start(string[] args) -> {\n"
        "    bool flag = true;\n"
        "    var values = [seed, 2, 3];\n"
        "    int32 chosen = flag ? values[1] : values[0];\n"
        "    var render = (int32 x) -> `value ${chosen + ((int32 y) -> y + x)(1)}`;\n"
        "    values[0] += chosen;\n"
        "    stdlib.print(render(2));\n"
        "    return values[0];\n"
        "};\n";
    char *assembly;

    REQUIRE_TRUE(build_assembly_from_source(source, &assembly), "emit runtime-backed assembly text");
    ASSERT_CONTAINS("call __calynda_rt_array_literal", assembly, "array literals lower to runtime helper calls in real assembly");
    ASSERT_CONTAINS("call __calynda_rt_template_build", assembly, "templates lower to runtime helper calls in real assembly");
    ASSERT_CONTAINS("call __calynda_rt_member_load", assembly, "member access lowers to runtime helper calls in real assembly");
    ASSERT_CONTAINS("call __calynda_rt_call_callable", assembly, "callable dispatch lowers to runtime helper calls in real assembly");
    ASSERT_CONTAINS("call calynda_rt_start_process", assembly, "assembly writer emits the runtime startup bridge");
    ASSERT_CONTAINS("calynda_closure_start_lambda", assembly, "lambda closures lower through callable wrapper symbols");
    ASSERT_CONTAINS("calynda_global_stdlib:", assembly, "imported package aliases get concrete data symbols");
    ASSERT_CONTAINS(".Ltxt_", assembly, "template text lowers into concrete rodata byte labels");
    ASSERT_TRUE(compile_assembly_text(assembly), "emitted runtime-backed assembly assembles with gcc -c");
    free(assembly);
}

static void test_asm_emit_lowers_string_literals_into_runtime_objects(void) {
    static const char source[] =
        "int32 fail = () -> {\n"
        "    throw \"boom\";\n"
        "    return 0;\n"
        "};\n"
        "start(string[] args) -> fail();\n";
    char *assembly;

    REQUIRE_TRUE(build_assembly_from_source(source, &assembly), "emit throw assembly text");
    ASSERT_CONTAINS(".Lstr_obj_", assembly, "string literals lower into concrete runtime string objects");
    ASSERT_CONTAINS(".Lstr_bytes_", assembly, "string literal objects carry separate byte storage labels");
    ASSERT_TRUE(compile_assembly_text(assembly), "emitted throw assembly assembles with gcc -c");
    free(assembly);
}

int main(void) {
    printf("Running asm emit tests...\n\n");

    RUN_TEST(test_asm_emit_compiles_minimal_direct_program);
    RUN_TEST(test_asm_emit_compiles_runtime_backed_program);
    RUN_TEST(test_asm_emit_lowers_string_literals_into_runtime_objects);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}