#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

bool hr_lower_top_level_decls(HirBuildContext *context) {
    size_t i;

    for (i = 0; i < context->ast_program->top_level_count; i++) {
        const AstTopLevelDecl *ast_decl = context->ast_program->top_level_decls[i];
        HirTopLevelDecl *hir_decl;

        /* Lower union declarations into HIR type metadata */
        if (ast_decl->kind == AST_TOP_LEVEL_UNION) {
            const AstUnionDecl *udecl = &ast_decl->as.union_decl;
            const Scope *root_scope = symbol_table_root_scope(context->symbols);
            const Symbol *symbol;
            size_t v;

            symbol = scope_lookup_local(root_scope, udecl->name);

            hir_decl = hr_top_level_decl_new(HIR_TOP_LEVEL_UNION);
            if (!hir_decl) {
                hr_set_error(context,
                             udecl->name_span,
                             NULL,
                             "Out of memory while lowering HIR union declaration.");
                return false;
            }

            hir_decl->as.union_decl.name = ast_copy_text(udecl->name);
            hir_decl->as.union_decl.symbol = symbol;
            hir_decl->as.union_decl.source_span = udecl->name_span;
            hir_decl->as.union_decl.is_exported = false;
            hir_decl->as.union_decl.is_static = false;
            hir_decl->as.union_decl.generic_param_count = udecl->generic_param_count;
            hir_decl->as.union_decl.generic_params = NULL;
            hir_decl->as.union_decl.variant_count = udecl->variant_count;
            hir_decl->as.union_decl.variants = NULL;

            if (symbol) {
                hir_decl->as.union_decl.is_exported = symbol->is_exported;
                hir_decl->as.union_decl.is_static = symbol->is_static;
            }

            if (udecl->generic_param_count > 0) {
                hir_decl->as.union_decl.generic_params = calloc(udecl->generic_param_count,
                                                                sizeof(char *));
                if (!hir_decl->as.union_decl.generic_params) {
                    free(hir_decl->as.union_decl.name);
                    free(hir_decl);
                    hr_set_error(context, udecl->name_span, NULL,
                                 "Out of memory while lowering union generic params.");
                    return false;
                }
                for (v = 0; v < udecl->generic_param_count; v++) {
                    hir_decl->as.union_decl.generic_params[v] = ast_copy_text(udecl->generic_params[v]);
                }
            }

            if (udecl->variant_count > 0) {
                hir_decl->as.union_decl.variants = calloc(udecl->variant_count,
                                                          sizeof(HirUnionVariant));
                if (!hir_decl->as.union_decl.variants) {
                    free(hir_decl->as.union_decl.generic_params);
                    free(hir_decl->as.union_decl.name);
                    free(hir_decl);
                    hr_set_error(context, udecl->name_span, NULL,
                                 "Out of memory while lowering union variants.");
                    return false;
                }
                for (v = 0; v < udecl->variant_count; v++) {
                    hir_decl->as.union_decl.variants[v].name =
                        ast_copy_text(udecl->variants[v].name);
                    hir_decl->as.union_decl.variants[v].has_payload =
                        udecl->variants[v].payload_type != NULL;
                    if (udecl->variants[v].payload_type) {
                        const TypeCheckInfo *tc_info = type_checker_get_symbol_info(context->checker, symbol);
                        hir_decl->as.union_decl.variants[v].payload_type =
                            tc_info ? tc_info->type : (CheckedType){0};
                    } else {
                        memset(&hir_decl->as.union_decl.variants[v].payload_type, 0,
                               sizeof(CheckedType));
                    }
                }
            }

            if (!hir_decl->as.union_decl.name) {
                free(hir_decl);
                hr_set_error(context, udecl->name_span, NULL,
                             "Out of memory while lowering union name.");
                return false;
            }

            if (!hr_append_top_level_decl(context->program, hir_decl)) {
                free(hir_decl);
                hr_set_error(context, udecl->name_span, NULL,
                             "Out of memory while adding union to HIR program.");
                return false;
            }
            continue;
        }

        /* Lower asm declarations into HIR opaque units */
        if (ast_decl->kind == AST_TOP_LEVEL_ASM) {
            const AstAsmDecl *adecl = &ast_decl->as.asm_decl;
            const Scope *root_scope = symbol_table_root_scope(context->symbols);
            const Symbol *symbol = scope_lookup_local(root_scope, adecl->name);
            const TypeCheckInfo *tc_info = symbol ? type_checker_get_symbol_info(context->checker, symbol) : NULL;
            size_t p;

            hir_decl = hr_top_level_decl_new(HIR_TOP_LEVEL_ASM);
            if (!hir_decl) {
                hr_set_error(context, adecl->name_span, NULL,
                             "Out of memory while lowering HIR asm declaration.");
                return false;
            }

            hir_decl->as.asm_decl.name = ast_copy_text(adecl->name);
            hir_decl->as.asm_decl.symbol = symbol;
            hir_decl->as.asm_decl.source_span = adecl->name_span;
            hir_decl->as.asm_decl.is_exported = symbol ? symbol->is_exported : false;
            hir_decl->as.asm_decl.is_static = symbol ? symbol->is_static : false;
            hir_decl->as.asm_decl.is_internal = symbol ? symbol->is_internal : false;
            hir_decl->as.asm_decl.return_type = tc_info ? tc_info->callable_return_type : (CheckedType){0};
            hir_decl->as.asm_decl.parameter_count = adecl->parameters.count;
            hir_decl->as.asm_decl.parameter_types = NULL;
            hir_decl->as.asm_decl.parameter_names = NULL;
            hir_decl->as.asm_decl.body = ast_copy_text_n(adecl->body, adecl->body_length);
            hir_decl->as.asm_decl.body_length = adecl->body_length;

            if (adecl->parameters.count > 0) {
                hir_decl->as.asm_decl.parameter_names = calloc(adecl->parameters.count,
                                                               sizeof(char *));
                if (!hir_decl->as.asm_decl.parameter_names) {
                    free(hir_decl->as.asm_decl.name);
                    free(hir_decl->as.asm_decl.body);
                    free(hir_decl);
                    hr_set_error(context, adecl->name_span, NULL,
                                 "Out of memory while lowering asm parameters.");
                    return false;
                }
                for (p = 0; p < adecl->parameters.count; p++) {
                    hir_decl->as.asm_decl.parameter_names[p] =
                        ast_copy_text(adecl->parameters.items[p].name);
                }
            }

            if (!hir_decl->as.asm_decl.name ||
                (adecl->body_length > 0 && !hir_decl->as.asm_decl.body)) {
                free(hir_decl->as.asm_decl.name);
                free(hir_decl->as.asm_decl.body);
                free(hir_decl);
                hr_set_error(context, adecl->name_span, NULL,
                             "Out of memory while lowering asm declaration name/body.");
                return false;
            }

            if (!hr_append_top_level_decl(context->program, hir_decl)) {
                free(hir_decl->as.asm_decl.name);
                free(hir_decl->as.asm_decl.body);
                free(hir_decl);
                hr_set_error(context, adecl->name_span, NULL,
                             "Out of memory while adding asm to HIR program.");
                return false;
            }
            continue;
        }

        if (ast_decl->kind == AST_TOP_LEVEL_BINDING) {
            const Scope *root_scope = symbol_table_root_scope(context->symbols);
            const Symbol *symbol;
            const TypeCheckInfo *info;

            symbol = scope_lookup_local(root_scope, ast_decl->as.binding_decl.name);
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
            hir_decl->as.start.body = hr_lower_body_to_block(context,
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
    }

    return true;
}
