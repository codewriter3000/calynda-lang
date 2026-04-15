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
char *qualified_name_to_archive_path(const AstQualifiedName *name) {
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
bool import_is_in_archive(const AstImportDecl *import_decl,
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
bool qualified_names_equal(const AstQualifiedName *a,
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
bool import_already_present(const AstProgram *merged,
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

bool copy_import_decl(AstImportDecl *dst, const AstImportDecl *src) {
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

