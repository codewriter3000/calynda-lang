#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

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

    case AST_EXPR_LAMBDA:
        {
            const Scope *scope = symbol_table_find_scope(context->symbols,
                                                         expression,
                                                         SCOPE_KIND_LAMBDA);

            if (!scope) {
                hir_expression_free(hir_expression);
                hr_set_error(context,
                             expression->source_span,
                             NULL,
                             "Internal error: missing lambda scope during HIR lowering.");
                return NULL;
            }

            if (!hr_lower_parameters(context,
                                     &hir_expression->as.lambda.parameters,
                                     &expression->as.lambda.parameters,
                                     scope)) {
                hir_expression_free(hir_expression);
                return NULL;
            }
            hir_expression->as.lambda.body = hr_lower_body_to_block(context,
                                                                    &expression->as.lambda.body);
            if (!hir_expression->as.lambda.body) {
                hir_expression_free(hir_expression);
                return NULL;
            }
        }
        return hir_expression;

    case AST_EXPR_CALL:
        hir_expression->as.call.callee = hr_lower_expression(context, expression->as.call.callee);
        if (!hir_expression->as.call.callee) {
            hir_expression_free(hir_expression);
            return NULL;
        }
        for (i = 0; i < expression->as.call.arguments.count; i++) {
            HirExpression *argument = hr_lower_expression(context,
                                                          expression->as.call.arguments.items[i]);

            if (!argument) {
                hir_expression_free(hir_expression);
                return NULL;
            }
            if (!hr_append_argument(&hir_expression->as.call, argument)) {
                hir_expression_free(argument);
                hir_expression_free(hir_expression);
                hr_set_error(context,
                             expression->source_span,
                             NULL,
                             "Out of memory while lowering HIR calls.");
                return NULL;
            }
        }
        return hir_expression;

    case AST_EXPR_MEMBER:
        hir_expression->as.member.target = hr_lower_expression(context,
                                                               expression->as.member.target);
        hir_expression->as.member.member = ast_copy_text(expression->as.member.member);
        if (!hir_expression->as.member.member) {
            hir_expression_free(hir_expression);
            hr_set_error(context,
                         expression->source_span,
                         NULL,
                         "Out of memory while lowering HIR member access.");
            return NULL;
        }
        return hir_expression;

    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            HirExpression *element = hr_lower_expression(context,
                                                         expression->as.array_literal.elements.items[i]);

            if (!element) {
                hir_expression_free(hir_expression);
                return NULL;
            }
            if (!hr_append_array_element(&hir_expression->as.array_literal, element)) {
                hir_expression_free(element);
                hir_expression_free(hir_expression);
                hr_set_error(context,
                             expression->source_span,
                             NULL,
                             "Out of memory while lowering HIR array literals.");
                return NULL;
            }
        }
        return hir_expression;

    default:
        hir_expression_free(hir_expression);
        return NULL;
    }
}
