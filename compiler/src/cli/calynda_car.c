#define _POSIX_C_SOURCE 200809L

#include "calynda_internal.h"
#include "car.h"
#include "target.h"

#include <stdlib.h>
#include <string.h>

/*
 * Multi-file compilation from a CAR archive.
 *
 * Strategy: merge all top-level declarations from all files into a single
 * AstProgram, then run the normal single-file compilation pipeline.
 *
 * - Each file is parsed independently.
 * - Top-level declarations are transferred (pointer ownership) into a
 *   merged AstProgram.
 * - Imports to packages defined within the archive are stripped.
 * - External imports (e.g. io.stdlib) are preserved and deduplicated.
 * - Exactly one file may contain a start() or boot() declaration.
 */

/* ----------------------------------------------------------------
 *  Helpers: check whether a qualified name matches any archive path
 * ---------------------------------------------------------------- */

/*
 * Convert a qualified name (e.g. "com.example.math") to a file path
 * within the archive (e.g. "com/example/math.cal").
 * Returns a malloc'd string or NULL on failure.
 */
static char *qualified_name_to_archive_path(const AstQualifiedName *name) {
    size_t total_length = 0;
    size_t i;
    char *result;
    char *cursor;

    if (!name || name->count == 0) {
        return NULL;
    }

    for (i = 0; i < name->count; i++) {
        total_length += strlen(name->segments[i]);
        if (i > 0) {
            total_length += 1; /* '/' separator */
        }
    }
    total_length += 4; /* ".cal" suffix */

    result = malloc(total_length + 1);
    if (!result) {
        return NULL;
    }

    cursor = result;
    for (i = 0; i < name->count; i++) {
        size_t part_length = strlen(name->segments[i]);
        if (i > 0) {
            *cursor++ = '/';
        }
        memcpy(cursor, name->segments[i], part_length);
        cursor += part_length;
    }
    memcpy(cursor, ".cal", 4);
    cursor[4] = '\0';

    return result;
}

/*
 * Check whether a qualified import name corresponds to a file in the archive.
 */
static bool import_is_in_archive(const AstImportDecl *import_decl,
                                 const CarArchive *archive) {
    char *path;
    bool found;

    path = qualified_name_to_archive_path(&import_decl->module_name);
    if (!path) {
        return false;
    }

    found = car_archive_find_file(archive, path) != NULL;
    free(path);
    return found;
}

/*
 * Check whether two qualified names are equal.
 */
static bool qualified_names_equal(const AstQualifiedName *a,
                                  const AstQualifiedName *b) {
    size_t i;

    if (a->count != b->count) {
        return false;
    }
    for (i = 0; i < a->count; i++) {
        if (strcmp(a->segments[i], b->segments[i]) != 0) {
            return false;
        }
    }
    return true;
}

/*
 * Check whether an import is already present in the merged program.
 */
static bool import_already_present(const AstProgram *merged,
                                   const AstImportDecl *import_decl) {
    size_t i;

    for (i = 0; i < merged->import_count; i++) {
        if (merged->imports[i].kind == import_decl->kind &&
            qualified_names_equal(&merged->imports[i].module_name,
                                  &import_decl->module_name)) {
            return true;
        }
    }
    return false;
}

/* ----------------------------------------------------------------
 *  Copy an import declaration (deep copy for the merged program)
 * ---------------------------------------------------------------- */

static bool copy_import_decl(AstImportDecl *dst, const AstImportDecl *src) {
    size_t i;

    memset(dst, 0, sizeof(*dst));
    dst->kind = src->kind;

    /* Copy module name */
    ast_qualified_name_init(&dst->module_name);
    for (i = 0; i < src->module_name.count; i++) {
        if (!ast_qualified_name_append(&dst->module_name,
                                         src->module_name.segments[i])) {
            ast_import_decl_free(dst);
            return false;
        }
    }

    /* Copy alias if present */
    if (src->alias) {
        dst->alias = strdup(src->alias);
        if (!dst->alias) {
            ast_import_decl_free(dst);
            return false;
        }
    }

    /* Copy selected names */
    for (i = 0; i < src->selected_count; i++) {
        if (!ast_import_decl_add_selected(dst, src->selected_names[i])) {
            ast_import_decl_free(dst);
            return false;
        }
    }

    return true;
}

/* ----------------------------------------------------------------
 *  Public API
 * ---------------------------------------------------------------- */

int calynda_compile_car_to_machine_program(const CarArchive *archive,
                                           MachineProgram *machine_program,
                                           const TargetDescriptor *target) {
    size_t file_count;
    size_t i, j;
    int exit_code = 0;

    /* Per-file parsing state */
    char **sources = NULL;
    Parser *parsers = NULL;
    AstProgram *programs = NULL;
    bool *parsed = NULL;

    /* Merged program and pipeline state */
    AstProgram merged;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir_program;
    MirProgram mir_program;
    LirProgram lir_program;
    CodegenProgram codegen_program;
    const SymbolTableError *symbol_error;
    const TypeCheckError *type_error;
    char diagnostic[256];

    if (!archive || !machine_program) {
        return 1;
    }

    file_count = archive->file_count;
    if (file_count == 0) {
        fprintf(stderr, "car: archive contains no files\n");
        return 1;
    }

    /* Allocate per-file arrays */
    sources = calloc(file_count, sizeof(*sources));
    parsers = calloc(file_count, sizeof(*parsers));
    programs = calloc(file_count, sizeof(*programs));
    parsed = calloc(file_count, sizeof(*parsed));
    if (!sources || !parsers || !programs || !parsed) {
        fprintf(stderr, "car: out of memory\n");
        exit_code = 1;
        goto cleanup_parse;
    }

    /* ---- Phase 1: Parse all files ---- */
    for (i = 0; i < file_count; i++) {
        const CarFile *file = &archive->files[i];
        const ParserError *parse_error;

        /* Make a mutable copy of the source (parser may modify) */
        sources[i] = malloc(file->content_length + 1);
        if (!sources[i]) {
            fprintf(stderr, "car: out of memory for %s\n", file->path);
            exit_code = 1;
            goto cleanup_parse;
        }
        memcpy(sources[i], file->content, file->content_length);
        sources[i][file->content_length] = '\0';

        parser_init(&parsers[i], sources[i]);
        ast_program_init(&programs[i]);

        if (!parser_parse_program(&parsers[i], &programs[i])) {
            parse_error = parser_get_error(&parsers[i]);
            if (parse_error) {
                fprintf(stderr, "%s:%d:%d: parse error: %s\n",
                        file->path,
                        parse_error->token.line,
                        parse_error->token.column,
                        parse_error->message);
            }
            exit_code = 1;
            goto cleanup_parse;
        }
        parsed[i] = true;
    }

    /* ---- Phase 2: Build merged program ---- */
    ast_program_init(&merged);

    /* Transfer top-level declarations from all files */
    for (i = 0; i < file_count; i++) {
        AstProgram *prog = &programs[i];

        for (j = 0; j < prog->top_level_count; j++) {
            if (!ast_program_add_top_level_decl(&merged,
                                                prog->top_level_decls[j])) {
                fprintf(stderr, "car: out of memory during merge\n");
                exit_code = 1;
                goto cleanup_merged;
            }
            /* Ownership transferred — null out in source program */
            prog->top_level_decls[j] = NULL;
        }

        /* Collect non-archive imports (deduplicated) */
        for (j = 0; j < prog->import_count; j++) {
            AstImportDecl *imp = &prog->imports[j];

            if (import_is_in_archive(imp, archive)) {
                continue; /* Skip intra-archive imports */
            }
            if (import_already_present(&merged, imp)) {
                continue; /* Already added */
            }

            {
                AstImportDecl copy;
                if (!copy_import_decl(&copy, imp)) {
                    fprintf(stderr, "car: out of memory copying import\n");
                    exit_code = 1;
                    goto cleanup_merged;
                }
                if (!ast_program_add_import(&merged, &copy)) {
                    ast_import_decl_free(&copy);
                    fprintf(stderr, "car: out of memory adding import\n");
                    exit_code = 1;
                    goto cleanup_merged;
                }
            }
        }
    }

    /* ---- Phase 3: Normal compilation pipeline on merged program ---- */
    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir_program);
    mir_program_init(&mir_program);
    lir_program_init(&lir_program);
    codegen_program_init(&codegen_program);
    machine_program_init(machine_program);

    if (!symbol_table_build(&symbols, &merged)) {
        symbol_error = symbol_table_get_error(&symbols);
        if (symbol_error &&
            symbol_table_format_error(symbol_error, diagnostic,
                                      sizeof(diagnostic))) {
            fprintf(stderr, "car: semantic error: %s\n", diagnostic);
        } else {
            fprintf(stderr, "car: semantic error while building symbols\n");
        }
        exit_code = 1;
        goto cleanup_pipeline;
    }
    if (!type_checker_check_program(&checker, &merged, &symbols)) {
        type_error = type_checker_get_error(&checker);
        if (type_error &&
            type_checker_format_error(type_error, diagnostic,
                                      sizeof(diagnostic))) {
            fprintf(stderr, "car: type error: %s\n", diagnostic);
        } else {
            fprintf(stderr, "car: type error while checking program\n");
        }
        exit_code = 1;
        goto cleanup_pipeline;
    }
    if (!hir_build_program(&hir_program, &merged, &symbols, &checker) ||
        !mir_build_program(&mir_program, &hir_program) ||
        !lir_build_program(&lir_program, &mir_program) ||
        !codegen_build_program(&codegen_program, &lir_program, target) ||
        !machine_build_program(machine_program, &lir_program,
                               &codegen_program)) {
        fprintf(stderr, "car: backend lowering failed\n");
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

    return exit_code;
}
