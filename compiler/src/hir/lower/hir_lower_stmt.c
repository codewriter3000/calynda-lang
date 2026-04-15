#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

HirStatement *hr_lower_statement(HirBuildContext *context,
                                 const AstStatement *statement,
                                 const Scope *scope) {
    HirStatement *hir_statement;
    HirStatementKind hir_kind;

    if (!statement) {
        return NULL;
    }

    hir_kind = (statement->kind == AST_STMT_EXIT) ? HIR_STMT_RETURN
                                                  : (HirStatementKind)statement->kind;
    hir_statement = hr_statement_new(hir_kind);
    if (!hir_statement) {
        hr_set_error(context,
                     statement->source_span,
                     NULL,
                     "Out of memory while lowering HIR statements.");
        return NULL;
    }

    hir_statement->source_span = statement->source_span;
    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        {
            const Symbol *symbol = scope_lookup_local(scope, statement->as.local_binding.name);
            const TypeCheckInfo *info;

            if (!symbol) {
                hr_set_error(context,
                             statement->as.local_binding.name_span,
                             NULL,
                             "Internal error: missing symbol for local '%s'.",
                             statement->as.local_binding.name);
                hir_statement_free(hir_statement);
                return NULL;
            }

            info = type_checker_get_symbol_info(context->checker, symbol);
            if (!info) {
                hr_set_error(context,
                             statement->as.local_binding.name_span,
                             &symbol->declaration_span,
                             "Internal error: missing type info for local '%s'.",
                             statement->as.local_binding.name);
                hir_statement_free(hir_statement);
                return NULL;
            }

            hir_statement->as.local_binding.name = ast_copy_text(statement->as.local_binding.name);
            if (!hir_statement->as.local_binding.name) {
                hr_set_error(context,
                             statement->source_span,
                             NULL,
                             "Out of memory while lowering HIR locals.");
                hir_statement_free(hir_statement);
                return NULL;
            }
            hir_statement->as.local_binding.symbol = symbol;
            hir_statement->as.local_binding.type = info->type;
            hir_statement->as.local_binding.is_callable = info->is_callable;
            hir_statement->as.local_binding.is_final = symbol->is_final;
            hir_statement->as.local_binding.source_span = statement->as.local_binding.name_span;
            if (info->is_callable &&
                !hr_lower_callable_signature(context,
                                             info,
                                             &hir_statement->as.local_binding.callable_signature)) {
                hir_statement_free(hir_statement);
                return NULL;
            }
            hir_statement->as.local_binding.initializer = hr_lower_expression(
                context,
                statement->as.local_binding.initializer);
            if (!hir_statement->as.local_binding.initializer) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_RETURN:
        if (statement->as.return_expression) {
            hir_statement->as.return_expression = hr_lower_expression(context,
                                                                      statement->as.return_expression);
            if (!hir_statement->as.return_expression) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_THROW:
        if (statement->as.throw_expression) {
            hir_statement->as.throw_expression = hr_lower_expression(context,
                                                                     statement->as.throw_expression);
            if (!hir_statement->as.throw_expression) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_EXPRESSION:
        if (statement->as.expression) {
            hir_statement->as.expression = hr_lower_expression(context, statement->as.expression);
            if (!hir_statement->as.expression) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    case AST_STMT_EXIT:
        break;
    case AST_STMT_MANUAL:
        if (statement->as.manual.body) {
            hir_statement->as.manual_body = hr_lower_block(context,
                                                           statement->as.manual.body);
            if (!hir_statement->as.manual_body) {
                hir_statement_free(hir_statement);
                return NULL;
            }
        }
        break;
    }

    return hir_statement;
}
