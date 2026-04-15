#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

/* hir_lower_decls_bind.c */
bool hr_lower_binding_or_start_decl(HirBuildContext *context,
                                    const AstTopLevelDecl *ast_decl);

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

        if (!hr_lower_binding_or_start_decl(context, ast_decl)) {
            return false;
        }
    }

    return true;
}
