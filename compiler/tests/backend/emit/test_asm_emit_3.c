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

