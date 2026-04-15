#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

bool hr_lower_callable_signature(HirBuildContext *context,
                                 const TypeCheckInfo *info,
                                 HirCallableSignature *signature) {
    size_t i;

    if (!signature) {
        return false;
    }

    memset(signature, 0, sizeof(*signature));
    if (!info || !info->is_callable) {
        return true;
    }

    signature->return_type = info->callable_return_type;
    if (!info->parameters) {
        return true;
    }

    signature->has_parameter_types = true;
    signature->parameter_count = info->parameters->count;
    if (signature->parameter_count == 0) {
        return true;
    }

    signature->parameter_types = calloc(signature->parameter_count,
                                        sizeof(*signature->parameter_types));
    if (!signature->parameter_types) {
        hr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while lowering HIR callable signatures.");
        return false;
    }

    for (i = 0; i < info->parameters->count; i++) {
        const ResolvedType *resolved = type_resolver_get_type(&context->checker->resolver,
                                                              &info->parameters->items[i].type);

        if (!resolved) {
            hr_set_error(context,
                         info->parameters->items[i].name_span,
                         NULL,
                         "Internal error: missing resolved type for callable parameter '%s'.",
                         info->parameters->items[i].name);
            return false;
        }

        signature->parameter_types[i] = hr_checked_type_from_resolved_type(*resolved);
    }

    return true;
}

bool hr_lower_parameters(HirBuildContext *context,
                         HirParameterList *out,
                         const AstParameterList *parameters,
                         const Scope *scope) {
    size_t i;

    if (!parameters || !scope) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    for (i = 0; i < parameters->count; i++) {
        const Symbol *symbol = scope_lookup_local(scope, parameters->items[i].name);
        const TypeCheckInfo *info;
        HirParameter parameter;

        if (!symbol) {
            hr_set_error(context,
                         parameters->items[i].name_span,
                         NULL,
                         "Internal error: missing symbol for parameter '%s'.",
                         parameters->items[i].name);
            return false;
        }

        info = type_checker_get_symbol_info(context->checker, symbol);
        if (!info) {
            hr_set_error(context,
                         parameters->items[i].name_span,
                         &symbol->declaration_span,
                         "Internal error: missing type info for parameter '%s'.",
                         parameters->items[i].name);
            return false;
        }

        memset(&parameter, 0, sizeof(parameter));
        parameter.name = ast_copy_text(parameters->items[i].name);
        if (!parameter.name) {
            hr_set_error(context,
                         parameters->items[i].name_span,
                         NULL,
                         "Out of memory while lowering HIR parameters.");
            return false;
        }
        parameter.symbol = symbol;
        parameter.type = info->type;
        parameter.is_varargs = parameters->items[i].is_varargs;
        parameter.source_span = parameters->items[i].name_span;

        if (!hr_append_parameter(out, parameter)) {
            free(parameter.name);
            hr_set_error(context,
                         parameters->items[i].name_span,
                         NULL,
                         "Out of memory while lowering HIR parameters.");
            return false;
        }
    }

    return true;
}

HirBlock *hr_lower_body_to_block(HirBuildContext *context,
                                 const AstLambdaBody *body) {
    HirBlock *block;

    if (!body) {
        return NULL;
    }

    if (body->kind == AST_LAMBDA_BODY_BLOCK) {
        return hr_lower_block(context, body->as.block);
    }

    block = hr_block_new();
    if (!block) {
        hr_set_error(context,
                     body->as.expression ? body->as.expression->source_span : (AstSourceSpan){0},
                     NULL,
                     "Out of memory while lowering HIR blocks.");
        return NULL;
    }

    if (body->as.expression) {
        HirStatement *statement = hr_statement_new(HIR_STMT_RETURN);

        if (!statement) {
            hir_block_free(block);
            hr_set_error(context,
                         body->as.expression->source_span,
                         NULL,
                         "Out of memory while lowering HIR statements.");
            return NULL;
        }

        statement->source_span = body->as.expression->source_span;
        statement->as.return_expression = hr_lower_expression(context, body->as.expression);
        if (!statement->as.return_expression || !hr_append_statement(block, statement)) {
            hir_statement_free(statement);
            hir_block_free(block);
            if (!context->program->has_error) {
                hr_set_error(context,
                             body->as.expression->source_span,
                             NULL,
                             "Out of memory while lowering HIR statements.");
            }
            return NULL;
        }
    }

    return block;
}

HirBlock *hr_lower_block(HirBuildContext *context, const AstBlock *block) {
    const Scope *scope;
    HirBlock *hir_block;
    size_t i;

    if (!block) {
        return NULL;
    }

    scope = symbol_table_find_scope(context->symbols, block, SCOPE_KIND_BLOCK);
    if (!scope) {
        hr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Internal error: missing scope for block during HIR lowering.");
        return NULL;
    }

    hir_block = hr_block_new();
    if (!hir_block) {
        hr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while lowering HIR blocks.");
        return NULL;
    }

    for (i = 0; i < block->statement_count; i++) {
        HirStatement *statement = hr_lower_statement(context, block->statements[i], scope);

        if (!statement) {
            hir_block_free(hir_block);
            return NULL;
        }
        if (!hr_append_statement(hir_block, statement)) {
            hir_statement_free(statement);
            hir_block_free(hir_block);
            hr_set_error(context,
                         block->statements[i]->source_span,
                         NULL,
                         "Out of memory while lowering HIR statements.");
            return NULL;
        }
    }

    return hir_block;
}
