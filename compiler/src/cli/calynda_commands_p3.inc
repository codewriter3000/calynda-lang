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
                             const TargetDescriptor *target,
                             const CarArchive *archive_deps,
                             size_t archive_dep_count) {
    int exit_code;

    if (mode == CALYNDA_EMIT_MODE_ASM) {
        CarArchive archive;
        MachineProgram machine_program;

        if (has_car_extension(path)) {
            car_archive_init(&archive);
            if (!car_archive_read(&archive, path)) {
                fprintf(stderr, "%s: %s\n", path,
                        car_archive_get_error(&archive));
                car_archive_free(&archive);
                return 66;
            }
            exit_code = calynda_compile_car_to_machine_program(&archive,
                                                               &machine_program,
                                                               target,
                                                               archive_deps,
                                                               archive_dep_count);
            car_archive_free(&archive);
        } else {
            exit_code = calynda_compile_to_machine_program(path,
                                                           &machine_program,
                                                           target,
                                                           archive_deps,
                                                           archive_dep_count);
        }
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
                               const TargetDescriptor *target,
                               const CarArchive *archive_deps,
                               size_t archive_dep_count) {
    MachineProgram machine_program;
    char *assembly;
    char assembly_path[PATH_MAX];
    char executable_dir[PATH_MAX];
    char runtime_object_path[PATH_MAX];
    int link_exit_code;
    int exit_code;
    bool is_boot;

    exit_code = calynda_compile_to_machine_program(source_path, &machine_program,
                                                   target,
                                                   archive_deps,
                                                   archive_dep_count);
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
        const char *runtime_name = calynda_runtime_archive_name(target, is_boot);
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
                             const TargetDescriptor *target,
                             const CarArchive *archive_deps,
                             size_t archive_dep_count) {
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
        ? calynda_build_car_file(source_path, executable_path, target,
                                 archive_deps, archive_dep_count)
        : calynda_build_program_file(source_path, executable_path, target,
                                     archive_deps, archive_dep_count);
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
                           const TargetDescriptor *target,
                           const CarArchive *archive_deps,
                           size_t archive_dep_count) {
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
                                                       target,
                                                       archive_deps,
                                                       archive_dep_count);
    car_archive_free(&archive);
    if (exit_code != 0) {
        return exit_code;
    }

    is_boot = machine_program_has_boot(&machine_program);

#include "calynda_commands_p4.inc"
