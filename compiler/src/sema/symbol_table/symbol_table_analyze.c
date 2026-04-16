#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdlib.h>

bool st_analyze_top_level_decl(SymbolTable *table,
                               const AstTopLevelDecl *decl,
                               Scope *scope) {
    switch (decl->kind) {
    case AST_TOP_LEVEL_START:
        return st_analyze_start_decl(table, &decl->as.start_decl, scope);
    case AST_TOP_LEVEL_BINDING:
        return st_analyze_expression(table, decl->as.binding_decl.initializer, scope);

    case AST_TOP_LEVEL_UNION:
        return st_analyze_union_decl(table, &decl->as.union_decl, scope);

    case AST_TOP_LEVEL_ASM:
        /* Asm declarations have opaque bodies — nothing to analyze. */
        return true;

    case AST_TOP_LEVEL_LAYOUT:
        /* Layout declarations have no expressions to walk. */
        return true;
    }

    return false;
}

bool st_analyze_start_decl(SymbolTable *table, const AstStartDecl *start_decl,
                           Scope *parent_scope) {
    Scope *start_scope = st_scope_new(table, SCOPE_KIND_START, start_decl, parent_scope);

    if (!start_scope) {
        return false;
    }

    if (!st_add_parameter_symbols(table, &start_decl->parameters, start_scope)) {
        return false;
    }

    if (start_decl->body.kind == AST_LAMBDA_BODY_BLOCK) {
        return st_analyze_block(table, start_decl->body.as.block, start_scope);
    }

    return st_analyze_expression(table, start_decl->body.as.expression, start_scope);
}

bool st_analyze_union_decl(SymbolTable *table, const AstUnionDecl *union_decl,
                           Scope *parent_scope) {
    size_t i;
    Scope *union_scope = st_scope_new(table, SCOPE_KIND_UNION,
                                      union_decl, parent_scope);
    if (!union_scope) {
        return false;
    }

    for (i = 0; i < union_decl->generic_param_count; i++) {
        Symbol *type_param = st_symbol_new(table, SYMBOL_KIND_TYPE_PARAMETER,
                                           union_decl->generic_params[i], NULL,
                                           NULL, false, false, false, false, false,
                                           union_decl->name_span,
                                           union_decl,
                                           union_scope);
        if (!type_param) {
            return false;
        }

        if (!st_scope_append_symbol(table, union_scope, type_param)) {
            st_symbol_free(type_param);
            return false;
        }
    }

    for (i = 0; i < union_decl->variant_count; i++) {
        Symbol *variant_sym = st_symbol_new(table, SYMBOL_KIND_VARIANT,
                                            union_decl->variants[i].name, NULL,
                                            union_decl->variants[i].payload_type,
                                            false, false, false, false, false,
                                            union_decl->name_span,
                                            union_decl,
                                            union_scope);
        if (!variant_sym) {
            return false;
        }
        variant_sym->variant_index = i;
        variant_sym->variant_has_payload = (union_decl->variants[i].payload_type != NULL);
        variant_sym->variant_payload_type = union_decl->variants[i].payload_type;

        if (!st_scope_append_symbol(table, union_scope, variant_sym)) {
            st_symbol_free(variant_sym);
            return false;
        }
    }

    return true;
}

bool st_analyze_lambda_expression(SymbolTable *table,
                                  const AstExpression *lambda_expression,
                                  Scope *parent_scope) {
    Scope *lambda_scope = st_scope_new(table, SCOPE_KIND_LAMBDA,
                                       lambda_expression, parent_scope);

    if (!lambda_scope) {
        return false;
    }

    if (!st_add_parameter_symbols(table, &lambda_expression->as.lambda.parameters,
                                  lambda_scope)) {
        return false;
    }

    if (lambda_expression->as.lambda.body.kind == AST_LAMBDA_BODY_BLOCK) {
        return st_analyze_block(table, lambda_expression->as.lambda.body.as.block,
                                lambda_scope);
    }

    return st_analyze_expression(table, lambda_expression->as.lambda.body.as.expression,
                                 lambda_scope);
}

bool st_analyze_block(SymbolTable *table, const AstBlock *block,
                      Scope *parent_scope) {
    Scope *block_scope = st_scope_new(table, SCOPE_KIND_BLOCK, block, parent_scope);
    size_t i;

    if (!block_scope) {
        return false;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (!st_analyze_statement(table, block->statements[i], block_scope)) {
            return false;
        }
    }

    return true;
}

bool st_analyze_statement(SymbolTable *table, const AstStatement *statement,
                          Scope *scope) {
    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        if (!st_analyze_expression(table, statement->as.local_binding.initializer, scope)) {
            return false;
        }
        return st_add_local_symbol(table, &statement->as.local_binding, scope);

    case AST_STMT_RETURN:
        if (!statement->as.return_expression) {
            return true;
        }
        return st_analyze_expression(table, statement->as.return_expression, scope);

    case AST_STMT_EXIT:
        return true;

    case AST_STMT_THROW:
        return st_analyze_expression(table, statement->as.throw_expression, scope);

    case AST_STMT_EXPRESSION:
        return st_analyze_expression(table, statement->as.expression, scope);

    case AST_STMT_MANUAL:
        if (statement->as.manual.body) {
            return st_analyze_block(table, statement->as.manual.body, scope);
        }
        return true;
    }

    return false;
}
