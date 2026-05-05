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
        const char *runtime_name = calynda_runtime_archive_name(target, is_boot);
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
