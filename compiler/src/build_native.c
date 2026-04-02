#define _POSIX_C_SOURCE 200809L

#include "asm_emit.h"
#include "parser.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static char *read_entire_file(const char *path);
static bool executable_directory(char *buffer, size_t buffer_size);
static bool write_temp_assembly(const char *assembly, char *path_buffer, size_t buffer_size);
static int run_linker(const char *assembly_path,
                      const char *runtime_object_path,
                      const char *output_path);
static void print_usage(const char *program_name);
static int build_program_file(const char *source_path, const char *output_path);

int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 64;
    }

    return build_program_file(argv[1], argv[2]);
}

static char *read_entire_file(const char *path) {
    FILE *file;
    char *buffer;
    long size;
    size_t read_size;

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "%s: failed to seek: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0) {
        fprintf(stderr, "%s: failed to size file: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "%s: failed to rewind: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fprintf(stderr, "%s: out of memory while reading file\n", path);
        fclose(file);
        return NULL;
    }
    read_size = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    if (read_size != (size_t)size) {
        fprintf(stderr, "%s: failed to read file contents\n", path);
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

static bool executable_directory(char *buffer, size_t buffer_size) {
    ssize_t written;
    char *slash;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    written = readlink("/proc/self/exe", buffer, buffer_size - 1);
    if (written < 0 || (size_t)written >= buffer_size) {
        return false;
    }
    buffer[written] = '\0';

    slash = strrchr(buffer, '/');
    if (!slash) {
        return false;
    }
    *slash = '\0';
    return true;
}

static bool write_temp_assembly(const char *assembly, char *path_buffer, size_t buffer_size) {
    char template_path[] = "/tmp/calynda-native-XXXXXX";
    FILE *file;
    int fd;

    if (!assembly || !path_buffer || buffer_size == 0) {
        return false;
    }

    fd = mkstemp(template_path);
    if (fd < 0) {
        return false;
    }

    file = fdopen(fd, "wb");
    if (!file) {
        close(fd);
        unlink(template_path);
        return false;
    }
    if (fputs(assembly, file) == EOF || fclose(file) != 0) {
        unlink(template_path);
        return false;
    }

    if (strlen(template_path) + 1 > buffer_size) {
        unlink(template_path);
        return false;
    }

    memcpy(path_buffer, template_path, strlen(template_path) + 1);
    return true;
}

static int run_linker(const char *assembly_path,
                      const char *runtime_object_path,
                      const char *output_path) {
    pid_t child;
    int status;

    child = fork();
    if (child < 0) {
        return -1;
    }

    if (child == 0) {
        execlp("gcc",
               "gcc",
               "-no-pie",
               "-x",
               "assembler",
               assembly_path,
               "-x",
               "none",
               runtime_object_path,
               "-o",
               output_path,
               (char *)NULL);
        _exit(127);
    }

    if (waitpid(child, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
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
    const ParserError *parse_error;
    const SymbolTableError *symbol_error;
    const TypeCheckError *type_error;
    char diagnostic[256];
    int link_exit_code;
    int exit_code = 0;

    source = read_entire_file(source_path);
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
            fprintf(stderr,
                    "%s:%d:%d: parse error: %s\n",
                    source_path,
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
            fprintf(stderr, "%s: semantic error: %s\n", source_path, diagnostic);
        } else {
            fprintf(stderr, "%s: semantic error while building symbols\n", source_path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!type_checker_check_program(&checker, &program, &symbols)) {
        type_error = type_checker_get_error(&checker);
        if (type_error && type_checker_format_error(type_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: type error: %s\n", source_path, diagnostic);
        } else {
            fprintf(stderr, "%s: type error while checking program\n", source_path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!hir_build_program(&hir_program, &program, &symbols, &checker) ||
        !mir_build_program(&mir_program, &hir_program) ||
        !lir_build_program(&lir_program, &mir_program) ||
        !codegen_build_program(&codegen_program, &lir_program) ||
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
    if (!write_temp_assembly(assembly, assembly_path, sizeof(assembly_path))) {
        fprintf(stderr, "%s: failed to create temporary assembly file\n", source_path);
        exit_code = 1;
        goto cleanup;
    }
    if (!executable_directory(executable_dir, sizeof(executable_dir))) {
        strcpy(executable_dir, "build");
    }
    if (snprintf(runtime_object_path,
                 sizeof(runtime_object_path),
                 "%s/runtime.o",
                 executable_dir) < 0) {
        fprintf(stderr, "%s: failed to resolve runtime object path\n", source_path);
        exit_code = 1;
        goto cleanup;
    }

    link_exit_code = run_linker(assembly_path, runtime_object_path, output_path);
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