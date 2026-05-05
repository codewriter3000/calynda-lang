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

#include "car_read_p3.inc"
