#define _POSIX_C_SOURCE 200809L

#include "car.h"
#include "parser.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t read_uint32_le(const uint8_t *data) {
    return (uint32_t)data[0]
         | ((uint32_t)data[1] << 8)
         | ((uint32_t)data[2] << 16)
         | ((uint32_t)data[3] << 24);
}

static bool car_copy_type(AstType *dst, const AstType *src) {
    size_t i;

    if (!dst || !src) {
        return false;
    }

    memset(dst, 0, sizeof(*dst));
    dst->kind = src->kind;
    dst->primitive = src->primitive;
    if (src->name) {
        dst->name = ast_copy_text(src->name);
        if (!dst->name) {
            return false;
        }
    }
    for (i = 0; i < src->dimension_count; i++) {
        if (!ast_type_add_dimension(dst,
                                    src->dimensions[i].has_size,
                                    src->dimensions[i].size_literal)) {
            ast_type_free(dst);
            return false;
        }
    }
    for (i = 0; i < src->generic_args.count; i++) {
        AstGenericArg arg;

        memset(&arg, 0, sizeof(arg));
        arg.kind = src->generic_args.items[i].kind;
        if (arg.kind == AST_GENERIC_ARG_TYPE) {
            arg.type = calloc(1, sizeof(*arg.type));
            if (!arg.type || !car_copy_type(arg.type, src->generic_args.items[i].type)) {
                if (arg.type) {
                    ast_type_free(arg.type);
                    free(arg.type);
                }
                ast_type_free(dst);
                return false;
            }
        }
        if (!ast_type_add_generic_arg(dst, &arg)) {
            if (arg.type) {
                ast_type_free(arg.type);
                free(arg.type);
            }
            ast_type_free(dst);
            return false;
        }
    }

    return true;
}

static bool car_copy_parameter_list(AstParameterList *dst,
                                    const AstParameterList *src) {
    size_t i;

    if (!dst || !src) {
        return false;
    }

    ast_parameter_list_init(dst);
    for (i = 0; i < src->count; i++) {
        AstParameter parameter;

        memset(&parameter, 0, sizeof(parameter));
        parameter.is_varargs = src->items[i].is_varargs;
        parameter.name_span = src->items[i].name_span;
        if (!car_copy_type(&parameter.type, &src->items[i].type)) {
            ast_parameter_list_free(dst);
            return false;
        }
        if (src->items[i].name) {
            parameter.name = ast_copy_text(src->items[i].name);
            if (!parameter.name) {
                ast_type_free(&parameter.type);
                ast_parameter_list_free(dst);
                return false;
            }
        }
        if (!ast_parameter_list_append(dst, &parameter)) {
            ast_type_free(&parameter.type);
            free(parameter.name);
            ast_parameter_list_free(dst);
            return false;
        }
    }

    return true;
}

static bool car_exported_symbol_append(CarExportedSymbol **symbols,
                                       size_t *count,
                                       size_t *capacity,
                                       CarExportedSymbol *symbol) {
    CarExportedSymbol *resized;
    size_t new_capacity;

    if (!symbols || !count || !capacity || !symbol) {
        return false;
    }
    if (*count < *capacity) {
        (*symbols)[(*count)++] = *symbol;
        memset(symbol, 0, sizeof(*symbol));
        return true;
    }

    new_capacity = *capacity == 0 ? 4 : *capacity * 2;
    resized = realloc(*symbols, new_capacity * sizeof(*resized));
    if (!resized) {
        return false;
    }
    *symbols = resized;
    *capacity = new_capacity;
    (*symbols)[(*count)++] = *symbol;
    memset(symbol, 0, sizeof(*symbol));
    return true;
}

static bool car_qualified_name_matches(const AstQualifiedName *name,
                                       const char *package_name) {
    size_t i;
    size_t offset = 0;

    if (!name || !package_name) {
        return false;
    }
    if (name->count == 0) {
        return package_name[0] == '\0';
    }

    for (i = 0; i < name->count; i++) {
        size_t length = strlen(name->segments[i]);

        if (strncmp(package_name + offset, name->segments[i], length) != 0) {
            return false;
        }
        offset += length;
        if (i + 1 < name->count) {
            if (package_name[offset] != '.') {
                return false;
            }
            offset++;
        }
    }

    return package_name[offset] == '\0';
}

static bool car_decl_is_exported(const AstModifier *modifiers, size_t modifier_count) {
    return ast_decl_has_modifier(modifiers, modifier_count, AST_MODIFIER_EXPORT);
}

static bool car_collect_program_exports(const AstProgram *program,
                                        const char *package_name,
                                        CarExportedSymbol **symbols,
                                        size_t *count,
                                        size_t *capacity) {
    size_t i;

    if (!program || !package_name || !symbols || !count || !capacity) {
        return false;
    }
    if (!program->has_package ||
        !car_qualified_name_matches(&program->package_name, package_name)) {
        return true;
    }

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];
        CarExportedSymbol symbol;

        if (!decl) {
            continue;
        }

        memset(&symbol, 0, sizeof(symbol));
        symbol.package_name = ast_copy_text(package_name);
        if (!symbol.package_name) {
            return false;
        }

        switch (decl->kind) {
        case AST_TOP_LEVEL_BINDING:
            if (!car_decl_is_exported(decl->as.binding_decl.modifiers,
                                      decl->as.binding_decl.modifier_count)) {
                car_exported_symbol_free(&symbol);
                continue;
            }
            symbol.name = ast_copy_text(decl->as.binding_decl.name);
            if (!symbol.name ||
                !car_copy_type(&symbol.declared_type, &decl->as.binding_decl.declared_type)) {
                car_exported_symbol_free(&symbol);
                return false;
            }
            if (decl->as.binding_decl.initializer &&
                decl->as.binding_decl.initializer->kind == AST_EXPR_LAMBDA) {
                symbol.kind = CAR_EXPORTED_SYMBOL_CALLABLE;
                if (!car_copy_type(&symbol.return_type, &decl->as.binding_decl.declared_type) ||
                    !car_copy_parameter_list(&symbol.parameters,
                                             &decl->as.binding_decl.initializer->as.lambda.parameters)) {
                    car_exported_symbol_free(&symbol);
                    return false;
                }
            } else {
                symbol.kind = CAR_EXPORTED_SYMBOL_VALUE;
            }
            break;

        case AST_TOP_LEVEL_ASM:
            if (!car_decl_is_exported(decl->as.asm_decl.modifiers,
                                      decl->as.asm_decl.modifier_count)) {
                car_exported_symbol_free(&symbol);
                continue;
            }
            symbol.kind = CAR_EXPORTED_SYMBOL_CALLABLE;
            symbol.name = ast_copy_text(decl->as.asm_decl.name);
            if (!symbol.name ||
                !car_copy_type(&symbol.return_type, &decl->as.asm_decl.return_type) ||
                !car_copy_parameter_list(&symbol.parameters,
                                         &decl->as.asm_decl.parameters)) {
                car_exported_symbol_free(&symbol);
                return false;
            }
            break;

        case AST_TOP_LEVEL_UNION:
            if (!car_decl_is_exported(decl->as.union_decl.modifiers,
                                      decl->as.union_decl.modifier_count)) {
                car_exported_symbol_free(&symbol);
                continue;
            }
            symbol.kind = CAR_EXPORTED_SYMBOL_UNION;
            symbol.name = ast_copy_text(decl->as.union_decl.name);
            symbol.generic_param_count = decl->as.union_decl.generic_param_count;
            if (!symbol.name) {
                car_exported_symbol_free(&symbol);
                return false;
            }
            break;

        case AST_TOP_LEVEL_TYPE_ALIAS:
            if (!car_decl_is_exported(decl->as.type_alias_decl.modifiers,
                                      decl->as.type_alias_decl.modifier_count)) {
                car_exported_symbol_free(&symbol);
                continue;
            }
            symbol.kind = CAR_EXPORTED_SYMBOL_TYPE_ALIAS;
            symbol.name = ast_copy_text(decl->as.type_alias_decl.name);
            if (!symbol.name ||
                !car_copy_type(&symbol.declared_type,
                               &decl->as.type_alias_decl.target_type)) {
                car_exported_symbol_free(&symbol);
                return false;
            }
            break;

        case AST_TOP_LEVEL_START:
        case AST_TOP_LEVEL_LAYOUT:
            car_exported_symbol_free(&symbol);
            continue;
        }

        if (!car_exported_symbol_append(symbols, count, capacity, &symbol)) {
            car_exported_symbol_free(&symbol);
            return false;
        }
    }

    return true;
}

void car_exported_symbol_free(CarExportedSymbol *symbol) {
    if (!symbol) {
        return;
    }

    free(symbol->package_name);
    free(symbol->name);
    ast_type_free(&symbol->declared_type);
    ast_type_free(&symbol->return_type);
    ast_parameter_list_free(&symbol->parameters);
    memset(symbol, 0, sizeof(*symbol));
}

void car_exported_symbol_list_free(CarExportedSymbol *symbols,
                                   size_t symbol_count) {
    size_t i;

    if (!symbols) {
        return;
    }

    for (i = 0; i < symbol_count; i++) {
        car_exported_symbol_free(&symbols[i]);
    }
    free(symbols);
}

bool car_archive_collect_exported_symbols(const CarArchive *archive,
                                          const char *package_name,
                                          CarExportedSymbol **symbols_out,
                                          size_t *symbol_count_out) {
    size_t i;
    CarExportedSymbol *symbols = NULL;
    size_t symbol_count = 0;
    size_t symbol_capacity = 0;

    if (!archive || !package_name || !symbols_out || !symbol_count_out) {
        return false;
    }

    *symbols_out = NULL;
    *symbol_count_out = 0;
    for (i = 0; i < archive->file_count; i++) {
        char *source = NULL;
        Parser parser;
        AstProgram program;
        bool parsed = false;

        source = malloc(archive->files[i].content_length + 1);
        if (!source) {
            car_exported_symbol_list_free(symbols, symbol_count);
            return false;
        }
        memcpy(source, archive->files[i].content, archive->files[i].content_length);
        source[archive->files[i].content_length] = '\0';

        parser_init(&parser, source);
        ast_program_init(&program);
        if (!parser_parse_program(&parser, &program)) {
            parser_free(&parser);
            ast_program_free(&program);
            free(source);
            car_exported_symbol_list_free(symbols, symbol_count);
            return false;
        }
        parsed = true;

        if (!car_collect_program_exports(&program, package_name,
                                         &symbols, &symbol_count, &symbol_capacity)) {
            if (parsed) {
                ast_program_free(&program);
            }
            parser_free(&parser);
            free(source);
            car_exported_symbol_list_free(symbols, symbol_count);
            return false;
        }

        ast_program_free(&program);
        parser_free(&parser);
        free(source);
    }

    *symbols_out = symbols;
    *symbol_count_out = symbol_count;
    return true;
}

bool car_archive_read_from_memory(CarArchive *archive,
                                  const uint8_t *data,
                                  size_t data_length) {
    uint32_t file_count;
    size_t offset = 0;
    size_t i;

    if (!archive || !data) {
        return false;
    }

    /* Read and validate magic header */
    if (data_length < 8) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "Archive too small to contain a valid header.");
        archive->has_error = true;
        return false;
    }

    if (data[0] != CAR_MAGIC_0 || data[1] != CAR_MAGIC_1 ||
        data[2] != CAR_MAGIC_2 || data[3] != CAR_VERSION) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "Invalid CAR archive magic or unsupported version.");
        archive->has_error = true;
        return false;
    }
    offset = 4;

    file_count = read_uint32_le(data + offset);
    offset += 4;

    for (i = 0; i < file_count; i++) {
        uint32_t path_length;
        uint32_t content_length;
        char *path;
        char *content;

        /* Read path_length */
        if (offset + 4 > data_length) {
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Truncated archive at file %zu path length.", i);
            archive->has_error = true;
            return false;
        }
        path_length = read_uint32_le(data + offset);
        offset += 4;

        /* Read path */
        if (offset + path_length > data_length) {
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Truncated archive at file %zu path data.", i);
            archive->has_error = true;
            return false;
        }
        path = malloc(path_length + 1);
        if (!path) {
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Out of memory reading file %zu path.", i);
            archive->has_error = true;
            return false;
        }
        memcpy(path, data + offset, path_length);
        path[path_length] = '\0';
        offset += path_length;

        /* Read content_length */
        if (offset + 4 > data_length) {
            free(path);
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Truncated archive at file %zu content length.", i);
            archive->has_error = true;
            return false;
        }
        content_length = read_uint32_le(data + offset);
        offset += 4;

        /* Read content */
        if (offset + content_length > data_length) {
            free(path);
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Truncated archive at file %zu content data.", i);
            archive->has_error = true;
            return false;
        }
        content = malloc(content_length + 1);
        if (!content) {
            free(path);
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Out of memory reading file %zu content.", i);
            archive->has_error = true;
            return false;
        }
        memcpy(content, data + offset, content_length);
        content[content_length] = '\0';
        offset += content_length;

        if (!car_archive_add_file(archive, path, content, content_length)) {
            free(path);
            free(content);
            return false;
        }
        free(path);
        free(content);
    }

    return true;
}

bool car_archive_read(CarArchive *archive, const char *path) {
    FILE *file;
    uint8_t *data;
    long size;
    size_t read_size;
    bool result;

    if (!archive || !path) {
        return false;
    }

    file = fopen(path, "rb");
    if (!file) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: %s", path, strerror(errno));
        archive->has_error = true;
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: failed to seek.", path);
        archive->has_error = true;
        fclose(file);
        return false;
    }
    size = ftell(file);
    if (size < 0) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: failed to determine file size.", path);
        archive->has_error = true;
        fclose(file);
        return false;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: failed to rewind.", path);
        archive->has_error = true;
        fclose(file);
        return false;
    }

    data = malloc((size_t)size);
    if (!data) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: out of memory.", path);
        archive->has_error = true;
        fclose(file);
        return false;
    }

    read_size = fread(data, 1, (size_t)size, file);
    fclose(file);

    if (read_size != (size_t)size) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: failed to read complete file.", path);
        archive->has_error = true;
        free(data);
        return false;
    }

    result = car_archive_read_from_memory(archive, data, (size_t)size);
    free(data);
    return result;
}
