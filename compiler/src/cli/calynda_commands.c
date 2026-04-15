#define _POSIX_C_SOURCE 200809L

#include "calynda_internal.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
        const char *runtime_name =
            target->kind == TARGET_KIND_AARCH64_AAPCS_ELF
                ? "calynda_runtime_aarch64.a"
                : "calynda_runtime.a";
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
        const char *runtime_name =
            target->kind == TARGET_KIND_AARCH64_AAPCS_ELF
                ? "calynda_runtime_aarch64.a"
                : "calynda_runtime.a";
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