
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

static bool calynda_parse_compile_options(int *argc, char ***argv,
                                          CalyndaCompileOptions *options,
                                          FILE *err) {
    int read_index = 0;
    int write_index = 0;

    while (read_index < *argc) {
        const char *current = (*argv)[read_index];
        size_t i;
        bool matched = false;

        if (current && current[0] == '-') {
            for (i = 0; i < sizeof(k_compile_option_specs) / sizeof(k_compile_option_specs[0]); i++) {
                const CalyndaOptionSpec *spec = &k_compile_option_specs[i];
                const char *value = NULL;
                int consumed = 1;

                if (strcmp(current, spec->name) != 0) {
                    continue;
                }
                if (spec->takes_value) {
                    if (read_index + 1 >= *argc) {
                        fprintf(err, "missing value for option: %s\n", current);
                        return false;
                    }
                    value = (*argv)[read_index + 1];
                    consumed = 2;
                }
                if (!spec->apply(value, options, err)) {
                    return false;
                }
                read_index += consumed;
                matched = true;
                break;
            }
        }

        if (!matched) {
            (*argv)[write_index++] = (*argv)[read_index++];
        }
    }
    *argc = write_index;
    return true;
}

static bool calynda_load_archive_dependencies(const CalyndaCompileOptions *options,
                                              CarArchive **archives_out,
                                              size_t *archive_count_out,
                                              FILE *err) {
    CarArchive *archives;
    size_t i;

    if (!archives_out || !archive_count_out) {
        return false;
    }

    *archives_out = NULL;
    *archive_count_out = 0;
    if (!options || options->archive_count == 0) {
        return true;
    }

    archives = calloc(options->archive_count, sizeof(*archives));
    if (!archives) {
        fprintf(err, "out of memory while loading archive dependencies\n");
        return false;
    }

    for (i = 0; i < options->archive_count; i++) {
        car_archive_init(&archives[i]);
        if (!car_archive_read(&archives[i], options->archive_paths[i])) {
            fprintf(err, "%s: %s\n",
                    options->archive_paths[i],
                    car_archive_get_error(&archives[i]));
            calynda_free_archive_dependencies(archives, i + 1);
            return false;
        }
    }

    *archives_out = archives;
    *archive_count_out = options->archive_count;
    return true;
}

static void calynda_free_archive_dependencies(CarArchive *archives,
                                              size_t archive_count) {
    size_t i;

    if (!archives) {
        return;
    }

    for (i = 0; i < archive_count; i++) {
        car_archive_free(&archives[i]);
    }
    free(archives);
}

#include "calynda_commands_p3.inc"
