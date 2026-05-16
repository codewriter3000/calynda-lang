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
