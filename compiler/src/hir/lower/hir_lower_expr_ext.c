#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

static const AstExpression *hr_strip_grouping_expression(const AstExpression *expression) {
    while (expression && expression->kind == AST_EXPR_GROUPING) {
        expression = expression->as.grouping.inner;
    }
    return expression;
}

static const Scope *hr_find_inline_parameter_scope(HirBuildContext *context,
                                                   const AstExpression *callee) {
    const AstExpression *stripped = hr_strip_grouping_expression(callee);

    if (!context || !stripped) {
        return NULL;
    }

    if (stripped->kind == AST_EXPR_LAMBDA) {
        return symbol_table_find_scope(context->symbols, stripped, SCOPE_KIND_LAMBDA);
    }

    if (stripped->kind == AST_EXPR_IDENTIFIER) {
        const Symbol *symbol = symbol_table_resolve_identifier(context->symbols, stripped);

        if (!symbol || !symbol->declaration) {
            return NULL;
        }

        if (symbol->kind == SYMBOL_KIND_TOP_LEVEL_BINDING) {
            const AstBindingDecl *binding = (const AstBindingDecl *)symbol->declaration;

            if (binding->initializer && binding->initializer->kind == AST_EXPR_LAMBDA) {
                return symbol_table_find_scope(context->symbols,
                                               binding->initializer,
                                               SCOPE_KIND_LAMBDA);
            }
        } else if (symbol->kind == SYMBOL_KIND_LOCAL) {
            const AstLocalBindingStatement *binding =
                (const AstLocalBindingStatement *)symbol->declaration;

            if (binding->initializer && binding->initializer->kind == AST_EXPR_LAMBDA) {
                return symbol_table_find_scope(context->symbols,
                                               binding->initializer,
                                               SCOPE_KIND_LAMBDA);
            }
        }
    }

    return NULL;
}

static const Symbol **hr_build_inline_parameter_symbols(HirBuildContext *context,
                                                        const AstExpression *callee,
                                                        const AstParameterList *parameters) {
    const Scope *scope;
    const Symbol **symbols;
    size_t i;

    if (!parameters || parameters->count == 0) {
        return NULL;
    }

    symbols = calloc(parameters->count, sizeof(*symbols));
    if (!symbols) {
        hr_set_error(context,
                     callee ? callee->source_span : (AstSourceSpan){0},
                     NULL,
                     "Out of memory while lowering optional call arguments.");
        return NULL;
    }

    scope = hr_find_inline_parameter_scope(context, callee);
    if (!scope) {
        return symbols;
    }

    for (i = 0; i < parameters->count; i++) {
        symbols[i] = scope_lookup_local(scope, parameters->items[i].name);
    }

    return symbols;
}

static bool hr_find_inline_parameter_index(const HirBuildContext *context,
                                           const Symbol *symbol,
                                           size_t *out_index) {
    size_t i;

    if (!context || !symbol || !out_index || !context->inline_parameter_symbols) {
        return false;
    }

    for (i = 0; i < context->inline_resolved_count; i++) {
        if (context->inline_parameter_symbols[i] == symbol &&
            context->inline_argument_sources &&
            context->inline_argument_sources[i] != NULL) {
            *out_index = i;
            return true;
        }
    }

    return false;
}

static HirExpression *hr_lower_optional_argument(HirBuildContext *context,
                                                 const AstExpression *expression,
                                                 const AstParameterList *parameters,
                                                 const Symbol **parameter_symbols,
                                                 const AstExpression *const *argument_sources,
                                                 size_t resolved_count) {
    const AstParameterList *saved_parameters = context->inline_parameters;
    const Symbol **saved_symbols = context->inline_parameter_symbols;
    const AstExpression *const *saved_sources = context->inline_argument_sources;
    size_t saved_count = context->inline_resolved_count;
    HirExpression *result;

    context->inline_parameters = parameters;
    context->inline_parameter_symbols = parameter_symbols;
    context->inline_argument_sources = argument_sources;
    context->inline_resolved_count = resolved_count;
    result = hr_lower_expression(context, expression);
    context->inline_parameters = saved_parameters;
    context->inline_parameter_symbols = saved_symbols;
    context->inline_argument_sources = saved_sources;
    context->inline_resolved_count = saved_count;
    return result;
}

HirExpression *hr_lower_expr_complex(HirBuildContext *context,
                                     const AstExpression *expression,
                                     HirExpression *hir_expression,
                                     const TypeCheckInfo *info) {
    size_t i;

    (void)info;

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                HirTemplatePart part;

                memset(&part, 0, sizeof(part));
                part.kind = expression->as.literal.as.template_parts.items[i].kind;
                if (part.kind == AST_TEMPLATE_PART_TEXT) {
                    part.as.text = ast_copy_text(
                        expression->as.literal.as.template_parts.items[i].as.text);
                    if (!part.as.text) {
                        hir_expression_free(hir_expression);
                        hr_set_error(context,
                                     expression->source_span,
                                     NULL,
                                     "Out of memory while lowering HIR templates.");
                        return NULL;
                    }
                } else {
                    part.as.expression = hr_lower_expression(
                        context,
                        expression->as.literal.as.template_parts.items[i].as.expression);
                    if (!part.as.expression) {
                        hir_expression_free(hir_expression);
                        return NULL;
                    }
                }

                if (!hr_append_template_part(&hir_expression->as.template_parts, part)) {
                    if (part.kind == AST_TEMPLATE_PART_TEXT) {
                        free(part.as.text);
                    } else {
                        hir_expression_free(part.as.expression);
                    }
                    hir_expression_free(hir_expression);
                    hr_set_error(context,
                                 expression->source_span,
                                 NULL,
                                 "Out of memory while lowering HIR templates.");
                    return NULL;
                }
            }
            return hir_expression;
        }

        hir_expression->as.literal.kind = expression->as.literal.kind;
        if (expression->as.literal.kind == AST_LITERAL_BOOL) {
            hir_expression->as.literal.as.bool_value = expression->as.literal.as.bool_value;
        } else if (expression->as.literal.kind != AST_LITERAL_NULL) {
            hir_expression->as.literal.as.text = ast_copy_text(expression->as.literal.as.text);
            if (!hir_expression->as.literal.as.text) {
                hir_expression_free(hir_expression);
                hr_set_error(context,
                             expression->source_span,
                             NULL,
                             "Out of memory while lowering HIR literals.");
                return NULL;
            }
        }
        return hir_expression;

    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *symbol = symbol_table_resolve_identifier(context->symbols, expression);
            size_t inline_parameter_index;

            if (symbol &&
                hr_find_inline_parameter_index(context,
                                               symbol,
                                               &inline_parameter_index)) {
                hir_expression_free(hir_expression);
                return hr_lower_optional_argument(context,
                                                  context->inline_argument_sources[inline_parameter_index],
                                                  context->inline_parameters,
                                                  context->inline_parameter_symbols,
                                                  context->inline_argument_sources,
                                                  inline_parameter_index);
            }

            if (!symbol) {
                hir_expression_free(hir_expression);
                hr_set_error(context,
                             expression->source_span,
                             NULL,
                             "Internal error: unresolved identifier '%s' during HIR lowering.",
                             expression->as.identifier ? expression->as.identifier : "<unknown>");
                return NULL;
            }

            hir_expression->as.symbol.symbol = symbol;
            hir_expression->as.symbol.name = ast_copy_text(expression->as.identifier);
            hir_expression->as.symbol.kind = symbol->kind;
            hir_expression->as.symbol.type = info->type;
            hir_expression->as.symbol.source_span = expression->source_span;
            if (!hir_expression->as.symbol.name) {
                hir_expression_free(hir_expression);
                hr_set_error(context,
                             expression->source_span,
                             NULL,
                             "Out of memory while lowering HIR symbols.");
                return NULL;
            }
        }
        return hir_expression;

#include "hir_lower_expr_ext_p2.inc"
