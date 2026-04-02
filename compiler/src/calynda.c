#define _POSIX_C_SOURCE 200809L

#include "asm_emit.h"
#include "bytecode.h"
#include "parser.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum {
    CALYNDA_EMIT_MODE_ASM = 0,
    CALYNDA_EMIT_MODE_BYTECODE
} CalyndaEmitMode;

static char *read_entire_file(const char *path);
static bool executable_directory(char *buffer, size_t buffer_size);
static bool write_temp_file(const char *prefix,
                            const char *contents,
                            char *path_buffer,
                            size_t buffer_size);
static int run_linker(const char *assembly_path,
                      const char *runtime_object_path,
                      const char *output_path);
static int run_child_process(const char *path, char *const argv[]);
static bool parse_build_output(int argc,
                               char **argv,
                               const char *default_output,
                               const char **output_path);
static void print_usage(FILE *out, const char *program_name);
static int compile_to_machine_program(const char *path,
                                      MachineProgram *machine_program);
static int compile_to_bytecode_program(const char *path,
                                       BytecodeProgram *bytecode_program);
static int emit_program_file(const char *path, CalyndaEmitMode mode);
static int build_program_file(const char *source_path, const char *output_path);
static int run_program_file(const char *source_path, int argc, char **argv);

int main(int argc, char **argv) {
    const char *program_name = argc > 0 ? argv[0] : "calynda";

    if (argc < 2) {
        print_usage(stderr, program_name);
        return 64;
    }

    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {
        print_usage(stdout, program_name);
        return 0;
    }
    if (strcmp(argv[1], "asm") == 0) {
        if (argc != 3) {
            print_usage(stderr, program_name);
            return 64;
        }
        return emit_program_file(argv[2], CALYNDA_EMIT_MODE_ASM);
    }
    if (strcmp(argv[1], "bytecode") == 0) {
        if (argc != 3) {
            print_usage(stderr, program_name);
            return 64;
        }
        return emit_program_file(argv[2], CALYNDA_EMIT_MODE_BYTECODE);
    }
    if (strcmp(argv[1], "build") == 0) {
        const char *output_path;
        char default_output[PATH_MAX];
        const char *source_path;
        const char *slash;
        const char *basename;
        size_t length;

        if (argc < 3) {
            print_usage(stderr, program_name);
            return 64;
        }

        source_path = argv[2];
        slash = strrchr(source_path, '/');
        basename = slash ? slash + 1 : source_path;
        length = strlen(basename);
        if (length > 4 && strcmp(basename + length - 4, ".cal") == 0) {
            length -= 4;
        }
        if (snprintf(default_output,
                     sizeof(default_output),
                     "build/%.*s",
                     (int)length,
                     basename) < 0) {
            fprintf(stderr, "failed to build default output path\n");
            return 1;
        }
        if (!parse_build_output(argc - 3, argv + 3, default_output, &output_path)) {
            print_usage(stderr, program_name);
            return 64;
        }
        return build_program_file(source_path, output_path);
    }
    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            print_usage(stderr, program_name);
            return 64;
        }
        return run_program_file(argv[2], argc - 3, argv + 3);
    }

    print_usage(stderr, program_name);
    return 64;
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

static bool write_temp_file(const char *prefix,
                            const char *contents,
                            char *path_buffer,
                            size_t buffer_size) {
    char template_path[64];
    FILE *file;
    int fd;

    if (!prefix || !contents || !path_buffer || buffer_size == 0) {
        return false;
    }

    if (snprintf(template_path, sizeof(template_path), "/tmp/%s-XXXXXX", prefix) < 0) {
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
    if (fputs(contents, file) == EOF || fclose(file) != 0) {
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

static int run_child_process(const char *path, char *const argv[]) {
    pid_t child;
    int status;

    child = fork();
    if (child < 0) {
        return -1;
    }

    if (child == 0) {
        execv(path, argv);
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

static bool parse_build_output(int argc,
                               char **argv,
                               const char *default_output,
                               const char **output_path) {
    if (!output_path || !default_output) {
        return false;
    }

    *output_path = default_output;
    if (argc == 0) {
        return true;
    }
    if (argc == 2 && strcmp(argv[0], "-o") == 0) {
        *output_path = argv[1];
        return true;
    }
    return false;
}

static void print_usage(FILE *out, const char *program_name) {
    fprintf(out, "Usage: %s <command> ...\n", program_name);
    fprintf(out, "\nCommands:\n");
    fprintf(out, "  build <source.cal> [-o output]   Build a native executable\n");
    fprintf(out, "  run <source.cal> [args...]       Build a temp executable and run it\n");
    fprintf(out, "  asm <source.cal>                 Emit x86_64 assembly to stdout\n");
    fprintf(out, "  bytecode <source.cal>            Emit portable bytecode text to stdout\n");
    fprintf(out, "  help                             Show this help\n");
}

static int compile_to_machine_program(const char *path,
                                      MachineProgram *machine_program) {
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

    if (!path || !machine_program) {
        return 1;
    }

    source = read_entire_file(path);
    if (!source) {
        return 66;
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
    if (!hir_build_program(&hir_program, &program, &symbols, &checker) ||
        !mir_build_program(&mir_program, &hir_program) ||
        !lir_build_program(&lir_program, &mir_program) ||
        !codegen_build_program(&codegen_program, &lir_program) ||
        !machine_build_program(machine_program, &lir_program, &codegen_program)) {
        fprintf(stderr, "%s: backend lowering failed\n", path);
        exit_code = 1;
        goto cleanup;
    }

cleanup:
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

static int compile_to_bytecode_program(const char *path,
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

    if (!path || !bytecode_program) {
        return 1;
    }

    source = read_entire_file(path);
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
    if (!hir_build_program(&hir_program, &program, &symbols, &checker) ||
        !mir_build_program(&mir_program, &hir_program) ||
        !bytecode_build_program(bytecode_program, &mir_program)) {
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
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
    free(source);
    return exit_code;
}

static int emit_program_file(const char *path, CalyndaEmitMode mode) {
    int exit_code;

    if (mode == CALYNDA_EMIT_MODE_ASM) {
        MachineProgram machine_program;

        exit_code = compile_to_machine_program(path, &machine_program);
        if (exit_code != 0) {
            return exit_code;
        }
        if (!asm_emit_program(stdout, &machine_program)) {
            fprintf(stderr, "%s: failed to write assembly output\n", path);
            exit_code = 1;
        }
        machine_program_free(&machine_program);
        return exit_code;
    }

    {
        BytecodeProgram bytecode_program;

        exit_code = compile_to_bytecode_program(path, &bytecode_program);
        if (exit_code != 0) {
            return exit_code;
        }
        if (!bytecode_dump_program(stdout, &bytecode_program)) {
            fprintf(stderr, "%s: failed to write bytecode output\n", path);
            exit_code = 1;
        }
        bytecode_program_free(&bytecode_program);
    }
    return exit_code;
}

static int build_program_file(const char *source_path, const char *output_path) {
    MachineProgram machine_program;
    char *assembly;
    char assembly_path[PATH_MAX];
    char executable_dir[PATH_MAX];
    char runtime_object_path[PATH_MAX];
    int link_exit_code;
    int exit_code;

    exit_code = compile_to_machine_program(source_path, &machine_program);
    if (exit_code != 0) {
        return exit_code;
    }

    assembly = asm_emit_program_to_string(&machine_program);
    machine_program_free(&machine_program);
    if (!assembly) {
        fprintf(stderr, "%s: failed to render native assembly\n", source_path);
        return 1;
    }
    if (!write_temp_file("calynda-native", assembly, assembly_path, sizeof(assembly_path))) {
        fprintf(stderr, "%s: failed to create temporary assembly file\n", source_path);
        free(assembly);
        return 1;
    }
    if (!executable_directory(executable_dir, sizeof(executable_dir))) {
        strcpy(executable_dir, "build");
    }
    if (snprintf(runtime_object_path,
                 sizeof(runtime_object_path),
                 "%s/runtime.o",
                 executable_dir) < 0) {
        fprintf(stderr, "%s: failed to resolve runtime object path\n", source_path);
        unlink(assembly_path);
        free(assembly);
        return 1;
    }

    link_exit_code = run_linker(assembly_path, runtime_object_path, output_path);
    if (link_exit_code != 0) {
        fprintf(stderr,
                "%s: native link failed (gcc exit %d). Preserved assembly at %s\n",
                source_path,
                link_exit_code,
                assembly_path);
        free(assembly);
        return 1;
    }

    unlink(assembly_path);
    free(assembly);
    return 0;
}

static int run_program_file(const char *source_path, int argc, char **argv) {
    char executable_path[PATH_MAX];
    char template_path[] = "/tmp/calynda-run-XXXXXX";
    char **child_argv;
    int fd;
    int exit_code;
    int i;

    fd = mkstemp(template_path);
    if (fd < 0) {
        fprintf(stderr, "failed to create temporary executable path\n");
        return 1;
    }
    close(fd);
    unlink(template_path);
    memcpy(executable_path, template_path, sizeof(template_path));

    exit_code = build_program_file(source_path, executable_path);
    if (exit_code != 0) {
        unlink(executable_path);
        return exit_code;
    }

    child_argv = calloc((size_t)argc + 2, sizeof(*child_argv));
    if (!child_argv) {
        unlink(executable_path);
        fprintf(stderr, "out of memory while preparing process arguments\n");
        return 1;
    }
    child_argv[0] = executable_path;
    for (i = 0; i < argc; i++) {
        child_argv[i + 1] = argv[i];
    }
    child_argv[argc + 1] = NULL;

    exit_code = run_child_process(executable_path, child_argv);
    free(child_argv);
    unlink(executable_path);
    if (exit_code < 0) {
        fprintf(stderr, "%s: failed to run generated executable\n", source_path);
        return 1;
    }
    return exit_code;
}