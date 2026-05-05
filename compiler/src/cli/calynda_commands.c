#define _POSIX_C_SOURCE 200809L

#include "calynda_internal.h"

#include <dirent.h>
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
static bool calynda_option_add_archive(const char *value,
                                       CalyndaCompileOptions *options,
                                       FILE *err);
static bool calynda_option_add_archive_path(const char *value,
                                            CalyndaCompileOptions *options,
                                            FILE *err);
static bool calynda_parse_compile_options(int *argc, char ***argv,
                                          CalyndaCompileOptions *options,
                                          FILE *err);
static bool calynda_compile_options_append_archive(CalyndaCompileOptions *options,
                                                   const char *path,
                                                   FILE *err);
static bool calynda_load_archive_dependencies(const CalyndaCompileOptions *options,
                                              CarArchive **archives_out,
                                              size_t *archive_count_out,
                                              FILE *err);
static void calynda_free_archive_dependencies(CarArchive *archives,
                                              size_t archive_count);
static const char *calynda_runtime_archive_name(const TargetDescriptor *target,
                                                bool is_boot);
static bool parse_build_output(int argc,
                               char **argv,
                               const char *default_output,
                               const char **output_path);
static int emit_program_file(const char *path, CalyndaEmitMode mode,
                             const TargetDescriptor *target,
                             const CarArchive *archive_deps,
                             size_t archive_dep_count);

static const CalyndaOptionSpec k_compile_option_specs[] = {
    { "--manual-bounds-check", false, calynda_option_enable_manual_bounds },
    { "--strict-race-check", false, calynda_option_enable_strict_race },
    { "--target", true, calynda_option_set_target },
    { "--archive", true, calynda_option_add_archive },
    { "--archive-path", true, calynda_option_add_archive_path }
};

static const char *calynda_runtime_archive_name(const TargetDescriptor *target,
                                                bool is_boot) {
    if (is_boot) {
        if (target->kind == TARGET_KIND_AARCH64_AAPCS_ELF) {
            return "calynda_runtime_boot_aarch64.a";
        }
        if (target->kind == TARGET_KIND_RISCV64_LP64D_ELF) {
            return "calynda_runtime_boot_riscv64.a";
        }
        return "calynda_runtime_boot.a";
    }

    if (target->kind == TARGET_KIND_AARCH64_AAPCS_ELF) {
        return "calynda_runtime_aarch64.a";
    }
    if (target->kind == TARGET_KIND_RISCV64_LP64D_ELF) {
        return "calynda_runtime_riscv64.a";
    }
    return "calynda_runtime.a";
}

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
    fprintf(out, "  asm [compiler options] <source>                 Emit assembly to stdout\n");
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
    fprintf(out, "  --archive path.car                              Add a dependency CAR archive (repeatable)\n");
    fprintf(out, "  --archive-path dir                              Add all .car files from a dependency directory\n");
    fprintf(out, "\nBuild, run, and asm accept .cal (single file) or .car (multi-file archive).\n");
    fprintf(out, "bytecode currently accepts .cal only.\n");
}

int calynda_command_asm(const char *program_name, int argc, char **argv) {
    CalyndaCompileOptions options;
    CarArchive *archive_deps = NULL;
    size_t archive_dep_count = 0;
    int exit_code;

    calynda_compile_options_init(&options);
    if (!calynda_parse_compile_options(&argc, &argv, &options, stderr)) {
        calynda_print_usage(stderr, program_name);
        calynda_compile_options_free(&options);
        return 64;
    }
    if (argc != 1) {
        calynda_print_usage(stderr, program_name);
        calynda_compile_options_free(&options);
        return 64;
    }
    if (!calynda_load_archive_dependencies(&options, &archive_deps,
                                           &archive_dep_count, stderr)) {
        calynda_compile_options_free(&options);
        return 66;
    }

    calynda_apply_compile_options(&options);
    exit_code = emit_program_file(argv[0], CALYNDA_EMIT_MODE_ASM, options.target,
                                  archive_deps, archive_dep_count);
    calynda_free_archive_dependencies(archive_deps, archive_dep_count);
    calynda_compile_options_free(&options);
    return exit_code;
}

int calynda_command_bytecode(const char *program_name, int argc, char **argv) {
    if (argc != 1) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }

    calynda_apply_compile_options(NULL);
    return emit_program_file(argv[0], CALYNDA_EMIT_MODE_BYTECODE,
                             target_get_default(), NULL, 0);
}

int calynda_command_build(const char *program_name, int argc, char **argv) {
    CalyndaCompileOptions options;
    CarArchive *archive_deps = NULL;
    size_t archive_dep_count = 0;
    const char *output_path;
    char default_output[PATH_MAX];
    const char *source_path;
    const char *slash;
    const char *basename;
    size_t length;

    calynda_compile_options_init(&options);
    if (!calynda_parse_compile_options(&argc, &argv, &options, stderr)) {
        calynda_print_usage(stderr, program_name);
        calynda_compile_options_free(&options);
        return 64;
    }
    if (argc < 1) {
        calynda_print_usage(stderr, program_name);
        calynda_compile_options_free(&options);
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
        calynda_compile_options_free(&options);
        return 1;
    }
    if (!parse_build_output(argc - 1, argv + 1, default_output, &output_path)) {
        calynda_print_usage(stderr, program_name);
        calynda_compile_options_free(&options);
        return 64;
    }
    if (!calynda_load_archive_dependencies(&options, &archive_deps,
                                           &archive_dep_count, stderr)) {
        calynda_compile_options_free(&options);
        return 66;
    }

    calynda_apply_compile_options(&options);
    if (has_car_extension(source_path)) {
        int exit_code = calynda_build_car_file(source_path, output_path,
                                               options.target,
                                               archive_deps, archive_dep_count);
        calynda_free_archive_dependencies(archive_deps, archive_dep_count);
        calynda_compile_options_free(&options);
        return exit_code;
    }
    {
        int exit_code = calynda_build_program_file(source_path, output_path,
                                                   options.target,
                                                   archive_deps, archive_dep_count);
        calynda_free_archive_dependencies(archive_deps, archive_dep_count);
        calynda_compile_options_free(&options);
        return exit_code;
    }
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
#include "calynda_commands_p2.inc"
