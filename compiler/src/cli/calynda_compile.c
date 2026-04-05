#include "calynda_internal.h"
#include "target.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

int calynda_compile_to_machine_program(const char *path,
                                       MachineProgram *machine_program,
                                       const TargetDescriptor *target) {
    char *source;
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    CodegenProgram codegen_program;
    const ParserError *parse_error;
    const SymbolTableError *symbol_error;
    const TypeCheckError *type_error;
    char diagnostic[256];
    int exit_code = 0;

    if (!path || !machine_program) {
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
    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    machine_program_init(machine_program);
    parser_init(&parser, source);

    if (!parser_parse_program(&parser, &program)) {
        parse_error = parser_get_error(&parser);
        if (parse_error) {
            fprintf(stderr,
                    "%s:%d:%d: parse error: %s\n",
                    path,
                    parse_error->token.line,
                    parse_error->token.column,
                    parse_error->message);
        }
        parser_free(&parser);
        free(source);
        return 1;
    }

    if (!symbol_table_build(&symbols, &program)) {
        symbol_error = symbol_table_get_error(&symbols);
        if (symbol_error && symbol_table_format_error(symbol_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: semantic error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: semantic error while building symbols\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!type_checker_check_program(&checker, &program, &symbols)) {
        type_error = type_checker_get_error(&checker);
        if (type_error && type_checker_format_error(type_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: type error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: type error while checking program\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!hir_build_program(&hir_program, &program, &symbols, &checker) ||
        !mir_build_program(&mir_program, &hir_program) ||
        !lir_build_program(&lir_program, &mir_program) ||
        !codegen_build_program(&codegen_program, &lir_program, target) ||
        !machine_build_program(machine_program, &lir_program, &codegen_program)) {
        fprintf(stderr, "%s: backend lowering failed\n", path);
        exit_code = 1;
        goto cleanup;
    }

cleanup:
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
    char diagnostic[256];
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
            fprintf(stderr,
                    "%s:%d:%d: parse error: %s\n",
                    path,
                    parse_error->token.line,
                    parse_error->token.column,
                    parse_error->message);
        }
        parser_free(&parser);
        free(source);
        return 1;
    }

    if (!symbol_table_build(&symbols, &program)) {
        symbol_error = symbol_table_get_error(&symbols);
        if (symbol_error && symbol_table_format_error(symbol_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: semantic error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: semantic error while building symbols\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!type_checker_check_program(&checker, &program, &symbols)) {
        type_error = type_checker_get_error(&checker);
        if (type_error && type_checker_format_error(type_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: type error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: type error while checking program\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }
    if (!hir_build_program(&hir_program, &program, &symbols, &checker) ||
        !mir_build_program(&mir_program, &hir_program) ||
        !bytecode_build_program(bytecode_program, &mir_program)) {
        bytecode_error = bytecode_get_error(bytecode_program);
        if (bytecode_error && bytecode_format_error(bytecode_error, diagnostic, sizeof(diagnostic))) {
            fprintf(stderr, "%s: bytecode error: %s\n", path, diagnostic);
        } else {
            fprintf(stderr, "%s: bytecode lowering failed\n", path);
        }
        exit_code = 1;
        goto cleanup;
    }

cleanup:
    mir_program_free(&mir_program);
    hir_program_free(&hir_program);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
    free(source);
    return exit_code;
}
