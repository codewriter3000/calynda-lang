#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdlib.h>
#include <string.h>

static Symbol *st_find_import_symbol_mutable(SymbolTable *table, const char *name) {
    size_t i;

    if (!table || !name) {
        return NULL;
    }

    for (i = 0; i < table->import_count; i++) {
        if (table->imports[i] && table->imports[i]->name &&
            strcmp(table->imports[i]->name, name) == 0) {
            return table->imports[i];
        }
    }

    return NULL;
}

static bool st_make_import_qualified_name(SymbolTable *table,
                                          const char *package_name,
                                          const char *symbol_name,
                                          char **qualified_name_out) {
    char *qualified_name;
    size_t package_length;
    size_t symbol_length;

    if (!table || !package_name || !symbol_name || !qualified_name_out) {
        return false;
    }

    package_length = strlen(package_name);
    symbol_length = strlen(symbol_name);
    qualified_name = malloc(package_length + 1 + symbol_length + 1);
    if (!qualified_name) {
        st_set_error(table,
                     "Out of memory while creating imported symbol name.");
        return false;
    }

    memcpy(qualified_name, package_name, package_length);
    qualified_name[package_length] = '.';
    memcpy(qualified_name + package_length + 1, symbol_name, symbol_length);
    qualified_name[package_length + 1 + symbol_length] = '\0';
    *qualified_name_out = qualified_name;
    return true;
}

static void st_reset_external_symbol_metadata(Symbol *symbol) {
    if (!symbol) {
        return;
    }

    if (symbol->has_external_declared_type) {
        ast_type_free(&symbol->external_declared_type);
        memset(&symbol->external_declared_type, 0, sizeof(symbol->external_declared_type));
        symbol->has_external_declared_type = false;
    }
    if (symbol->has_external_return_type) {
        ast_type_free(&symbol->external_return_type);
        memset(&symbol->external_return_type, 0, sizeof(symbol->external_return_type));
        symbol->has_external_return_type = false;
    }
    ast_parameter_list_free(&symbol->external_parameters);
    symbol->declared_type = NULL;
    symbol->generic_param_count = 0;
}

static void st_attach_export_metadata(Symbol *symbol,
                                      CarExportedSymbol *exported_symbol) {
    if (!symbol || !exported_symbol) {
        return;
    }

    st_reset_external_symbol_metadata(symbol);

    switch (exported_symbol->kind) {
    case CAR_EXPORTED_SYMBOL_VALUE:
        symbol->kind = SYMBOL_KIND_IMPORT;
        symbol->external_declared_type = exported_symbol->declared_type;
        memset(&exported_symbol->declared_type, 0, sizeof(exported_symbol->declared_type));
        symbol->has_external_declared_type = true;
        symbol->declared_type = &symbol->external_declared_type;
        break;

    case CAR_EXPORTED_SYMBOL_CALLABLE:
        symbol->kind = SYMBOL_KIND_ASM_BINDING;
        symbol->external_return_type = exported_symbol->return_type;
        memset(&exported_symbol->return_type, 0, sizeof(exported_symbol->return_type));
        symbol->has_external_return_type = true;
        symbol->external_parameters = exported_symbol->parameters;
        memset(&exported_symbol->parameters, 0, sizeof(exported_symbol->parameters));
        break;

    case CAR_EXPORTED_SYMBOL_UNION:
        symbol->kind = SYMBOL_KIND_UNION;
        symbol->generic_param_count = exported_symbol->generic_param_count;
        break;

    case CAR_EXPORTED_SYMBOL_TYPE_ALIAS:
        symbol->kind = SYMBOL_KIND_TYPE_ALIAS;
        symbol->external_declared_type = exported_symbol->declared_type;
        memset(&exported_symbol->declared_type, 0, sizeof(exported_symbol->declared_type));
        symbol->has_external_declared_type = true;
        symbol->declared_type = &symbol->external_declared_type;
        break;
    }
}

static bool st_inject_wildcard_package(SymbolTable *table,
                                       const AstImportDecl *imp,
                                       const char *qualified_name,
                                       CarExportedSymbol *exported_symbols,
                                       size_t exported_count) {
    size_t export_index;

    if (!table || !imp || !qualified_name) {
        return false;
    }

    for (export_index = 0; export_index < exported_count; export_index++) {
        CarExportedSymbol *exported_symbol = &exported_symbols[export_index];
        Symbol *existing_import;
        Symbol *symbol;
        char *import_qualified_name = NULL;

        if (!st_make_import_qualified_name(table,
                                           qualified_name,
                                           exported_symbol->name,
                                           &import_qualified_name)) {
            return false;
        }

        existing_import = st_find_import_symbol_mutable(table, exported_symbol->name);
        if (existing_import) {
            if (existing_import->qualified_name &&
                strcmp(existing_import->qualified_name, import_qualified_name) == 0) {
                st_attach_export_metadata(existing_import, exported_symbol);
                free(import_qualified_name);
                continue;
            }

            st_set_error_at(table,
                            imp->module_name.tail_span,
                            &existing_import->declaration_span,
                            "Ambiguous import: '%s' is already imported.",
                            exported_symbol->name);
            free(import_qualified_name);
            return false;
        }

        symbol = st_symbol_new(table, SYMBOL_KIND_IMPORT,
                               exported_symbol->name, import_qualified_name,
                               NULL, false, false,
                               false, false, false,
                               false,
                               imp->module_name.tail_span,
                               imp, table->root_scope);
        free(import_qualified_name);
        if (!symbol) {
            return false;
        }

        st_attach_export_metadata(symbol, exported_symbol);
        if (!st_imports_append(table, symbol)) {
            st_symbol_free(symbol);
            return false;
        }
    }

    return true;
}

bool st_add_package_symbol(SymbolTable *table, const AstProgram *program) {
    char *name;
    char *qualified_name;

    if (!program->has_package) {
        return true;
    }

    name = st_qualified_name_tail(&program->package_name);
    qualified_name = st_qualified_name_to_string(&program->package_name);
    if (!name || !qualified_name) {
        free(name);
        free(qualified_name);
        st_set_error(table, "Out of memory while creating package symbol.");
        return false;
    }

    table->package_symbol = st_symbol_new(table, SYMBOL_KIND_PACKAGE,
                                          name, qualified_name,
                                          NULL, false, false,
                                          false, false, false,
                                          false,
                                          program->package_name.tail_span,
                                          &program->package_name, NULL);
    free(name);
    free(qualified_name);
    return table->package_symbol != NULL;
}

bool st_add_import_symbols(SymbolTable *table, const AstProgram *program) {
    size_t i;

    for (i = 0; i < program->import_count; i++) {
        const AstImportDecl *imp = &program->imports[i];
        char *qualified_name = st_qualified_name_to_string(&imp->module_name);

        if (!qualified_name) {
            st_set_error(table, "Out of memory while creating import symbol.");
            return false;
        }

        switch (imp->kind) {
        case AST_IMPORT_PLAIN: {
            char *name = st_qualified_name_tail(&imp->module_name);
            const Symbol *existing_import;
            Symbol *symbol;

            if (!name) {
                free(qualified_name);
                st_set_error(table, "Out of memory while creating import symbol.");
                return false;
            }

            existing_import = symbol_table_find_import(table, name);
            if (existing_import) {
                st_set_error_at(table,
                                imp->module_name.tail_span,
                                &existing_import->declaration_span,
                                "Duplicate import alias '%s'.",
                                name);
                free(name);
                free(qualified_name);
                return false;
            }

#include "symbol_table_imports_p2.inc"
