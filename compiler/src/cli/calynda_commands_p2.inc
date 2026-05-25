
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
    CarArchive *archive_deps = NULL;
    size_t archive_dep_count = 0;
    int exit_code;

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
    if (!calynda_load_archive_dependencies(&options, &archive_deps,
                                           &archive_dep_count, stderr)) {
        calynda_compile_options_free(&options);
        return 66;
    }

    calynda_apply_compile_options(&options);
    exit_code = calynda_run_program_file(argv[0], argc - 1, argv + 1,
                                         options.target,
                                         archive_deps, archive_dep_count);
    calynda_free_archive_dependencies(archive_deps, archive_dep_count);
    calynda_compile_options_free(&options);
    return exit_code;
}

static bool calynda_option_set_gc(const char *value,
                                  CalyndaCompileOptions *options,
                                  FILE *err) {
    if (strcmp(value, "marksweep") == 0) {
        options->gc_mode = CALYNDA_GC_MARKSWEEP;
        return true;
    }
    if (strcmp(value, "legacy") == 0) {
        options->gc_mode = CALYNDA_GC_LEGACY;
        return true;
    }
    fprintf(err, "unknown GC mode: %s (expected 'marksweep' or 'legacy')\n", value);
    return false;
}

static bool calynda_option_set_gc_plugin(const char *value,
                                         CalyndaCompileOptions *options,
                                         FILE *err) {
    (void)err;
    options->gc_plugin_path = value;
    return true;
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

static bool calynda_option_disable_performance_warnings(const char *value,
                                                        CalyndaCompileOptions *options,
                                                        FILE *err) {
    (void)value;
    (void)err;
    options->performance_warnings = false;
    return true;
}

static bool calynda_option_enable_performance_advisories(const char *value,
                                                         CalyndaCompileOptions *options,
                                                         FILE *err) {
    (void)value;
    (void)err;
    options->performance_advisories = true;
    return true;
}

static bool calynda_option_enable_size_focus(const char *value,
                                             CalyndaCompileOptions *options,
                                             FILE *err) {
    (void)value;
    (void)err;
    options->size_focus = true;
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

static bool calynda_compile_options_append_archive(CalyndaCompileOptions *options,
                                                   const char *path,
                                                   FILE *err) {
    char **resized;
    size_t new_capacity;
    char *copy;

    if (!options || !path) {
        return false;
    }

    copy = strdup(path);
    if (!copy) {
        fprintf(err, "out of memory while recording archive dependency\n");
        return false;
    }
    if (options->archive_count < options->archive_capacity) {
        options->archive_paths[options->archive_count++] = copy;
        return true;
    }

    new_capacity = options->archive_capacity == 0 ? 4 : options->archive_capacity * 2;
    resized = realloc(options->archive_paths, new_capacity * sizeof(*resized));
    if (!resized) {
        free(copy);
        fprintf(err, "out of memory while growing archive dependency list\n");
        return false;
    }
    options->archive_paths = resized;
    options->archive_capacity = new_capacity;
    options->archive_paths[options->archive_count++] = copy;
    return true;
}

static bool calynda_option_add_archive(const char *value,
                                       CalyndaCompileOptions *options,
                                       FILE *err) {
    return calynda_compile_options_append_archive(options, value, err);
}

static bool calynda_option_add_archive_path(const char *value,
                                            CalyndaCompileOptions *options,
                                            FILE *err) {
    DIR *dir;
    struct dirent *entry;
    bool found_any = false;

    dir = opendir(value);
    if (!dir) {
        fprintf(err, "failed to open archive path: %s\n", value);
        return false;
    }

    while ((entry = readdir(dir)) != NULL) {
        size_t length = strlen(entry->d_name);
        char path[PATH_MAX];

        if (length <= 4 || strcmp(entry->d_name + length - 4, ".car") != 0) {
            continue;
        }
        if (snprintf(path, sizeof(path), "%s/%s", value, entry->d_name) < 0) {
            closedir(dir);
            fprintf(err, "failed to build archive dependency path in %s\n", value);
            return false;
        }
        if (!calynda_compile_options_append_archive(options, path, err)) {
            closedir(dir);
            return false;
        }
        found_any = true;
    }

    closedir(dir);
    if (!found_any) {
        fprintf(err, "no .car archives found in %s\n", value);
        return false;
    }
    return true;
}

#include "calynda_commands_p5.inc"
#include "calynda_commands_p3.inc"
