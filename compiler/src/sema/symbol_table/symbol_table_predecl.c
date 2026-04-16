#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdlib.h>
#include <string.h>

bool st_predeclare_top_level_bindings(SymbolTable *table, const AstProgram *program) {
    size_t i;

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (decl->kind == AST_TOP_LEVEL_BINDING) {
            const AstBindingDecl *binding_decl = &decl->as.binding_decl;
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  binding_decl->name);
            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, binding_decl->name);
            }
            Symbol *symbol;

            if (conflicting_symbol) {
                st_set_error_at(table,
                                binding_decl->name_span,
                                &conflicting_symbol->declaration_span,
                                "Duplicate symbol '%s' in %s.",
                                binding_decl->name,
                                scope_kind_name(table->root_scope->kind));
                return false;
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_TOP_LEVEL_BINDING,
                                   binding_decl->name, NULL,
                                   &binding_decl->declared_type,
                                   binding_decl->is_inferred_type,
                                   st_top_level_binding_is_final(binding_decl),
                                   ast_decl_has_modifier(binding_decl->modifiers,
                                                         binding_decl->modifier_count,
                                                         AST_MODIFIER_EXPORT),
                                   ast_decl_has_modifier(binding_decl->modifiers,
                                                         binding_decl->modifier_count,
                                                         AST_MODIFIER_STATIC),
                                   ast_decl_has_modifier(binding_decl->modifiers,
                                                         binding_decl->modifier_count,
                                                         AST_MODIFIER_INTERNAL),
                                   binding_decl->name_span,
                                   binding_decl,
                                   table->root_scope);
            if (!symbol) {
                return false;
            }

            if (!st_scope_append_symbol(table, table->root_scope, symbol)) {
                st_symbol_free(symbol);
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_UNION) {
            const AstUnionDecl *union_decl = &decl->as.union_decl;
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  union_decl->name);
            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, union_decl->name);
            }
            Symbol *symbol;

            if (conflicting_symbol) {
                st_set_error_at(table,
                                union_decl->name_span,
                                &conflicting_symbol->declaration_span,
                                "Duplicate symbol '%s' in %s.",
                                union_decl->name,
                                scope_kind_name(table->root_scope->kind));
                return false;
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_UNION,
                                   union_decl->name, NULL,
                                   NULL, false, false,
                                   ast_decl_has_modifier(union_decl->modifiers,
                                                         union_decl->modifier_count,
                                                         AST_MODIFIER_EXPORT),
                                   ast_decl_has_modifier(union_decl->modifiers,
                                                         union_decl->modifier_count,
                                                         AST_MODIFIER_STATIC),
                                   false,
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
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  type_alias_decl->name);
            Symbol *symbol;

            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, type_alias_decl->name);
            }
            if (conflicting_symbol) {
                st_set_error_at(table,
                                type_alias_decl->name_span,
                                &conflicting_symbol->declaration_span,
                                "Duplicate symbol '%s' in %s.",
                                type_alias_decl->name,
                                scope_kind_name(table->root_scope->kind));
                return false;
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_TYPE_ALIAS,
                                   type_alias_decl->name, NULL,
                                   &type_alias_decl->target_type,
                                   false, true,
                                   ast_decl_has_modifier(type_alias_decl->modifiers,
                                                         type_alias_decl->modifier_count,
                                                         AST_MODIFIER_EXPORT),
                                   false,
                                   false,
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
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  asm_decl->name);
            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, asm_decl->name);
            }
            Symbol *symbol;

            if (conflicting_symbol) {
                st_set_error_at(table,
                                asm_decl->name_span,
                                &conflicting_symbol->declaration_span,
                                "Duplicate symbol '%s' in %s.",
                                asm_decl->name,
                                scope_kind_name(table->root_scope->kind));
                return false;
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_ASM_BINDING,
                                   asm_decl->name, NULL,
                                   &asm_decl->return_type,
                                   false, /* is_inferred_type */
                                   true,  /* is_final — asm bindings are always final */
                                   ast_decl_has_modifier(asm_decl->modifiers,
                                                         asm_decl->modifier_count,
                                                         AST_MODIFIER_EXPORT),
                                   ast_decl_has_modifier(asm_decl->modifiers,
                                                         asm_decl->modifier_count,
                                                         AST_MODIFIER_STATIC),
                                   ast_decl_has_modifier(asm_decl->modifiers,
                                                         asm_decl->modifier_count,
                                                         AST_MODIFIER_INTERNAL),
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
        } else if (decl->kind == AST_TOP_LEVEL_LAYOUT) {
            const AstLayoutDecl *layout_decl = &decl->as.layout_decl;
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  layout_decl->name);
            Symbol *symbol;

            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, layout_decl->name);
            }
            if (conflicting_symbol) {
                st_set_error_at(table,
                                layout_decl->name_span,
                                &conflicting_symbol->declaration_span,
                                "Duplicate symbol '%s' in %s.",
                                layout_decl->name,
                                scope_kind_name(table->root_scope->kind));
                return false;
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_LAYOUT,
                                   layout_decl->name, NULL,
                                   NULL, false, false, false, false, false,
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
