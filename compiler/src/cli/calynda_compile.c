#include "calynda_internal.h"
#include "target.h"
#include "type_checker.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static bool g_global_bounds_check = false;
static bool g_global_strict_race_check = false;

void calynda_compile_options_init(CalyndaCompileOptions *options) {
    if (!options) {
        return;
    }
    options->manual_bounds_check = false;
    options->strict_race_check = false;
    options->target = target_get_default();
    options->archive_paths = NULL;
    options->archive_count = 0;
    options->archive_capacity = 0;
}

void calynda_compile_options_free(CalyndaCompileOptions *options) {
    size_t i;

    if (!options) {
        return;
    }

    for (i = 0; i < options->archive_count; i++) {
        free(options->archive_paths[i]);
    }
    free(options->archive_paths);
    options->archive_paths = NULL;
    options->archive_count = 0;
    options->archive_capacity = 0;
}

void calynda_apply_compile_options(const CalyndaCompileOptions *options) {
    if (!options) {
        calynda_set_global_bounds_check(false);
        calynda_set_global_strict_race_check(false);
        return;
    }
    calynda_set_global_bounds_check(options->manual_bounds_check);
    calynda_set_global_strict_race_check(options->strict_race_check);
}

void calynda_set_global_bounds_check(bool enabled) {
    g_global_bounds_check = enabled;
}

void calynda_set_global_strict_race_check(bool enabled) {
    g_global_strict_race_check = enabled;
}

int calynda_compile_to_machine_program(const char *path,
                                       MachineProgram *machine_program,
                                       const TargetDescriptor *target,
                                       const CarArchive *archive_deps,
                                       size_t archive_dep_count) {
    char *source;
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    CodegenProgram codegen_program;
    const ParserError *parse_error;
    const SymbolTableError *symbol_error;
    const TypeCheckError *type_error;
    char diagnostic[256];
    int exit_code = 0;
    bool strict_race_check_enabled = g_global_strict_race_check;

    (void)strict_race_check_enabled;

    if (!path || !machine_program) {
        return 1;
    }

    source = calynda_read_entire_file(path);
    if (!source) {
        return 66;
    }

    if (archive_dep_count > 0) {
        CarArchive source_archive;

        car_archive_init(&source_archive);
        if (!car_archive_add_file(&source_archive, path, source, strlen(source))) {
            fprintf(stderr, "%s: %s\n", path, car_archive_get_error(&source_archive));
            car_archive_free(&source_archive);
            free(source);
            return 1;
        }
        {
            int exit_code = calynda_compile_car_to_machine_program(&source_archive,
                                                                   machine_program,
                                                                   target,
                                                                   archive_deps,
                                                                   archive_dep_count);
            car_archive_free(&source_archive);
            free(source);
            return exit_code;
        }
    }

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    machine_program_init(machine_program);
    parser_init(&parser, source);

    if (!parser_parse_program(&parser, &program)) {
        parse_error = parser_get_error(&parser);
        if (parse_error) {
            fprintf(stderr,
                    "%s:%d:%d: parse error: %s\n",
                    path,
                    parse_error->token.line,
                    parse_error->token.column,
                    parse_error->message);
        }
        parser_free(&parser);
        free(source);
        return 1;
    }

    if (!symbol_table_build_with_archive_deps(&symbols, &program,
                                              archive_deps,
                                              archive_dep_count)) {
        symbol_error = symbol_table_get_error(&symbols);
        if (symbol_error && symbol_table_format_error(symbol_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: semantic error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: semantic error while building symbols\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    type_checker_set_global_strict_race_check(g_global_strict_race_check);
    if (!type_checker_check_program(&checker, &program, &symbols)) {
        type_error = type_checker_get_error(&checker);
        if (type_error && type_checker_format_error(type_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: type error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: type error while checking program\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!hir_build_program(&hir_program, &program, &symbols, &checker)) {
        const HirBuildError *hir_error = hir_get_error(&hir_program);

        if (hir_error && hir_format_error(hir_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: HIR error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: HIR lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!mir_build_program(&mir_program, &hir_program, g_global_bounds_check)) {
        const MirBuildError *mir_error = mir_get_error(&mir_program);

        if (mir_error && mir_format_error(mir_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: MIR error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: MIR lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!lir_build_program(&lir_program, &mir_program)) {
        const LirBuildError *lir_error = lir_get_error(&lir_program);

        if (lir_error && lir_format_error(lir_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: LIR error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: LIR lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!codegen_build_program(&codegen_program, &lir_program, target)) {
        const CodegenBuildError *codegen_error = codegen_get_error(&codegen_program);

        if (codegen_error && codegen_format_error(codegen_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: codegen error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: codegen lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!machine_build_program(machine_program, &lir_program, &codegen_program)) {
        const MachineBuildError *machine_error = machine_get_error(machine_program);

        if (machine_error && machine_format_error(machine_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: machine error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: backend lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }

cleanup:
    type_checker_set_global_strict_race_check(false);
    codegen_program_free(&codegen_program);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
    free(source);
    return exit_code;
}

int calynda_compile_to_bytecode_program(const char *path,
                                        BytecodeProgram *bytecode_program) {
    char *source;
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    const ParserError *parse_error;
    const SymbolTableError *symbol_error;
    const TypeCheckError *type_error;
    const BytecodeBuildError *bytecode_error;
    char diagnostic[256];
    int exit_code = 0;
    bool strict_race_check_enabled = g_global_strict_race_check;

    (void)strict_race_check_enabled;

    if (!path || !bytecode_program) {
        return 1;
    }

    source = calynda_read_entire_file(path);
    if (!source) {
        return 66;
    }

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    bytecode_program_init(bytecode_program);
    parser_init(&parser, source);

    if (!parser_parse_program(&parser, &program)) {
        parse_error = parser_get_error(&parser);
        if (parse_error) {
            fprintf(stderr,
                    "%s:%d:%d: parse error: %s\n",
                    path,
                    parse_error->token.line,
                    parse_error->token.column,
                    parse_error->message);
        }
        parser_free(&parser);
        free(source);
        return 1;
    }

    if (!symbol_table_build(&symbols, &program)) {
        symbol_error = symbol_table_get_error(&symbols);
        if (symbol_error && symbol_table_format_error(symbol_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: semantic error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: semantic error while building symbols\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    type_checker_set_global_strict_race_check(g_global_strict_race_check);
    if (!type_checker_check_program(&checker, &program, &symbols)) {
        type_error = type_checker_get_error(&checker);
        if (type_error && type_checker_format_error(type_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: type error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: type error while checking program\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!hir_build_program(&hir_program, &program, &symbols, &checker)) {
        const HirBuildError *hir_error = hir_get_error(&hir_program);

        if (hir_error && hir_format_error(hir_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: HIR error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: HIR lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!mir_build_program(&mir_program, &hir_program, g_global_bounds_check)) {
        const MirBuildError *mir_error = mir_get_error(&mir_program);

        if (mir_error && mir_format_error(mir_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: MIR error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: MIR lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!bytecode_build_program(bytecode_program, &mir_program)) {
        bytecode_error = bytecode_get_error(bytecode_program);
        if (bytecode_error && bytecode_format_error(bytecode_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: bytecode error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: bytecode lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }

cleanup:
    type_checker_set_global_strict_race_check(false);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
    free(source);
    return exit_code;
}
