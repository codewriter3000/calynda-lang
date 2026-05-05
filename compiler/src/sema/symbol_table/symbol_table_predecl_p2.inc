            symbol = st_symbol_new(table, SYMBOL_KIND_UNION,
                                   union_decl->name, NULL,
                                   NULL, false, false,
                                   ast_decl_has_modifier(union_decl->modifiers,
                                                         union_decl->modifier_count,
                                                         AST_MODIFIER_EXPORT),
                                   ast_decl_has_modifier(union_decl->modifiers,
                                                         union_decl->modifier_count,
                                                         AST_MODIFIER_STATIC),
                                   false, false,
                                   union_decl->name_span,
                                   union_decl,
                                   table->root_scope);
            if (!symbol) {
                return false;
            }

            symbol->generic_param_count = union_decl->generic_param_count;

            if (!st_scope_append_symbol(table, table->root_scope, symbol)) {
                st_symbol_free(symbol);
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_TYPE_ALIAS) {
            const AstTypeAliasDecl *type_alias_decl = &decl->as.type_alias_decl;
            const OverloadSet *conflicting_overload_set =
                scope_lookup_local_overload_set(table->root_scope, type_alias_decl->name);
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  type_alias_decl->name);
            Symbol *symbol;

            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, type_alias_decl->name);
            }
            if (conflicting_symbol || conflicting_overload_set) {
                return st_report_duplicate(table,
                                           type_alias_decl->name_span,
                                           type_alias_decl->name,
                                           table->root_scope,
                                           conflicting_symbol,
                                           conflicting_overload_set);
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_TYPE_ALIAS,
                                   type_alias_decl->name, NULL,
                                   &type_alias_decl->target_type,
                                   false, true,
                                   ast_decl_has_modifier(type_alias_decl->modifiers,
                                                         type_alias_decl->modifier_count,
                                                         AST_MODIFIER_EXPORT),
                                   false, false, false,
                                   type_alias_decl->name_span,
                                   type_alias_decl,
                                   table->root_scope);
            if (!symbol) {
                return false;
            }

            if (!st_scope_append_symbol(table, table->root_scope, symbol)) {
                st_symbol_free(symbol);
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_ASM) {
            const AstAsmDecl *asm_decl = &decl->as.asm_decl;
            const OverloadSet *existing_overload_set =
                scope_lookup_local_overload_set(table->root_scope, asm_decl->name);
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  asm_decl->name);
            bool can_overload_existing_symbol;
            Symbol *symbol;

            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, asm_decl->name);
            }

            can_overload_existing_symbol =
                st_symbol_is_callable_top_level_decl(conflicting_symbol) &&
                symbol_table_find_import(table, asm_decl->name) == NULL;

            if (conflicting_symbol && !can_overload_existing_symbol) {
                return st_report_duplicate(table,
                                           asm_decl->name_span,
                                           asm_decl->name,
                                           table->root_scope,
                                           conflicting_symbol,
                                           existing_overload_set);
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_ASM_BINDING,
                                   asm_decl->name, NULL,
                                   &asm_decl->return_type,
                                   false,
                                   true,
                                   ast_decl_has_modifier(asm_decl->modifiers,
                                                         asm_decl->modifier_count,
                                                         AST_MODIFIER_EXPORT),
                                   ast_decl_has_modifier(asm_decl->modifiers,
                                                         asm_decl->modifier_count,
                                                         AST_MODIFIER_STATIC),
                                   ast_decl_has_modifier(asm_decl->modifiers,
                                                         asm_decl->modifier_count,
                                                         AST_MODIFIER_INTERNAL),
                                   ast_decl_has_modifier(asm_decl->modifiers,
                                                         asm_decl->modifier_count,
                                                         AST_MODIFIER_THREAD_LOCAL),
                                   asm_decl->name_span,
                                   asm_decl,
                                   table->root_scope);
            if (!symbol) {
                return false;
            }

            if (!st_scope_append_symbol(table, table->root_scope, symbol)) {
                st_symbol_free(symbol);
                return false;
            }
            if (existing_overload_set || can_overload_existing_symbol) {
                if (!st_maybe_attach_to_overload_set(table,
                                                     table->root_scope,
                                                     asm_decl->name,
                                                     symbol,
                                                     existing_overload_set
                                                         ? NULL
                                                         : conflicting_symbol)) {
                    return false;
                }
            }
        } else if (decl->kind == AST_TOP_LEVEL_LAYOUT) {
            const AstLayoutDecl *layout_decl = &decl->as.layout_decl;
            const OverloadSet *conflicting_overload_set =
                scope_lookup_local_overload_set(table->root_scope, layout_decl->name);
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  layout_decl->name);
            Symbol *symbol;

            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, layout_decl->name);
            }
            if (conflicting_symbol || conflicting_overload_set) {
                return st_report_duplicate(table,
                                           layout_decl->name_span,
                                           layout_decl->name,
                                           table->root_scope,
                                           conflicting_symbol,
                                           conflicting_overload_set);
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_LAYOUT,
                                   layout_decl->name, NULL,
                                   NULL, false, false, false, false, false, false,
                                   layout_decl->name_span,
                                   layout_decl,
                                   table->root_scope);
            if (!symbol) {
                return false;
            }

            if (!st_scope_append_symbol(table, table->root_scope, symbol)) {
                st_symbol_free(symbol);
                return false;
            }
        }
    }

    return true;
}
