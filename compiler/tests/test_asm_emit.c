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

static bool build_assembly_from_source_with_target(const char *source,
                                                   char **assembly_out,
                                                   const TargetDescriptor *target);

static bool build_assembly_from_source(const char *source, char **assembly_out) {
    return build_assembly_from_source_with_target(source, assembly_out, target_get_default());
}

static bool build_assembly_from_source_with_target(const char *source,
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
        !mir_build_program(&mir_program, &hir_program) ||
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

/* ---- ARM64 tests ---- */

static bool build_aarch64_assembly(const char *source, char **assembly_out) {
    TargetKind kind = TARGET_KIND_AARCH64_AAPCS_ELF;
    return build_assembly_from_source_with_target(source, assembly_out,
                                                  target_get_descriptor(kind));
}

static void test_asm_emit_aarch64_minimal_program(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    int32 total = add(1, 2);\n"
        "    return total;\n"
        "};\n";
    char *assembly;

    REQUIRE_TRUE(build_aarch64_assembly(source, &assembly), "emit aarch64 assembly text");
    /* Must NOT contain x86_64 markers */
    ASSERT_TRUE(strstr(assembly, ".intel_syntax") == NULL,
                "aarch64 output must not contain .intel_syntax");
    ASSERT_TRUE(strstr(assembly, "QWORD PTR") == NULL,
                "aarch64 output must not contain x86_64 memory operands");
    /* ARM64 structural markers */
    ASSERT_CONTAINS(".globl calynda_unit_add", assembly,
                    "aarch64 output exports the add unit symbol");
    ASSERT_CONTAINS(".globl calynda_program_start", assembly,
                    "aarch64 output emits a callable start wrapper");
    ASSERT_CONTAINS(".globl main", assembly,
                    "aarch64 output emits a process entry point");
    /* ARM64 ABI: stp/ldp prologue, bl calls, x-registers */
    ASSERT_CONTAINS("stp x29, x30", assembly,
                    "aarch64 prologue saves frame pointer and link register");
    ASSERT_CONTAINS("bl calynda_unit_add", assembly,
                    "aarch64 direct calls use bl instruction");
    ASSERT_CONTAINS("mov x29, sp", assembly,
                    "aarch64 prologue sets up frame pointer");
    free(assembly);
}

static void test_asm_emit_aarch64_runtime_backed_program(void) {
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

    REQUIRE_TRUE(build_aarch64_assembly(source, &assembly), "emit aarch64 runtime-backed assembly text");
    /* ARM64 runtime calls use bl instead of call */
    ASSERT_CONTAINS("bl __calynda_rt_array_literal", assembly,
                    "aarch64 array literals lower to bl runtime helper calls");
    ASSERT_CONTAINS("bl __calynda_rt_template_build", assembly,
                    "aarch64 templates lower to bl runtime helper calls");
    ASSERT_CONTAINS("bl __calynda_rt_member_load", assembly,
                    "aarch64 member access lowers to bl runtime helper calls");
    ASSERT_CONTAINS("bl calynda_rt_start_process", assembly,
                    "aarch64 start wrapper calls runtime startup via bl");
    /* ARM64 entry glue uses adrp+add for PIC addressing */
    ASSERT_CONTAINS("adrp", assembly,
                    "aarch64 entry glue uses PC-relative addressing");
    free(assembly);
}

static void test_asm_emit_aarch64_register_usage(void) {
    static const char source[] =
        "int32 compute = (int32 a, int32 b, int32 c) -> a + b * c;\n"
        "start(string[] args) -> compute(10, 20, 30);\n";
    char *assembly;

    REQUIRE_TRUE(build_aarch64_assembly(source, &assembly), "emit aarch64 register assembly text");
    /* ARM64 uses x-registers for operations */
    ASSERT_CONTAINS("add ", assembly,
                    "aarch64 output uses add instructions");
    ASSERT_CONTAINS("mul ", assembly,
                    "aarch64 output uses mul instructions");
    /* Arguments arrive in x0-x2 per AAPCS64 */
    ASSERT_TRUE(strstr(assembly, "rdi") == NULL && strstr(assembly, "rsi") == NULL,
                "aarch64 output must not contain x86_64 register names");
    free(assembly);
}

static void test_asm_emit_asm_decl_emits_raw_body(void) {
    static const char source[] =
        "int32 my_add = asm(int32 a, int32 b) -> {\n"
        "    mov eax, edi\n"
        "    add eax, esi\n"
        "    ret\n"
        "};\n"
        "start(string[] args) -> my_add(1, 2);\n";
    char *assembly;

    REQUIRE_TRUE(build_assembly_from_source(source, &assembly), "emit asm decl assembly text");
    ASSERT_CONTAINS(".globl calynda_unit_my_add", assembly, "asm decl gets a .globl directive");
    ASSERT_CONTAINS("mov eax, edi", assembly, "asm body line 1 emitted verbatim");
    ASSERT_CONTAINS("add eax, esi", assembly, "asm body line 2 emitted verbatim");
    ASSERT_CONTAINS("ret", assembly, "asm body line 3 emitted verbatim");
    ASSERT_CONTAINS("call calynda_unit_my_add", assembly, "start body calls asm function");
    ASSERT_TRUE(compile_assembly_text(assembly), "emitted asm-decl assembly assembles with gcc -c");
    free(assembly);
}

static void test_asm_emit_asm_decl_no_params(void) {
    static const char source[] =
        "int32 get_zero = asm() -> {\n"
        "    xor eax, eax\n"
        "    ret\n"
        "};\n"
        "start(string[] args) -> get_zero();\n";
    char *assembly;

    REQUIRE_TRUE(build_assembly_from_source(source, &assembly), "emit zero-param asm decl assembly text");
    ASSERT_CONTAINS(".globl calynda_unit_get_zero", assembly, "zero-param asm decl gets .globl");
    ASSERT_CONTAINS("xor eax, eax", assembly, "zero-param asm body emitted verbatim");
    ASSERT_TRUE(compile_assembly_text(assembly), "emitted zero-param asm assembly assembles with gcc -c");
    free(assembly);
}

/* ------------------------------------------------------------------ */
/*  Boot entry tests                                                  */
/* ------------------------------------------------------------------ */

static void test_asm_emit_boot_emits_start_label(void) {
    static const char source[] = "boot() -> 42;\n";
    char *assembly;

    REQUIRE_TRUE(build_assembly_from_source(source, &assembly), "emit boot assembly text");
    ASSERT_CONTAINS(".globl _start", assembly, "boot emits _start label");
    ASSERT_CONTAINS("syscall", assembly, "boot exits via syscall");
    ASSERT_TRUE(strstr(assembly, ".globl main") == NULL,
                "boot does not emit main entry point");
    ASSERT_TRUE(strstr(assembly, "calynda_program_start") == NULL,
                "boot does not emit calynda_program_start wrapper");
    ASSERT_TRUE(strstr(assembly, "calynda_rt_start_process") == NULL,
                "boot does not call runtime start process");
    ASSERT_TRUE(compile_assembly_text(assembly), "emitted boot assembly assembles with gcc -c");
    free(assembly);
}

static void test_asm_emit_boot_aarch64_emits_start_label(void) {
    static const char source[] = "boot() -> 42;\n";
    char *assembly;

    REQUIRE_TRUE(build_assembly_from_source_with_target(
                     source, &assembly,
                     target_get_descriptor(TARGET_KIND_AARCH64_AAPCS_ELF)),
                 "emit boot aarch64 assembly text");
    ASSERT_CONTAINS(".globl _start", assembly, "boot aarch64 emits _start label");
    ASSERT_CONTAINS("svc", assembly, "boot aarch64 exits via svc");
    ASSERT_TRUE(strstr(assembly, ".globl main") == NULL,
                "boot aarch64 does not emit main entry point");
    ASSERT_TRUE(strstr(assembly, "calynda_program_start") == NULL,
                "boot aarch64 does not emit calynda_program_start wrapper");
    free(assembly);
}

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

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}