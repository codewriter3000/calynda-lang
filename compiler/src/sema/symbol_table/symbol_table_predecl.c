#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool st_binding_decl_is_callable(const AstBindingDecl *binding_decl) {
    return binding_decl &&
           binding_decl->initializer &&
           binding_decl->initializer->kind == AST_EXPR_LAMBDA;
}

static bool st_symbol_is_callable_top_level_decl(const Symbol *symbol) {
    if (!symbol) {
        return false;
    }

    if (symbol->kind == SYMBOL_KIND_ASM_BINDING) {
        return true;
    }

    if (symbol->kind != SYMBOL_KIND_TOP_LEVEL_BINDING || !symbol->declaration) {
        return false;
    }

    return st_binding_decl_is_callable((const AstBindingDecl *)symbol->declaration);
}

static bool st_set_unique_callable_name(SymbolTable *table, Symbol *symbol) {
    char unique_name[256];
    int written;
    char *copied_name;

    if (!symbol || !symbol->name) {
        return false;
    }

    written = snprintf(unique_name,
                       sizeof(unique_name),
                       "%s$%d_%d",
                       symbol->name,
                       symbol->declaration_span.start_line,
                       symbol->declaration_span.start_column);
    if (written < 0 || (size_t)written >= sizeof(unique_name)) {
        st_set_error_at(table,
                        symbol->declaration_span,
                        NULL,
                        "Failed to name overloaded callable '%s'.",
                        symbol->name);
        return false;
    }

    copied_name = ast_copy_text(unique_name);
    if (!copied_name) {
        st_set_error(table, "Out of memory while naming overloaded callables.");
        return false;
    }

    free(symbol->qualified_name);
    symbol->qualified_name = copied_name;
    return true;
}

static bool st_maybe_attach_to_overload_set(SymbolTable *table,
                                            Scope *scope,
                                            const char *name,
                                            Symbol *symbol,
                                            const Symbol *existing_symbol) {
    const OverloadSet *existing_overload_set = scope_lookup_local_overload_set(scope, name);
    OverloadSet *overload_set;

    if (existing_overload_set) {
        if (!st_set_unique_callable_name(table, symbol)) {
            return false;
        }
        return st_overload_set_append_symbol(table,
                                             (OverloadSet *)existing_overload_set,
                                             symbol);
    }

    if (existing_symbol &&
        !st_set_unique_callable_name(table, (Symbol *)existing_symbol)) {
        return false;
    }
    if (!st_set_unique_callable_name(table, symbol)) {
        return false;
    }

    overload_set = st_overload_set_new(table,
                                       name,
                                       scope,
                                       existing_symbol ? existing_symbol->declaration_span
                                                       : symbol->declaration_span);
    if (!overload_set) {
        return false;
    }

    if (existing_symbol &&
        !st_overload_set_append_symbol(table, overload_set, (Symbol *)existing_symbol)) {
        st_overload_set_free(overload_set);
        return false;
    }
    if (!st_overload_set_append_symbol(table, overload_set, symbol)) {
        st_overload_set_free(overload_set);
        return false;
    }
    if (!st_scope_append_overload_set(table, scope, overload_set)) {
        st_overload_set_free(overload_set);
        return false;
    }

    return true;
}

static bool st_report_duplicate(SymbolTable *table,
                                AstSourceSpan primary_span,
                                const char *name,
                                const Scope *scope,
                                const Symbol *conflicting_symbol,
                                const OverloadSet *conflicting_overload_set) {
    const AstSourceSpan *related_span = NULL;

    if (conflicting_symbol) {
        related_span = &conflicting_symbol->declaration_span;
    } else if (conflicting_overload_set) {
        related_span = &conflicting_overload_set->declaration_span;
    }

    st_set_error_at(table,
                    primary_span,
                    related_span,
                    "Duplicate symbol '%s' in %s.",
                    name,
                    scope_kind_name(scope->kind));
    return false;
}

bool st_predeclare_top_level_bindings(SymbolTable *table, const AstProgram *program) {
    size_t i;

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (decl->kind == AST_TOP_LEVEL_BINDING) {
            const AstBindingDecl *binding_decl = &decl->as.binding_decl;
            const OverloadSet *existing_overload_set =
                scope_lookup_local_overload_set(table->root_scope, binding_decl->name);
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  binding_decl->name);
            bool can_overload_existing_symbol;
            Symbol *symbol;

            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, binding_decl->name);
            }

            can_overload_existing_symbol =
                st_binding_decl_is_callable(binding_decl) &&
                st_symbol_is_callable_top_level_decl(conflicting_symbol) &&
                symbol_table_find_import(table, binding_decl->name) == NULL;

            if ((conflicting_symbol && !can_overload_existing_symbol) ||
                (existing_overload_set && !st_binding_decl_is_callable(binding_decl))) {
                return st_report_duplicate(table,
                                           binding_decl->name_span,
                                           binding_decl->name,
                                           table->root_scope,
                                           conflicting_symbol,
                                           existing_overload_set);
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
                                   ast_decl_has_modifier(binding_decl->modifiers,
                                                         binding_decl->modifier_count,
                                                         AST_MODIFIER_THREAD_LOCAL),
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
            if (existing_overload_set || can_overload_existing_symbol) {
                if (!st_maybe_attach_to_overload_set(table,
                                                     table->root_scope,
                                                     binding_decl->name,
                                                     symbol,
                                                     existing_overload_set
                                                         ? NULL
                                                         : conflicting_symbol)) {
                    return false;
                }
            }
        } else if (decl->kind == AST_TOP_LEVEL_UNION) {
            const AstUnionDecl *union_decl = &decl->as.union_decl;
            const OverloadSet *conflicting_overload_set =
                scope_lookup_local_overload_set(table->root_scope, union_decl->name);
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  union_decl->name);
            Symbol *symbol;

            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, union_decl->name);
            }
            if (conflicting_symbol || conflicting_overload_set) {
                return st_report_duplicate(table,
                                           union_decl->name_span,
                                           union_decl->name,
                                           table->root_scope,
                                           conflicting_symbol,
                                           conflicting_overload_set);
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
