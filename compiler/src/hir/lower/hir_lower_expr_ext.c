#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

static const AstExpression *hr_strip_grouping_expression(const AstExpression *expression) {
    while (expression && expression->kind == AST_EXPR_GROUPING) {
        expression = expression->as.grouping.inner;
    }
    return expression;
}

static CheckedType hr_type_check_source_type(const TypeCheckInfo *info) {
    if (!info) {
        return (CheckedType){ CHECKED_TYPE_INVALID, 0, 0, NULL, NULL, 0, false };
    }

    return info->is_callable ? info->callable_return_type : info->type;
}

static bool hr_checked_type_is_string(CheckedType type) {
    return type.kind == CHECKED_TYPE_VALUE &&
           type.array_depth == 0 &&
           type.primitive == AST_PRIMITIVE_STRING;
}

static HirExpression *hr_make_template_string_literal(HirBuildContext *context,
                                                      const AstExpression *expression) {
    const AstTemplatePartList *parts;
    size_t i;
    size_t total_length = 0;
    char *joined;
    char *cursor;
    HirExpression *literal_expr;

    if (!context || !expression || expression->kind != AST_EXPR_LITERAL ||
        expression->as.literal.kind != AST_LITERAL_TEMPLATE) {
        return NULL;
    }

    parts = &expression->as.literal.as.template_parts;
    for (i = 0; i < parts->count; i++) {
        const char *text;

        if (parts->items[i].kind != AST_TEMPLATE_PART_TEXT) {
            return NULL;
        }

        text = parts->items[i].as.text ? parts->items[i].as.text : "";
        total_length += strlen(text);
    }

    joined = malloc(total_length + 3);
    literal_expr = hr_expression_new(HIR_EXPR_LITERAL);
    if (!joined || !literal_expr) {
        free(joined);
        hir_expression_free(literal_expr);
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering template literal text.");
        return NULL;
    }

    cursor = joined;
    *cursor++ = '"';
    for (i = 0; i < parts->count; i++) {
        const char *text = parts->items[i].as.text ? parts->items[i].as.text : "";
        size_t text_length = strlen(text);

        if (text_length > 0) {
            memcpy(cursor, text, text_length);
            cursor += text_length;
        }
    }
    *cursor++ = '"';
    *cursor = '\0';

    literal_expr->type = (CheckedType){CHECKED_TYPE_VALUE,
                                       AST_PRIMITIVE_STRING,
                                       0,
                                       NULL,
                                       NULL,
                                       0,
                                       false};
    literal_expr->source_span = expression->source_span;
    literal_expr->as.literal.kind = AST_LITERAL_STRING;
    literal_expr->as.literal.as.text = joined;
    return literal_expr;
}

static const AstExpression *hr_find_template_single_expression(HirBuildContext *context,
                                                               const AstExpression *expression,
                                                               const TypeCheckInfo **expression_info_out,
                                                               bool *has_non_empty_text_out) {
    const AstTemplatePartList *parts;
    const AstExpression *value_expression = NULL;
    const TypeCheckInfo *value_info = NULL;
    bool has_non_empty_text = false;
    size_t i;

    if (expression_info_out) {
        *expression_info_out = NULL;
    }
    if (has_non_empty_text_out) {
        *has_non_empty_text_out = false;
    }
    if (!context || !expression || expression->kind != AST_EXPR_LITERAL ||
        expression->as.literal.kind != AST_LITERAL_TEMPLATE) {
        return NULL;
    }

    parts = &expression->as.literal.as.template_parts;
    for (i = 0; i < parts->count; i++) {
        if (parts->items[i].kind == AST_TEMPLATE_PART_TEXT) {
            const char *text = parts->items[i].as.text ? parts->items[i].as.text : "";

            if (text[0] != '\0') {
                has_non_empty_text = true;
            }
            continue;
        }

        if (value_expression != NULL) {
            return NULL;
        }

        value_expression = parts->items[i].as.expression;
    }

    if (has_non_empty_text_out) {
        *has_non_empty_text_out = has_non_empty_text;
    }

    if (!value_expression || has_non_empty_text) {
        return NULL;
    }

    value_info = type_checker_get_expression_info(context->checker, value_expression);
    if (!value_info ||
        (value_info->is_callable && value_info->parameters && value_info->parameters->count == 0)) {
        return NULL;
    }

    if (expression_info_out) {
        *expression_info_out = value_info;
    }

    return value_expression;
}

static HirExpression *hr_make_string_cast_expression(HirBuildContext *context,
                                                     const AstExpression *expression,
                                                     HirExpression *inner) {
    HirExpression *cast_expr;
    CheckedType string_type;

    if (!context || !expression || !inner) {
        return NULL;
    }

    string_type = (CheckedType){CHECKED_TYPE_VALUE,
                                AST_PRIMITIVE_STRING,
                                0,
                                NULL,
                                NULL,
                                0,
                                false};
    cast_expr = hr_expression_new(HIR_EXPR_CAST);
    if (!cast_expr) {
        hir_expression_free(inner);
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering fast template cast.");
        return NULL;
    }

    cast_expr->type = string_type;
    cast_expr->source_span = expression->source_span;
    cast_expr->as.cast.target_type = string_type;
    cast_expr->as.cast.expression = inner;
    return cast_expr;
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
            const TypeCheckInfo *single_expression_info = NULL;
            const AstExpression *single_expression;
            bool has_non_empty_text = false;
            bool has_value_part = false;

            single_expression = hr_find_template_single_expression(context,
                                                                   expression,
                                                                   &single_expression_info,
                                                                   &has_non_empty_text);
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                if (expression->as.literal.as.template_parts.items[i].kind ==
                    AST_TEMPLATE_PART_EXPRESSION) {
                    has_value_part = true;
                    break;
                }
            }

            if (!has_value_part) {
                hir_expression_free(hir_expression);
                return hr_make_template_string_literal(context, expression);
            }

            if (single_expression && !context->current_boot_context &&
                !context->current_manual_context &&
                !context->current_size_focus) {
                HirExpression *lowered_value = hr_lower_expression(context,
                                                                   single_expression);

                hir_expression_free(hir_expression);
                if (!lowered_value) {
                    return NULL;
                }

                if (single_expression_info &&
                    hr_checked_type_is_string(
                        hr_type_check_source_type(single_expression_info))) {
                    return lowered_value;
                }

                return hr_make_string_cast_expression(context,
                                                      expression,
                                                      lowered_value);
            }

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
