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

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

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
bool build_assembly_from_source(const char *source, char **assembly_out);
bool build_assembly_from_source_with_target(const char *source,
                                                   char **assembly_out,
                                                   const TargetDescriptor *target);
bool compile_assembly_text(const char *assembly);


void test_asm_emit_asm_decl_no_params(void) {
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

void test_asm_emit_boot_emits_start_label(void) {
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


void test_asm_emit_boot_aarch64_emits_start_label(void) {
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


/* ------------------------------------------------------------------ */
/*  RV64 tests                                                         */
/* ------------------------------------------------------------------ */

static bool build_riscv64_assembly(const char *source, char **assembly_out) {
    return build_assembly_from_source_with_target(source, assembly_out,
               target_get_descriptor(TARGET_KIND_RISCV64_LP64D_ELF));
}

void test_asm_emit_riscv64_minimal_program(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    int32 total = add(1, 2);\n"
        "    return total;\n"
        "};\n";
    char *assembly;

    REQUIRE_TRUE(build_riscv64_assembly(source, &assembly), "emit rv64 assembly text");
    /* Must NOT contain x86_64 or ARM64 markers */
    ASSERT_TRUE(strstr(assembly, ".intel_syntax") == NULL,
                "rv64 output must not contain .intel_syntax");
    ASSERT_TRUE(strstr(assembly, "QWORD PTR") == NULL,
                "rv64 output must not contain x86_64 memory operands");
    ASSERT_TRUE(strstr(assembly, "stp ") == NULL,
                "rv64 output must not contain arm64 stp instructions");
    /* RV64 structural markers */
    ASSERT_CONTAINS(".globl calynda_unit_add", assembly,
                    "rv64 output exports the add unit symbol");
    ASSERT_CONTAINS(".globl calynda_program_start", assembly,
                    "rv64 output emits a callable start wrapper");
    ASSERT_CONTAINS(".globl main", assembly,
                    "rv64 output emits a process entry point");
    /* RV64 ABI: sd/ld prologue, call instruction, s0 frame pointer */
    ASSERT_CONTAINS("sd ra,", assembly,
                    "rv64 prologue saves return address");
    ASSERT_CONTAINS("sd s0,", assembly,
                    "rv64 prologue saves frame pointer");
    ASSERT_CONTAINS("addi s0, sp,", assembly,
                    "rv64 prologue sets up frame pointer");
    ASSERT_CONTAINS("call calynda_unit_add", assembly,
                    "rv64 direct calls use call instruction");
    free(assembly);
}

void test_asm_emit_riscv64_runtime_backed_program(void) {
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

    REQUIRE_TRUE(build_riscv64_assembly(source, &assembly), "emit rv64 runtime assembly text");
    ASSERT_CONTAINS("call __calynda_rt_array_literal", assembly,
                    "rv64 arrays lower to call runtime helpers");
    ASSERT_CONTAINS("call __calynda_rt_template_build", assembly,
                    "rv64 templates lower to call runtime helpers");
    ASSERT_CONTAINS("call calynda_rt_start_process", assembly,
                    "rv64 start wrapper calls runtime startup");
    ASSERT_CONTAINS("la a0, calynda_program_start", assembly,
                    "rv64 entry glue uses la for PIC addressing");
    ASSERT_CONTAINS("calynda_closure_start_lambda", assembly,
                    "rv64 lambda closures lower through callable wrapper symbols");
    free(assembly);
}

void test_asm_emit_boot_riscv64_emits_start_label(void) {
    static const char source[] = "boot() -> 42;\n";
    char *assembly;

    REQUIRE_TRUE(build_riscv64_assembly(source, &assembly),
                 "emit boot rv64 assembly text");
    ASSERT_CONTAINS(".globl _start", assembly, "boot rv64 emits _start label");
    ASSERT_CONTAINS("ecall", assembly, "boot rv64 exits via ecall");
    ASSERT_TRUE(strstr(assembly, ".globl main") == NULL,
                "boot rv64 does not emit main entry point");
    ASSERT_TRUE(strstr(assembly, "calynda_program_start") == NULL,
                "boot rv64 does not emit calynda_program_start wrapper");
    free(assembly);
}

