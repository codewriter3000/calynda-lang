#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

bool hr_lower_binding_or_start_decl(HirBuildContext *context,
                                    const AstTopLevelDecl *ast_decl) {
    HirTopLevelDecl *hir_decl;

    if (ast_decl->kind == AST_TOP_LEVEL_BINDING) {
        const Symbol *symbol;
        const TypeCheckInfo *info;

        symbol = symbol_table_find_symbol_for_declaration(context->symbols,
                                                          &ast_decl->as.binding_decl);
        if (!symbol) {
            hr_set_error(context,
                         ast_decl->as.binding_decl.name_span,
                         NULL,
                         "Internal error: missing top-level symbol '%s' during HIR lowering.",
                         ast_decl->as.binding_decl.name);
            return false;
        }

        info = type_checker_get_symbol_info(context->checker, symbol);
        if (!info) {
            hr_set_error(context,
                         ast_decl->as.binding_decl.name_span,
                         &symbol->declaration_span,
                         "Internal error: missing type info for top-level binding '%s'.",
                         ast_decl->as.binding_decl.name);
            return false;
        }

        hir_decl = hr_top_level_decl_new(HIR_TOP_LEVEL_BINDING);
        if (!hir_decl) {
            hr_set_error(context,
                         ast_decl->as.binding_decl.name_span,
                         NULL,
                         "Out of memory while lowering HIR top-level bindings.");
            return false;
        }

        hir_decl->as.binding.name = ast_copy_text(ast_decl->as.binding_decl.name);
        hir_decl->as.binding.symbol = symbol;
        hir_decl->as.binding.type = info->type;
        hir_decl->as.binding.is_final = symbol->is_final;
        hir_decl->as.binding.is_exported = symbol->is_exported;
        hir_decl->as.binding.is_static = symbol->is_static;
        hir_decl->as.binding.is_internal = symbol->is_internal;
        hir_decl->as.binding.is_callable = info->is_callable;
        hir_decl->as.binding.source_span = ast_decl->as.binding_decl.name_span;
        if (info->is_callable &&
            !hr_lower_callable_signature(context, info, &hir_decl->as.binding.callable_signature)) {
            free(hir_decl->as.binding.name);
            free(hir_decl);
            return false;
        }
        hir_decl->as.binding.initializer = hr_lower_expression(context,
                                                               ast_decl->as.binding_decl.initializer);
        if (!hir_decl->as.binding.name || !hir_decl->as.binding.initializer) {
            if (!context->program->has_error) {
                hr_set_error(context,
                             ast_decl->as.binding_decl.name_span,
                             NULL,
                             "Out of memory while lowering HIR top-level bindings.");
            }
            free(hir_decl->as.binding.name);
            hr_free_callable_signature(&hir_decl->as.binding.callable_signature);
            hir_expression_free(hir_decl->as.binding.initializer);
            free(hir_decl);
            return false;
        }
    } else {
        const Scope *start_scope = symbol_table_find_scope(context->symbols,
                                                           &ast_decl->as.start_decl,
                                                           SCOPE_KIND_START);

        hir_decl = hr_top_level_decl_new(HIR_TOP_LEVEL_START);
        if (!hir_decl) {
            hr_set_error(context,
                         ast_decl->as.start_decl.start_span,
                         NULL,
                         "Out of memory while lowering HIR start declaration.");
            return false;
        }

        if (!start_scope) {
            free(hir_decl);
            hr_set_error(context,
                         ast_decl->as.start_decl.start_span,
                         NULL,
                         "Internal error: missing start scope during HIR lowering.");
            return false;
        }

        hir_decl->as.start.source_span = ast_decl->as.start_decl.start_span;
        hir_decl->as.start.is_boot = ast_decl->as.start_decl.is_boot;
        if (!hr_lower_parameters(context,
                                 &hir_decl->as.start.parameters,
                                 &ast_decl->as.start_decl.parameters,
                                 start_scope)) {
            free(hir_decl);
            return false;
        }
        hir_decl->as.start.body = hr_lower_start_body_to_block(context,
                                                               &ast_decl->as.start_decl.body);
        if (!hir_decl->as.start.body) {
            hr_free_parameter_list(&hir_decl->as.start.parameters);
            free(hir_decl);
            return false;
        }
    }

    if (!hr_append_top_level_decl(context->program, hir_decl)) {
        if (hir_decl->kind == HIR_TOP_LEVEL_BINDING) {
            free(hir_decl->as.binding.name);
            hr_free_callable_signature(&hir_decl->as.binding.callable_signature);
            hir_expression_free(hir_decl->as.binding.initializer);
        } else {
            hr_free_parameter_list(&hir_decl->as.start.parameters);
            hir_block_free(hir_decl->as.start.body);
        }
        free(hir_decl);
        hr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while assembling the HIR program.");
        return false;
    }

    return true;
}
