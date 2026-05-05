            symbol = st_symbol_new(table, SYMBOL_KIND_IMPORT, name, qualified_name,
                                   NULL, false, false,
                                   false, false, false,
                                   false,
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
                                   false,
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
            /* Dependency archive exports are injected after archive metadata
               is loaded, once the module's exported names are known. */
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
                                       false,
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

bool st_symbol_table_load_archive_deps(SymbolTable *table,
                                       const AstProgram *program,
                                       const CarArchive *archive_deps,
                                       size_t archive_dep_count) {
    size_t i;

    if (!table || !program || !archive_deps || archive_dep_count == 0) {
        return true;
    }

    for (i = 0; i < program->import_count; i++) {
        const AstImportDecl *imp = &program->imports[i];
        char *qualified_name = st_qualified_name_to_string(&imp->module_name);
        size_t archive_index;

        if (!qualified_name) {
            st_set_error(table, "Out of memory while loading archive dependencies.");
            return false;
        }

        for (archive_index = 0; archive_index < archive_dep_count; archive_index++) {
            CarExportedSymbol *exported_symbols = NULL;
            size_t exported_count = 0;
            size_t export_index;

            if (!car_archive_collect_exported_symbols(&archive_deps[archive_index],
                                                      qualified_name,
                                                      &exported_symbols,
                                                      &exported_count)) {
                st_set_error(table,
                             "Failed to enumerate exported symbols for archive dependency '%s'.",
                             qualified_name);
                free(qualified_name);
                return false;
            }

            if (imp->kind == AST_IMPORT_SELECTIVE) {
                for (export_index = 0; export_index < exported_count; export_index++) {
                    size_t selected_index;

                    for (selected_index = 0; selected_index < imp->selected_count; selected_index++) {
                        if (strcmp(exported_symbols[export_index].name,
                                   imp->selected_names[selected_index]) == 0) {
                            Symbol *import_symbol = st_find_import_symbol_mutable(
                                table, imp->selected_names[selected_index]);
                            if (import_symbol) {
                                st_attach_export_metadata(import_symbol,
                                                          &exported_symbols[export_index]);
                            }
                            break;
                        }
                    }
                }
            } else if (imp->kind == AST_IMPORT_WILDCARD &&
                       !st_inject_wildcard_package(table,
                                                   imp,
                                                   qualified_name,
                                                   exported_symbols,
                                                   exported_count)) {
                car_exported_symbol_list_free(exported_symbols, exported_count);
                free(qualified_name);
                return false;
            }

            car_exported_symbol_list_free(exported_symbols, exported_count);
        }

        free(qualified_name);
    }

    return true;
}
