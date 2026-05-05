#define _POSIX_C_SOURCE 200809L

#include "calynda_internal.h"
#include "car.h"
#include "target.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* calynda_car_helpers.c */
char *qualified_name_to_archive_path(const AstQualifiedName *name);
bool import_is_in_archive(const AstImportDecl *import_decl,
                           const CarArchive *archive);
bool qualified_names_equal(const AstQualifiedName *a, const AstQualifiedName *b);
bool import_already_present(const AstProgram *merged,
                              const AstImportDecl *import_decl);
bool copy_import_decl(AstImportDecl *dst, const AstImportDecl *src);
static bool append_archive_files(CarArchive *dst, const CarArchive *src);

/* Scan to a specific line in a source string (1-based). */
static const char *car_diag_find_line(const char *source, int target_line) {
    int cur = 1;
    if (target_line <= 0 || !source) return NULL;
    while (*source) {
        if (cur == target_line) return source;
        if (*source++ == '\n') cur++;
    }
    return (cur == target_line) ? source : NULL;
}

/* Find the archive source file most likely to contain an error at line:col.
 *
 * Primary heuristic: the correct file will have a non-whitespace character at
 * exactly col_start on that line (the error token starts there). Files that
 * have fewer characters on that line (e.g. a closing "};") will be skipped.
 *
 * Fallback: file with fewest total lines that still has >= target_line lines. */
static const char *car_find_source_for_span(const CarArchive *archive,
                                             char * const *sources,
                                             size_t source_count,
                                             int line, int col_start,
                                             const char **path_out) {
    size_t i;
    const char *best_src = NULL;
    const char *best_path = NULL;
    int best_lines = INT_MAX;

    *path_out = NULL;
    if (!sources || line <= 0) return NULL;

    /* First pass: prefer file where col_start has non-whitespace on that line */
    if (col_start > 0) {
        for (i = 0; i < source_count; i++) {
            const char *line_ptr;
            int line_len;
            if (!sources[i]) continue;
            line_ptr = car_diag_find_line(sources[i], line);
            if (!line_ptr) continue;
            line_len = 0;
            while (line_ptr[line_len] && line_ptr[line_len] != '\n') line_len++;
            if (line_len >= col_start &&
                    (unsigned char)line_ptr[col_start - 1] > ' ') {
                *path_out = archive->files[i].path;
                return sources[i];
            }
        }
    }

    /* Fallback: file with fewest lines that still has >= line lines */
    for (i = 0; i < source_count; i++) {
        const char *p;
        int lines = 1;
        if (!sources[i]) continue;
        for (p = sources[i]; *p; p++)
            if (*p == '\n') lines++;
        if (lines >= line && lines < best_lines) {
            best_lines = lines;
            best_src   = sources[i];
            best_path  = archive->files[i].path;
        }
    }
    *path_out = best_path;
    return best_src;
}

/* ----------------------------------------------------------------
 *  Public API
 * ---------------------------------------------------------------- */

int calynda_compile_car_to_machine_program(const CarArchive *archive,
                                           MachineProgram *machine_program,
                                           const TargetDescriptor *target,
                                           const CarArchive *archive_deps,
                                           size_t archive_dep_count) {
    const CarArchive *source_archive = archive;
    CarArchive combined_archive;
    bool has_combined_archive = false;
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

    if (!archive || !machine_program) {
        return 1;
    }

    if (archive_dep_count > 0) {
        size_t dep_index;

        car_archive_init(&combined_archive);
        if (!append_archive_files(&combined_archive, archive)) {
            fprintf(stderr, "car: %s\n", car_archive_get_error(&combined_archive));
            car_archive_free(&combined_archive);
            return 1;
        }
        for (dep_index = 0; dep_index < archive_dep_count; dep_index++) {
            if (!append_archive_files(&combined_archive, &archive_deps[dep_index])) {
                fprintf(stderr, "car: %s\n", car_archive_get_error(&combined_archive));
                car_archive_free(&combined_archive);
                return 1;
            }
        }
        source_archive = &combined_archive;
        has_combined_archive = true;
    }

    file_count = source_archive->file_count;
    if (file_count == 0) {
        fprintf(stderr, "car: archive contains no files\n");
        if (has_combined_archive) {
            car_archive_free(&combined_archive);
        }
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
        const CarFile *file = &source_archive->files[i];
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
                int col_end = parse_error->token.column +
                              (int)(parse_error->token.length > 0
                                    ? parse_error->token.length - 1 : 0);
                calynda_print_diagnostic(file->path, sources[i],
                                         parse_error->token.line,
                                         parse_error->token.column, col_end,
                                         "parse error", parse_error->message);
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

            if (import_is_in_archive(imp, source_archive)) {
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

#include "calynda_car_p2.inc"
