#define _POSIX_C_SOURCE 200809L

#include "calynda_internal.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef bool (*CalyndaOptionApplyFn)(const char *value,
                                     CalyndaCompileOptions *options,
                                     FILE *err);

typedef struct {
    const char *name;
    bool takes_value;
    CalyndaOptionApplyFn apply;
} CalyndaOptionSpec;

static bool calynda_option_enable_manual_bounds(const char *value,
                                                CalyndaCompileOptions *options,
                                                FILE *err);
static bool calynda_option_enable_strict_race(const char *value,
                                              CalyndaCompileOptions *options,
                                              FILE *err);
static bool calynda_option_set_target(const char *value,
                                      CalyndaCompileOptions *options,
                                      FILE *err);
static bool calynda_parse_compile_options(int *argc, char ***argv,
                                          CalyndaCompileOptions *options,
                                          FILE *err);
static bool parse_build_output(int argc,
                               char **argv,
                               const char *default_output,
                               const char **output_path);
static int emit_program_file(const char *path, CalyndaEmitMode mode,
                             const TargetDescriptor *target);

static const CalyndaOptionSpec k_compile_option_specs[] = {
    { "--manual-bounds-check", false, calynda_option_enable_manual_bounds },
    { "--strict-race-check", false, calynda_option_enable_strict_race },
    { "--target", true, calynda_option_set_target }
};

void calynda_print_version(FILE *out) {
    fprintf(out, "%s\n", calynda_cli_version_line());
}

void calynda_print_usage(FILE *out, const char *program_name) {
    fprintf(out, "Usage: %s [--help] [--version] <command> [args...]\n",
            program_name);
    fprintf(out, "\nCommands:\n");
    fprintf(out, "  build [compiler options] <source> [-o output]   Build a native executable\n");
    fprintf(out, "  run [compiler options] <source> [args...]       Build a temp executable and run it\n");
    fprintf(out, "  pack <directory> [-o output.car]                Pack a directory into a .car archive\n");
    fprintf(out, "  asm [compiler options] <source.cal>             Emit assembly to stdout\n");
    fprintf(out, "  bytecode <source.cal>                           Emit portable bytecode text to stdout\n");
    fprintf(out, "  help                                            Show this help\n");
    fprintf(out, "  version                                         Show version metadata\n");
    fprintf(out, "\nGlobal options:\n");
    fprintf(out, "  --help, -h                                      Show this help\n");
    fprintf(out, "  --version                                       Print version metadata\n");
    fprintf(out, "\nCompiler options:\n");
    fprintf(out, "  --strict-race-check                             Enable the reserved alpha.2 strict race mode\n");
    fprintf(out, "  --manual-bounds-check                           Force manual memory ops through checked helpers\n");
    fprintf(out, "  --target T                                      Target x86_64, aarch64, or riscv64\n");
    fprintf(out, "\nSource files can be .cal (single file) or .car (multi-file archive).\n");
}

int calynda_command_asm(const char *program_name, int argc, char **argv) {
    CalyndaCompileOptions options;

    calynda_compile_options_init(&options);
    if (!calynda_parse_compile_options(&argc, &argv, &options, stderr)) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }
    if (argc != 1) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }

    calynda_apply_compile_options(&options);
    return emit_program_file(argv[0], CALYNDA_EMIT_MODE_ASM, options.target);
}

int calynda_command_bytecode(const char *program_name, int argc, char **argv) {
    if (argc != 1) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }

    calynda_apply_compile_options(NULL);
    return emit_program_file(argv[0], CALYNDA_EMIT_MODE_BYTECODE,
                             target_get_default());
}

int calynda_command_build(const char *program_name, int argc, char **argv) {
    CalyndaCompileOptions options;
    const char *output_path;
    char default_output[PATH_MAX];
    const char *source_path;
    const char *slash;
    const char *basename;
    size_t length;

    calynda_compile_options_init(&options);
    if (!calynda_parse_compile_options(&argc, &argv, &options, stderr)) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }
    if (argc < 1) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }

    source_path = argv[0];
    slash = strrchr(source_path, '/');
    basename = slash ? slash + 1 : source_path;
    length = strlen(basename);
    if ((length > 4 && strcmp(basename + length - 4, ".cal") == 0) ||
        (length > 4 && strcmp(basename + length - 4, ".car") == 0)) {
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
    if (!parse_build_output(argc - 1, argv + 1, default_output, &output_path)) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }

    calynda_apply_compile_options(&options);
    if (has_car_extension(source_path)) {
        return calynda_build_car_file(source_path, output_path, options.target);
    }
    return calynda_build_program_file(source_path, output_path, options.target);
}

int calynda_command_pack(const char *program_name, int argc, char **argv) {
    const char *output_path;
    const char *dir_path;

    if (argc < 1) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }

    dir_path = argv[0];
    if (argc >= 3 && strcmp(argv[1], "-o") == 0) {
        output_path = argv[2];
    } else if (argc == 1) {
        output_path = "project.car";
    } else {
        calynda_print_usage(stderr, program_name);
        return 64;
    }
    return calynda_pack_directory(dir_path, output_path);
}

int calynda_command_run(const char *program_name, int argc, char **argv) {
    CalyndaCompileOptions options;

    calynda_compile_options_init(&options);
    if (!calynda_parse_compile_options(&argc, &argv, &options, stderr)) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }
    if (argc < 1) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }

    calynda_apply_compile_options(&options);
    return calynda_run_program_file(argv[0], argc - 1, argv + 1, options.target);
}

static bool calynda_option_enable_manual_bounds(const char *value,
                                                CalyndaCompileOptions *options,
                                                FILE *err) {
    (void)value;
    (void)err;
    options->manual_bounds_check = true;
    return true;
}

static bool calynda_option_enable_strict_race(const char *value,
                                              CalyndaCompileOptions *options,
                                              FILE *err) {
    (void)value;
    (void)err;
    options->strict_race_check = true;
    return true;
}

static bool calynda_option_set_target(const char *value,
                                      CalyndaCompileOptions *options,
                                      FILE *err) {
    bool found = false;
    TargetKind kind = target_kind_from_name(value, &found);

    if (!found) {
        fprintf(err, "unknown target: %s\n", value);
        return false;
    }
    options->target = target_get_descriptor(kind);
    return true;
}

static bool calynda_parse_compile_options(int *argc, char ***argv,
                                          CalyndaCompileOptions *options,
                                          FILE *err) {
    while (*argc > 0) {
        const char *current = (*argv)[0];
        size_t i;
        bool matched = false;

        if (!current || current[0] != '-') {
            break;
        }

        for (i = 0; i < sizeof(k_compile_option_specs) / sizeof(k_compile_option_specs[0]); i++) {
            const CalyndaOptionSpec *spec = &k_compile_option_specs[i];
            const char *value = NULL;
            int consumed = 1;

            if (strcmp(current, spec->name) != 0) {
                continue;
            }
            if (spec->takes_value) {
                if (*argc < 2) {
                    fprintf(err, "missing value for option: %s\n", current);
                    return false;
                }
                value = (*argv)[1];
                consumed = 2;
            }
            if (!spec->apply(value, options, err)) {
                return false;
            }
            *argc -= consumed;
            *argv += consumed;
            matched = true;
            break;
        }

        if (!matched) {
            fprintf(err, "unknown option: %s\n", current);
            return false;
        }
    }
    return true;
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

static int emit_program_file(const char *path, CalyndaEmitMode mode,
                             const TargetDescriptor *target) {
    int exit_code;

    if (mode == CALYNDA_EMIT_MODE_ASM) {
        MachineProgram machine_program;

        exit_code = calynda_compile_to_machine_program(path, &machine_program,
                                                       target);
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

        exit_code = calynda_compile_to_bytecode_program(path, &bytecode_program);
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

int calynda_build_program_file(const char *source_path, const char *output_path,
                               const TargetDescriptor *target) {
    MachineProgram machine_program;
    char *assembly;
    char assembly_path[PATH_MAX];
    char executable_dir[PATH_MAX];
    char runtime_object_path[PATH_MAX];
    int link_exit_code;
    int exit_code;
    bool is_boot;

    exit_code = calynda_compile_to_machine_program(source_path, &machine_program,
                                                   target);
    if (exit_code != 0) {
        return exit_code;
    }

    is_boot = machine_program_has_boot(&machine_program);

    assembly = asm_emit_program_to_string(&machine_program);
    machine_program_free(&machine_program);
    if (!assembly) {
        fprintf(stderr, "%s: failed to render native assembly\n", source_path);
        return 1;
    }
    if (!calynda_write_temp_file("calynda-native", assembly, assembly_path, sizeof(assembly_path))) {
        fprintf(stderr, "%s: failed to create temporary assembly file\n", source_path);
        free(assembly);
        return 1;
    }
    if (!calynda_executable_directory(executable_dir, sizeof(executable_dir))) {
        strcpy(executable_dir, "build");
    }
    {
        const char *runtime_name = "calynda_runtime.a";
        if (target->kind == TARGET_KIND_AARCH64_AAPCS_ELF)
            runtime_name = "calynda_runtime_aarch64.a";
        else if (target->kind == TARGET_KIND_RISCV64_LP64D_ELF)
            runtime_name = "calynda_runtime_riscv64.a";
        if (snprintf(runtime_object_path,
                     sizeof(runtime_object_path),
                     "%s/%s",
                     executable_dir,
                     runtime_name) < 0) {
            fprintf(stderr, "%s: failed to resolve runtime object path\n", source_path);
            unlink(assembly_path);
            free(assembly);
            return 1;
        }
    }

    link_exit_code = calynda_run_linker(assembly_path, runtime_object_path,
                                         output_path, target, is_boot);
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

int calynda_run_program_file(const char *source_path, int argc, char **argv,
                             const TargetDescriptor *target) {
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

    exit_code = has_car_extension(source_path)
        ? calynda_build_car_file(source_path, executable_path, target)
        : calynda_build_program_file(source_path, executable_path, target);
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

    exit_code = calynda_run_child_process(executable_path, child_argv);
    free(child_argv);
    unlink(executable_path);
    if (exit_code < 0) {
        fprintf(stderr, "%s: failed to run generated executable\n", source_path);
        return 1;
    }
    return exit_code;
}

int calynda_build_car_file(const char *car_path, const char *output_path,
                           const TargetDescriptor *target) {
    CarArchive archive;
    MachineProgram machine_program;
    char *assembly;
    char assembly_path[PATH_MAX];
    char executable_dir[PATH_MAX];
    char runtime_object_path[PATH_MAX];
    int link_exit_code;
    int exit_code;
    bool is_boot;

    car_archive_init(&archive);
    if (!car_archive_read(&archive, car_path)) {
        fprintf(stderr, "%s: %s\n", car_path,
                car_archive_get_error(&archive));
        car_archive_free(&archive);
        return 66;
    }

    exit_code = calynda_compile_car_to_machine_program(&archive,
                                                       &machine_program,
                                                       target);
    car_archive_free(&archive);
    if (exit_code != 0) {
        return exit_code;
    }

    is_boot = machine_program_has_boot(&machine_program);

    assembly = asm_emit_program_to_string(&machine_program);
    machine_program_free(&machine_program);
    if (!assembly) {
        fprintf(stderr, "%s: failed to render native assembly\n", car_path);
        return 1;
    }
    if (!calynda_write_temp_file("calynda-native", assembly, assembly_path,
                                 sizeof(assembly_path))) {
        fprintf(stderr, "%s: failed to create temporary assembly file\n",
                car_path);
        free(assembly);
        return 1;
    }
    if (!calynda_executable_directory(executable_dir,
                                      sizeof(executable_dir))) {
        strcpy(executable_dir, "build");
    }
    {
        const char *runtime_name = "calynda_runtime.a";
        if (target->kind == TARGET_KIND_AARCH64_AAPCS_ELF)
            runtime_name = "calynda_runtime_aarch64.a";
        else if (target->kind == TARGET_KIND_RISCV64_LP64D_ELF)
            runtime_name = "calynda_runtime_riscv64.a";
        if (snprintf(runtime_object_path,
                     sizeof(runtime_object_path),
                     "%s/%s",
                     executable_dir,
                     runtime_name) < 0) {
            fprintf(stderr, "%s: failed to resolve runtime object path\n",
                    car_path);
            unlink(assembly_path);
            free(assembly);
            return 1;
        }
    }

    link_exit_code = calynda_run_linker(assembly_path, runtime_object_path,
                                        output_path, target, is_boot);
    if (link_exit_code != 0) {
        fprintf(stderr,
                "%s: native link failed (gcc exit %d). Preserved assembly at %s\n",
                car_path, link_exit_code, assembly_path);
        free(assembly);
        return 1;
    }

    unlink(assembly_path);
    free(assembly);
    return 0;
}

int calynda_pack_directory(const char *dir_path, const char *output_path) {
    CarArchive archive;

    car_archive_init(&archive);
    if (!car_archive_add_directory(&archive, dir_path)) {
        fprintf(stderr, "pack: %s\n", car_archive_get_error(&archive));
        car_archive_free(&archive);
        return 1;
    }

    if (archive.file_count == 0) {
        fprintf(stderr, "pack: no .cal files found in %s\n", dir_path);
        car_archive_free(&archive);
        return 1;
    }

    if (!car_archive_write(&archive, output_path)) {
        fprintf(stderr, "pack: failed to write %s\n", output_path);
        car_archive_free(&archive);
        return 1;
    }

    printf("packed %zu file(s) into %s\n", archive.file_count, output_path);
    car_archive_free(&archive);
    return 0;
}
