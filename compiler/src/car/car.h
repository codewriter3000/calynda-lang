#ifndef CALYNDA_CAR_H
#define CALYNDA_CAR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "ast.h"

/*
 * CAR (Calynda ARchive) source archive format.
 *
 * Binary layout:
 *   magic:        "CAR\x01"  (4 bytes — format version 1)
 *   file_count:   uint32_le
 *   For each file:
 *     path_length:    uint32_le
 *     path:           UTF-8 bytes (relative path, e.g. "com/example/math.cal")
 *     content_length: uint32_le
 *     content:        UTF-8 source bytes
 */

#define CAR_MAGIC_0 'C'
#define CAR_MAGIC_1 'A'
#define CAR_MAGIC_2 'R'
#define CAR_VERSION  1

typedef struct {
    char   *path;
    char   *content;
    size_t  content_length;
} CarFile;

typedef struct {
    CarFile *files;
    size_t   file_count;
    size_t   file_capacity;
    char     error_message[256];
    bool     has_error;
} CarArchive;

typedef enum {
    CAR_EXPORTED_SYMBOL_VALUE = 0,
    CAR_EXPORTED_SYMBOL_CALLABLE,
    CAR_EXPORTED_SYMBOL_UNION,
    CAR_EXPORTED_SYMBOL_TYPE_ALIAS
} CarExportedSymbolKind;

typedef struct {
    char                  *package_name;
    char                  *name;
    CarExportedSymbolKind  kind;
    AstType                declared_type;
    AstType                return_type;
    AstParameterList       parameters;
    size_t                 generic_param_count;
} CarExportedSymbol;

/* Lifecycle */
void car_archive_init(CarArchive *archive);
void car_archive_free(CarArchive *archive);

/* Reading */
bool car_archive_read(CarArchive *archive, const char *path);
bool car_archive_read_from_memory(CarArchive *archive,
                                  const uint8_t *data,
                                  size_t data_length);

/* Writing */
bool car_archive_write(const CarArchive *archive, const char *path);
bool car_archive_write_to_stream(const CarArchive *archive, FILE *out);

/* Building (for creating archives from source files) */
bool car_archive_add_file(CarArchive *archive,
                          const char *path,
                          const char *content,
                          size_t content_length);

/* Building from a directory tree */
bool car_archive_add_directory(CarArchive *archive, const char *dir_path);

/* Query */
const CarFile *car_archive_find_file(const CarArchive *archive,
                                     const char *path);
const char *car_archive_get_error(const CarArchive *archive);
bool car_archive_collect_exported_symbols(const CarArchive *archive,
                                          const char *package_name,
                                          CarExportedSymbol **symbols_out,
                                          size_t *symbol_count_out);
void car_exported_symbol_free(CarExportedSymbol *symbol);
void car_exported_symbol_list_free(CarExportedSymbol *symbols,
                                   size_t symbol_count);

#endif /* CALYNDA_CAR_H */
