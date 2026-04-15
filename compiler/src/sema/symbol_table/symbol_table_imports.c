#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdlib.h>
#include <string.h>

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

            symbol = st_symbol_new(table, SYMBOL_KIND_IMPORT, name, qualified_name,
                                   NULL, false, false,
                                   false, false, false,
                                   imp->module_name.tail_span,
                                   imp, table->root_scope);
            free(name);
            free(qualified_name);
            if (!symbol) {
                return false;
            }

            if (!st_imports_append(table, symbol)) {
                st_symbol_free(symbol);
                return false;
            }
            break;
        }

        case AST_IMPORT_ALIAS: {
            const Symbol *existing_import;
            Symbol *symbol;

            existing_import = symbol_table_find_import(table, imp->alias);
            if (existing_import) {
                st_set_error_at(table,
                                imp->module_name.tail_span,
                                &existing_import->declaration_span,
                                "Duplicate import alias '%s'.",
                                imp->alias);
                free(qualified_name);
                return false;
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_IMPORT, imp->alias, qualified_name,
                                   NULL, false, false,
                                   false, false, false,
                                   imp->module_name.tail_span,
                                   imp, table->root_scope);
            free(qualified_name);
            if (!symbol) {
                return false;
            }

            if (!st_imports_append(table, symbol)) {
                st_symbol_free(symbol);
                return false;
            }
            break;
        }

        case AST_IMPORT_WILDCARD:
            /* Wildcard imports bind names lazily at resolution time.
               Ambiguity detection across multiple wildcard imports
               requires a module loader to know each module's exported
               names.  Once a module loader exists, duplicate exported
               names from different wildcard imports must be flagged as
               compile errors per the V2 ambiguity spec. */
            free(qualified_name);
            break;

        case AST_IMPORT_SELECTIVE: {
            size_t j;

            for (j = 0; j < imp->selected_count; j++) {
                const Symbol *existing_import;
                Symbol *symbol;
                char *sel_qualified = NULL;
                size_t qlen = strlen(qualified_name);
                size_t slen = strlen(imp->selected_names[j]);

                sel_qualified = malloc(qlen + 1 + slen + 1);
                if (!sel_qualified) {
                    free(qualified_name);
                    st_set_error(table,
                                 "Out of memory while creating selective import.");
                    return false;
                }
                memcpy(sel_qualified, qualified_name, qlen);
                sel_qualified[qlen] = '.';
                memcpy(sel_qualified + qlen + 1, imp->selected_names[j], slen);
                sel_qualified[qlen + 1 + slen] = '\0';

                existing_import = symbol_table_find_import(table,
                                                           imp->selected_names[j]);
                if (existing_import) {
                    st_set_error_at(table,
                                    imp->module_name.tail_span,
                                    &existing_import->declaration_span,
                                    "Ambiguous import: '%s' is already imported.",
                                    imp->selected_names[j]);
                    free(sel_qualified);
                    free(qualified_name);
                    return false;
                }

                symbol = st_symbol_new(table, SYMBOL_KIND_IMPORT,
                                       imp->selected_names[j], sel_qualified,
                                       NULL, false, false,
                                       false, false, false,
                                       imp->module_name.tail_span,
                                       imp, table->root_scope);
                free(sel_qualified);
                if (!symbol) {
                    free(qualified_name);
                    return false;
                }

                if (!st_imports_append(table, symbol)) {
                    st_symbol_free(symbol);
                    free(qualified_name);
                    return false;
                }
            }

            free(qualified_name);
            break;
        }
        }
    }

    return true;
}
