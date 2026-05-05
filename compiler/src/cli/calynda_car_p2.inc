    /* ---- Phase 3: Normal compilation pipeline on merged program ---- */
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    machine_program_init(machine_program);

    if (!symbol_table_build_with_archive_deps(&symbols, &merged,
                                              archive_deps,
                                              archive_dep_count)) {
        symbol_error = symbol_table_get_error(&symbols);
        if (symbol_error) {
            const char *err_path;
            const char *err_src = car_find_source_for_span(
                source_archive, sources, file_count,
                symbol_error->primary_span.start_line,
                symbol_error->primary_span.start_column, &err_path);
            calynda_print_diagnostic(
                err_path ? err_path : "car", err_src,
                symbol_error->primary_span.start_line,
                symbol_error->primary_span.start_column,
                symbol_error->primary_span.end_column,
                "semantic error", symbol_error->message);
            if (symbol_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        symbol_error->related_span.start_line,
                        symbol_error->related_span.start_column);
        } else {
            size_t fi;
            fprintf(stderr, "car: internal compiler error: semantic pass failed without a diagnostic\n");
            fprintf(stderr, "   This is a compiler bug. Archive files compiled:\n");
            for (fi = 0; fi < file_count; fi++) {
                fprintf(stderr, "     %s\n", source_archive->files[fi].path);
            }
        }
        exit_code = 1;
        goto cleanup_pipeline;
    }
    if (!type_checker_check_program(&checker, &merged, &symbols)) {
        type_error = type_checker_get_error(&checker);
        if (type_error) {
            const char *err_path;
            const char *err_src = car_find_source_for_span(
                source_archive, sources, file_count,
                type_error->primary_span.start_line,
                type_error->primary_span.start_column, &err_path);
            calynda_print_diagnostic(
                err_path ? err_path : "car", err_src,
                type_error->primary_span.start_line,
                type_error->primary_span.start_column,
                type_error->primary_span.end_column,
                "type error", type_error->message);
            if (type_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        type_error->related_span.start_line,
                        type_error->related_span.start_column);
        } else {
            size_t fi;
            fprintf(stderr, "car: internal compiler error: type check failed without a diagnostic\n");
            fprintf(stderr, "   This is a compiler bug. Archive files compiled:\n");
            for (fi = 0; fi < file_count; fi++) {
                fprintf(stderr, "     %s\n", source_archive->files[fi].path);
            }
        }
        exit_code = 1;
        goto cleanup_pipeline;
    }
    if (!hir_build_program(&hir_program, &merged, &symbols, &checker)) {
        const HirBuildError *hir_error = hir_get_error(&hir_program);
        if (hir_error) {
            const char *err_path;
            const char *err_src = car_find_source_for_span(
                source_archive, sources, file_count,
                hir_error->primary_span.start_line,
                hir_error->primary_span.start_column, &err_path);
            calynda_print_diagnostic(
                err_path ? err_path : "car", err_src,
                hir_error->primary_span.start_line,
                hir_error->primary_span.start_column,
                hir_error->primary_span.end_column,
                "HIR error", hir_error->message);
            if (hir_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        hir_error->related_span.start_line,
                        hir_error->related_span.start_column);
        } else {
            fprintf(stderr, "car: HIR lowering failed\n");
        }
        exit_code = 1;
        goto cleanup_pipeline;
    }
    if (!mir_build_program(&mir_program, &hir_program, false)) {
        const MirBuildError *mir_error = mir_get_error(&mir_program);
        if (mir_error) {
            const char *err_path;
            const char *err_src = car_find_source_for_span(
                source_archive, sources, file_count,
                mir_error->primary_span.start_line,
                mir_error->primary_span.start_column, &err_path);
            calynda_print_diagnostic(
                err_path ? err_path : "car", err_src,
                mir_error->primary_span.start_line,
                mir_error->primary_span.start_column,
                mir_error->primary_span.end_column,
                "MIR error", mir_error->message);
        } else {
            fprintf(stderr, "car: MIR lowering failed\n");
        }
        exit_code = 1;
        goto cleanup_pipeline;
    }
    if (!lir_build_program(&lir_program, &mir_program)) {
        const LirBuildError *lir_error = lir_get_error(&lir_program);
        if (lir_error) {
            const char *err_path;
            const char *err_src = car_find_source_for_span(
                source_archive, sources, file_count,
                lir_error->primary_span.start_line,
                lir_error->primary_span.start_column, &err_path);
            calynda_print_diagnostic(
                err_path ? err_path : "car", err_src,
                lir_error->primary_span.start_line,
                lir_error->primary_span.start_column,
                lir_error->primary_span.end_column,
                "LIR error", lir_error->message);
            if (lir_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        lir_error->related_span.start_line,
                        lir_error->related_span.start_column);
        } else {
            fprintf(stderr, "car: LIR lowering failed\n");
        }
        exit_code = 1;
        goto cleanup_pipeline;
    }
    if (!codegen_build_program(&codegen_program, &lir_program, target)) {
        const CodegenBuildError *codegen_error = codegen_get_error(&codegen_program);
        if (codegen_error) {
            const char *err_path;
            const char *err_src = car_find_source_for_span(
                source_archive, sources, file_count,
                codegen_error->primary_span.start_line,
                codegen_error->primary_span.start_column, &err_path);
            calynda_print_diagnostic(
                err_path ? err_path : "car", err_src,
                codegen_error->primary_span.start_line,
                codegen_error->primary_span.start_column,
                codegen_error->primary_span.end_column,
                "codegen error", codegen_error->message);
            if (codegen_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        codegen_error->related_span.start_line,
                        codegen_error->related_span.start_column);
        } else {
            fprintf(stderr, "car: codegen lowering failed\n");
        }
        exit_code = 1;
        goto cleanup_pipeline;
    }
    if (!machine_build_program(machine_program, &lir_program, &codegen_program)) {
        const MachineBuildError *machine_error = machine_get_error(machine_program);
        if (machine_error) {
            const char *err_path;
            const char *err_src = car_find_source_for_span(
                source_archive, sources, file_count,
                machine_error->primary_span.start_line,
                machine_error->primary_span.start_column, &err_path);
            calynda_print_diagnostic(
                err_path ? err_path : "car", err_src,
                machine_error->primary_span.start_line,
                machine_error->primary_span.start_column,
                machine_error->primary_span.end_column,
                "machine error", machine_error->message);
            if (machine_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        machine_error->related_span.start_line,
                        machine_error->related_span.start_column);
        } else {
            fprintf(stderr, "car: machine code generation failed\n");
        }
        exit_code = 1;
        goto cleanup_pipeline;
    }

cleanup_pipeline:
    codegen_program_free(&codegen_program);
    lir_program_free(&lir_program);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);

cleanup_merged:
    ast_program_free(&merged);

cleanup_parse:
    if (programs) {
        for (i = 0; i < file_count; i++) {
            if (parsed && parsed[i]) {
                ast_program_free(&programs[i]);
            }
        }
    }
    if (parsers) {
        for (i = 0; i < file_count; i++) {
            parser_free(&parsers[i]);
        }
    }
    if (sources) {
        for (i = 0; i < file_count; i++) {
            free(sources[i]);
        }
    }
    free(sources);
    free(parsers);
    free(programs);
    free(parsed);
    if (has_combined_archive) {
        car_archive_free(&combined_archive);
    }

    return exit_code;
}

static bool append_archive_files(CarArchive *dst, const CarArchive *src) {
    size_t i;

    if (!dst || !src) {
        return false;
    }

    for (i = 0; i < src->file_count; i++) {
        if (!car_archive_add_file(dst,
                                  src->files[i].path,
                                  src->files[i].content,
                                  src->files[i].content_length)) {
            return false;
        }
    }

    return true;
}
