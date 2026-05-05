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

static bool type_check_source_fails(const char *source,
                                    char *diagnostic,
                                    size_t diagnostic_size) {
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    bool ok = false;

    memset(&ast_program, 0, sizeof(ast_program));
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);

    if (parser_parse_program(&parser, &ast_program) &&
        symbol_table_build(&symbols, &ast_program) &&
        !type_checker_check_program(&checker, &ast_program, &symbols) &&
        type_checker_get_error(&checker) &&
        type_checker_format_error(type_checker_get_error(&checker),
                                  diagnostic,
                                  diagnostic_size)) {
        ok = true;
    }

    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
    return ok;
}

static bool hir_build_source_fails(const char *source,
                                   char *diagnostic,
                                   size_t diagnostic_size) {
    Parser parser;
    AstProgram ast_program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    bool ok = false;

    memset(&ast_program, 0, sizeof(ast_program));
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    parser_init(&parser, source);

    if (parser_parse_program(&parser, &ast_program) &&
        symbol_table_build(&symbols, &ast_program) &&
        type_checker_check_program(&checker, &ast_program, &symbols) &&
        !hir_build_program(&hir_program, &ast_program, &symbols, &checker) &&
        hir_get_error(&hir_program) &&
        hir_format_error(hir_get_error(&hir_program),
                         diagnostic,
                         diagnostic_size)) {
        ok = true;
    }

    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&ast_program);
    parser_free(&parser);
    return ok;
}


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
    static const char source[] = "boot -> 42;\n";
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
    static const char source[] = "boot -> 42;\n";
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

void test_asm_emit_boot_rejects_unknown_imported_member(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "boot -> {\n"
        "    stdlib.missing();\n"
        "    return 0;\n"
        "};\n";
    char diagnostic[256];

    REQUIRE_TRUE(type_check_source_fails(source, diagnostic, sizeof(diagnostic)),
                 "type check invalid freestanding imported member");
    ASSERT_CONTAINS("Freestanding boot() code cannot lower imported member 'stdlib.missing' from 'io.stdlib' statically.",
                    diagnostic,
                    "boot rejects unsupported imported members before hosted runtime lowering");
}

void test_asm_emit_boot_rejects_multi_arg_imported_print(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "boot -> {\n"
        "    stdlib.print(1, 2);\n"
        "    return 0;\n"
        "};\n";
    char diagnostic[256];

    REQUIRE_TRUE(hir_build_source_fails(source, diagnostic, sizeof(diagnostic)),
                 "lower invalid freestanding imported print call");
    ASSERT_CONTAINS("Freestanding boot() code supports io.stdlib.print with 0 or 1 argument, but got 2.",
                    diagnostic,
                    "boot rejects imported print arities that cannot be lowered statically");
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

#include "test_asm_emit_3_p2.inc"
