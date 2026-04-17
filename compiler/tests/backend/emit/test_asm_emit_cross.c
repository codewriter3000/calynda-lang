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
#include <sys/wait.h>
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

bool build_assembly_from_source_with_target(const char *source,
                                            char **assembly_out,
                                            const TargetDescriptor *target);

/* ------------------------------------------------------------------ */
/*  Cross-compiler helpers                                             */
/* ------------------------------------------------------------------ */

static bool cross_compiler_available(const char *cc) {
    char command[128];
    snprintf(command, sizeof(command), "which %s > /dev/null 2>&1", cc);
    return system(command) == 0;
}

static bool compile_assembly_text_cross(const char *assembly, const char *cc) {
    char src_template[] = "/tmp/calynda-xasm-XXXXXX";
    char object_path[64];
    char command[256];
    FILE *file;
    int fd, exit_code;

    fd = mkstemp(src_template);
    if (fd < 0) return false;

    file = fdopen(fd, "wb");
    if (!file) { close(fd); unlink(src_template); return false; }
    if (fputs(assembly, file) == EOF || fclose(file) != 0) {
        unlink(src_template);
        return false;
    }

    snprintf(object_path, sizeof(object_path), "%s.o", src_template);
    snprintf(command, sizeof(command),
             "%s -x assembler -c %s -o %s > /dev/null 2>&1",
             cc, src_template, object_path);
    exit_code = system(command);
    unlink(src_template);
    unlink(object_path);
    return exit_code == 0;
}

static bool cross_link_boot(const char *assembly, const char *cc,
                            char *exe_path, size_t exe_size) {
    char src_template[] = "/tmp/calynda-xlink-XXXXXX";
    char out_template[] = "/tmp/calynda-xbin-XXXXXX";
    char command[512];
    FILE *file;
    int fd, out_fd, exit_code;

    fd = mkstemp(src_template);
    if (fd < 0) return false;
    file = fdopen(fd, "wb");
    if (!file) { close(fd); unlink(src_template); return false; }
    if (fputs(assembly, file) == EOF || fclose(file) != 0) {
        unlink(src_template);
        return false;
    }

    out_fd = mkstemp(out_template);
    if (out_fd < 0) { unlink(src_template); return false; }
    close(out_fd);

    snprintf(command, sizeof(command),
             "%s -nostdlib -static -x assembler %s -o %s > /dev/null 2>&1",
             cc, src_template, out_template);
    exit_code = system(command);
    unlink(src_template);

    if (exit_code != 0) {
        unlink(out_template);
        return false;
    }

    if (strlen(out_template) + 1 > exe_size) {
        unlink(out_template);
        return false;
    }
    memcpy(exe_path, out_template, strlen(out_template) + 1);
    return true;
}

/* ------------------------------------------------------------------ */
/*  RV64 cross-assembly validation                                     */
/* ------------------------------------------------------------------ */

void test_cross_asm_riscv64_assembles(void) {
    static const char *cc = "riscv64-linux-gnu-gcc";
    static const char source[] = "boot -> 42;\n";
    char *assembly;

    if (!cross_compiler_available(cc)) {
        printf("    [SKIP] %s not found\n", cc);
        return;
    }

    REQUIRE_TRUE(build_assembly_from_source_with_target(
                     source, &assembly,
                     target_get_descriptor(TARGET_KIND_RISCV64_LP64D_ELF)),
                 "emit rv64 boot assembly for cross-asm validation");
    ASSERT_TRUE(compile_assembly_text_cross(assembly, cc),
                "rv64 boot assembly assembles with riscv64-linux-gnu-gcc -c");
    free(assembly);
}

/* ------------------------------------------------------------------ */
/*  AArch64 cross-assembly validation                                  */
/* ------------------------------------------------------------------ */

void test_cross_asm_aarch64_assembles(void) {
    static const char *cc = "aarch64-linux-gnu-gcc";
    static const char source[] = "boot -> 42;\n";
    char *assembly;

    if (!cross_compiler_available(cc)) {
        printf("    [SKIP] %s not found\n", cc);
        return;
    }

    REQUIRE_TRUE(build_assembly_from_source_with_target(
                     source, &assembly,
                     target_get_descriptor(TARGET_KIND_AARCH64_AAPCS_ELF)),
                 "emit aarch64 boot assembly for cross-asm validation");
    ASSERT_TRUE(compile_assembly_text_cross(assembly, cc),
                "aarch64 boot assembly assembles with aarch64-linux-gnu-gcc -c");
    free(assembly);
}

/* ------------------------------------------------------------------ */
/*  RV64 cross-link validation                                         */
/* ------------------------------------------------------------------ */

void test_cross_build_riscv64_boot_links(void) {
    static const char *cc = "riscv64-linux-gnu-gcc";
    static const char source[] = "boot -> 42;\n";
    char *assembly;
    char exe_path[128];

    if (!cross_compiler_available(cc)) {
        printf("    [SKIP] %s not found\n", cc);
        return;
    }

    REQUIRE_TRUE(build_assembly_from_source_with_target(
                     source, &assembly,
                     target_get_descriptor(TARGET_KIND_RISCV64_LP64D_ELF)),
                 "emit rv64 boot assembly for cross-link");
    ASSERT_TRUE(cross_link_boot(assembly, cc, exe_path, sizeof(exe_path)),
                "rv64 boot program cross-links to static executable");
    unlink(exe_path);
    free(assembly);
}

/* ------------------------------------------------------------------ */
/*  RV64 QEMU user-mode execution — replaced by freestanding check   */
/* ------------------------------------------------------------------ */

/*
 * After the alpha.2 freestanding boot() fix, the RISC-V _start entry no
 * longer emits a Linux exit ecall.  The boot unit's return value is NOT
 * forwarded to the OS; instead, _start spins in an infinite loop after
 * the boot unit returns.  This is the correct bare-metal contract for
 * pointer/MMIO firmware targets.
 *
 * This test verifies the freestanding contract at the assembly-text level:
 *  - the assembly MUST contain the infinite-loop spin (j 1b)
 *  - the assembly MUST NOT contain the Linux-only ecall exit sequence
 *    (li a7, 93 / ecall) that would break any non-Linux target.
 */
void test_cross_run_riscv64_boot_qemu(void) {
    static const char *cc = "riscv64-linux-gnu-gcc";
    static const char source[] = "boot -> 42;\n";
    char *assembly;

    if (!cross_compiler_available(cc)) {
        printf("    [SKIP] %s not found\n", cc);
        return;
    }

    REQUIRE_TRUE(build_assembly_from_source_with_target(
                     source, &assembly,
                     target_get_descriptor(TARGET_KIND_RISCV64_LP64D_ELF)),
                 "emit rv64 boot assembly for freestanding contract check");

    /* Freestanding: _start must spin after the boot unit returns. */
    ASSERT_TRUE(strstr(assembly, "j 1b") != NULL,
                "rv64 freestanding boot entry spins (j 1b) instead of Linux exit");

    /* Linux-only exit must be gone. */
    ASSERT_TRUE(strstr(assembly, "li a7, 93") == NULL,
                "rv64 freestanding boot entry does not emit Linux sys_exit ecall");

    free(assembly);
}
