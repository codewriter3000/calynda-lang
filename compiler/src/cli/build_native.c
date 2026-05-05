#define _POSIX_C_SOURCE 200809L

#include "asm_emit.h"
#include "machine.h"
#include "parser.h"
#include "target.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *build_native_read_entire_file(const char *path);
bool build_native_executable_directory(char *buffer, size_t buffer_size);
bool build_native_write_temp_assembly(const char *assembly, char *path_buffer, size_t buffer_size);
int build_native_run_linker(const char *assembly_path,
                            const char *runtime_object_path,
                            const char *output_path,
                            bool is_boot);
void build_native_print_diagnostic(const char *path, const char *source,
                                    int line, int col_start, int col_end,
                                    const char *kind, const char *message);
static void print_usage(const char *program_name);
static int build_program_file(const char *source_path, const char *output_path);

int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 64;
    }

    return build_program_file(argv[1], argv[2]);
}

static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <source-file> <output-executable>\n", program_name);
    fprintf(stderr, "  Builds a native executable by emitting assembly and linking it with the runtime.\n");
}

static int build_program_file(const char *source_path, const char *output_path) {
    char *source;
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    CodegenProgram codegen_program;
    MachineProgram machine_program;
    char *assembly = NULL;
    char assembly_path[PATH_MAX];
    char executable_dir[PATH_MAX];
    char runtime_object_path[PATH_MAX];
    bool is_boot;
    const ParserError *parse_error;
    const SymbolTableError *symbol_error;
    const TypeCheckError *type_error;
    int link_exit_code;
    int exit_code = 0;

    source = build_native_read_entire_file(source_path);
    if (!source) {
        return 66;
    }

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    machine_program_init(&machine_program);
    parser_init(&parser, source);

    if (!parser_parse_program(&parser, &program)) {
        parse_error = parser_get_error(&parser);
        if (parse_error) {
            int col_end = parse_error->token.column +
                          (int)(parse_error->token.length > 0
                                ? parse_error->token.length - 1 : 0);
            build_native_print_diagnostic(source_path, source,
                                          parse_error->token.line,
                                          parse_error->token.column, col_end,
                                          "parse error", parse_error->message);
        }
        parser_free(&parser);
        free(source);
        return 1;
    }

    if (!symbol_table_build(&symbols, &program)) {
        symbol_error = symbol_table_get_error(&symbols);
        if (symbol_error) {
            build_native_print_diagnostic(source_path, source,
                                          symbol_error->primary_span.start_line,
                                          symbol_error->primary_span.start_column,
                                          symbol_error->primary_span.end_column,
                                          "semantic error", symbol_error->message);
            if (symbol_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        symbol_error->related_span.start_line,
                        symbol_error->related_span.start_column);
        } else {
            fprintf(stderr, "%s: semantic error while building symbols\n", source_path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!type_checker_check_program(&checker, &program, &symbols)) {
        type_error = type_checker_get_error(&checker);
        if (type_error) {
            build_native_print_diagnostic(source_path, source,
                                          type_error->primary_span.start_line,
                                          type_error->primary_span.start_column,
                                          type_error->primary_span.end_column,
                                          "type error", type_error->message);
            if (type_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        type_error->related_span.start_line,
                        type_error->related_span.start_column);
        } else {
            fprintf(stderr, "%s: type error while checking program\n", source_path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!hir_build_program(&hir_program, &program, &symbols, &checker) ||
        !mir_build_program(&mir_program, &hir_program, false) ||
        !lir_build_program(&lir_program, &mir_program) ||
        !codegen_build_program(&codegen_program, &lir_program, target_get_default()) ||
        !machine_build_program(&machine_program, &lir_program, &codegen_program)) {
        fprintf(stderr, "%s: backend lowering failed\n", source_path);
        exit_code = 1;
        goto cleanup;
    }

    assembly = asm_emit_program_to_string(&machine_program);
    if (!assembly) {
        fprintf(stderr, "%s: failed to render native assembly\n", source_path);
        exit_code = 1;
        goto cleanup;
    }
    if (!build_native_write_temp_assembly(assembly, assembly_path, sizeof(assembly_path))) {
        fprintf(stderr, "%s: failed to create temporary assembly file\n", source_path);
        exit_code = 1;
        goto cleanup;
    }
    if (!build_native_executable_directory(executable_dir, sizeof(executable_dir))) {
        strcpy(executable_dir, "build");
    }
    is_boot = machine_program_has_boot(&machine_program);
    if (snprintf(runtime_object_path,
                 sizeof(runtime_object_path),
                 "%s/%s",
                 executable_dir,
                 is_boot ? "calynda_runtime_boot.a" : "calynda_runtime.a") < 0) {
        fprintf(stderr, "%s: failed to resolve runtime object path\n", source_path);
        exit_code = 1;
        goto cleanup;
    }

    link_exit_code = build_native_run_linker(assembly_path, runtime_object_path,
                                              output_path,
                                              is_boot);
    if (link_exit_code != 0) {
        fprintf(stderr,
                "%s: native link failed (gcc exit %d). Preserved assembly at %s\n",
                source_path,
                link_exit_code,
                assembly_path);
        exit_code = 1;
        goto cleanup;
    }
    unlink(assembly_path);

cleanup:
    free(assembly);
    machine_program_free(&machine_program);
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