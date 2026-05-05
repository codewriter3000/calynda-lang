        if (machine_error) {
            calynda_print_diagnostic(path, source,
                                     machine_error->primary_span.start_line,
                                     machine_error->primary_span.start_column,
                                     machine_error->primary_span.end_column,
                                     "machine error", machine_error->message);
            if (machine_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        machine_error->related_span.start_line,
                        machine_error->related_span.start_column);
        } else {
            fprintf(stderr, "%s: backend lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }

cleanup:
    type_checker_set_global_strict_race_check(false);
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

int calynda_compile_to_bytecode_program(const char *path,
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
    int exit_code = 0;

    if (!path || !bytecode_program) {
        return 1;
    }

    source = calynda_read_entire_file(path);
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
            int col_end = parse_error->token.column +
                          (int)(parse_error->token.length > 0 ? parse_error->token.length - 1 : 0);
            calynda_print_diagnostic(path, source,
                                     parse_error->token.line,
                                     parse_error->token.column, col_end,
                                     "parse error", parse_error->message);
        }
        parser_free(&parser);
        free(source);
        return 1;
    }

    if (!symbol_table_build(&symbols, &program)) {
        symbol_error = symbol_table_get_error(&symbols);
        if (symbol_error) {
            calynda_print_diagnostic(path, source,
                                     symbol_error->primary_span.start_line,
                                     symbol_error->primary_span.start_column,
                                     symbol_error->primary_span.end_column,
                                     "semantic error", symbol_error->message);
            if (symbol_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        symbol_error->related_span.start_line,
                        symbol_error->related_span.start_column);
        } else {
            fprintf(stderr, "%s: semantic error while building symbols\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    type_checker_set_global_strict_race_check(g_global_strict_race_check);
    if (!type_checker_check_program(&checker, &program, &symbols)) {
        type_error = type_checker_get_error(&checker);
        if (type_error) {
            calynda_print_diagnostic(path, source,
                                     type_error->primary_span.start_line,
                                     type_error->primary_span.start_column,
                                     type_error->primary_span.end_column,
                                     "type error", type_error->message);
            if (type_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        type_error->related_span.start_line,
                        type_error->related_span.start_column);
        } else {
            fprintf(stderr, "%s: type error while checking program\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!hir_build_program(&hir_program, &program, &symbols, &checker)) {
        const HirBuildError *hir_error = hir_get_error(&hir_program);

        if (hir_error) {
            calynda_print_diagnostic(path, source,
                                     hir_error->primary_span.start_line,
                                     hir_error->primary_span.start_column,
                                     hir_error->primary_span.end_column,
                                     "HIR error", hir_error->message);
            if (hir_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        hir_error->related_span.start_line,
                        hir_error->related_span.start_column);
        } else {
            fprintf(stderr, "%s: HIR lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!mir_build_program(&mir_program, &hir_program, g_global_bounds_check)) {
        const MirBuildError *mir_error = mir_get_error(&mir_program);

        if (mir_error) {
            calynda_print_diagnostic(path, source,
                                     mir_error->primary_span.start_line,
                                     mir_error->primary_span.start_column,
                                     mir_error->primary_span.end_column,
                                     "MIR error", mir_error->message);
        } else {
            fprintf(stderr, "%s: MIR lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!bytecode_build_program(bytecode_program, &mir_program)) {
        bytecode_error = bytecode_get_error(bytecode_program);
        if (bytecode_error) {
            calynda_print_diagnostic(path, source,
                                     bytecode_error->primary_span.start_line,
                                     bytecode_error->primary_span.start_column,
                                     bytecode_error->primary_span.end_column,
                                     "bytecode error", bytecode_error->message);
            if (bytecode_error->has_related_span)
                fprintf(stderr, "   note: related location at %d:%d.\n",
                        bytecode_error->related_span.start_line,
                        bytecode_error->related_span.start_column);
        } else {
            fprintf(stderr, "%s: bytecode lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }

cleanup:
    type_checker_set_global_strict_race_check(false);
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
    free(source);
    return exit_code;
}
