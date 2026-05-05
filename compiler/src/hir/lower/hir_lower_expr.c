#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

static bool hr_init_helper_callable(HirBuildContext *context,
                                    HirExpression *symbol_expr,
                                    const char *name,
                                    CheckedType return_type,
                                    const CheckedType *parameter_types,
                                    size_t parameter_count,
                                    AstSourceSpan source_span) {
    size_t i;

    (void)context;

    if (!symbol_expr || !name) {
        return false;
    }

    symbol_expr->kind = HIR_EXPR_SYMBOL;
    symbol_expr->type = return_type;
    symbol_expr->is_callable = true;
    symbol_expr->source_span = source_span;
    symbol_expr->as.symbol.name = ast_copy_text(name);
    symbol_expr->as.symbol.kind = SYMBOL_KIND_IMPORT;
    symbol_expr->as.symbol.type = return_type;
    symbol_expr->as.symbol.source_span = source_span;
    symbol_expr->callable_signature.return_type = return_type;
    symbol_expr->callable_signature.parameter_count = parameter_count;
    symbol_expr->callable_signature.has_parameter_types = true;

    if (!symbol_expr->as.symbol.name) {
        return false;
    }

    if (parameter_count == 0) {
        return true;
    }

    symbol_expr->callable_signature.parameter_types =
        calloc(parameter_count, sizeof(*symbol_expr->callable_signature.parameter_types));
    if (!symbol_expr->callable_signature.parameter_types) {
        return false;
    }

    for (i = 0; i < parameter_count; i++) {
        symbol_expr->callable_signature.parameter_types[i] = parameter_types[i];
    }

    return true;
}

static HirExpression *hr_make_helper_symbol(HirBuildContext *context,
                                            const char *name,
                                            CheckedType return_type,
                                            const CheckedType *parameter_types,
                                            size_t parameter_count,
                                            AstSourceSpan source_span) {
    HirExpression *symbol_expr = hr_expression_new(HIR_EXPR_SYMBOL);

    if (!symbol_expr) {
        return NULL;
    }

    if (!hr_init_helper_callable(context, symbol_expr, name, return_type,
                                 parameter_types, parameter_count, source_span)) {
        hir_expression_free(symbol_expr);
        return NULL;
    }

    return symbol_expr;
}

static HirExpression *hr_make_string_literal_expression(HirBuildContext *context,
                                                        const char *text,
                                                        AstSourceSpan source_span) {
    HirExpression *literal_expr = hr_expression_new(HIR_EXPR_LITERAL);
    size_t text_length = text ? strlen(text) : 0;

    if (!literal_expr) {
        return NULL;
    }

    literal_expr->type =
        (CheckedType){CHECKED_TYPE_VALUE, AST_PRIMITIVE_STRING, 0, NULL, 0, false};
    literal_expr->source_span = source_span;
    literal_expr->as.literal.kind = AST_LITERAL_STRING;
    literal_expr->as.literal.as.text = malloc(text_length + 3);
    if (literal_expr->as.literal.as.text) {
        literal_expr->as.literal.as.text[0] = '"';
        if (text_length > 0) {
            memcpy(literal_expr->as.literal.as.text + 1, text, text_length);
        }
        literal_expr->as.literal.as.text[text_length + 1] = '"';
        literal_expr->as.literal.as.text[text_length + 2] = '\0';
    }
    if (!literal_expr->as.literal.as.text) {
        hir_expression_free(literal_expr);
        hr_set_error(context,
                     source_span,
                     NULL,
                     "Out of memory while lowering helper type metadata.");
        return NULL;
    }

    return literal_expr;
}

static HirExpression *hr_make_zero_integer_literal_expression(HirBuildContext *context,
                                                              AstSourceSpan source_span) {
    HirExpression *literal_expr = hr_expression_new(HIR_EXPR_LITERAL);

    if (!literal_expr) {
        return NULL;
    }

    literal_expr->type =
        (CheckedType){CHECKED_TYPE_VALUE, AST_PRIMITIVE_INT32, 0, NULL, 0, false};
    literal_expr->source_span = source_span;
    literal_expr->as.literal.kind = AST_LITERAL_INTEGER;
    literal_expr->as.literal.as.text = ast_copy_text("0");
    if (!literal_expr->as.literal.as.text) {
        hir_expression_free(literal_expr);
        hr_set_error(context,
                     source_span,
                     NULL,
                     "Out of memory while lowering integer literal.");
        return NULL;
    }

    return literal_expr;
}

static HirExpression *hr_make_index_expression(HirBuildContext *context,
                                               const AstExpression *expression,
                                               CheckedType result_type,
                                               HirExpression *target,
                                               HirExpression *index) {
    HirExpression *index_expr = hr_expression_new(HIR_EXPR_INDEX);
    AstSourceSpan source_span = expression ? expression->source_span : (AstSourceSpan){0};

    if (!index_expr) {
        hr_set_error(context,
                     source_span,
                     NULL,
                     "Out of memory while lowering builtin index expression.");
        return NULL;
    }

    index_expr->type = result_type;
    index_expr->source_span = source_span;
    index_expr->as.index.target = target;
    index_expr->as.index.index = index;
    return index_expr;
}

static bool hr_format_helper_type_text(const TypeCheckInfo *info,
                                       char *buffer,
                                       size_t buffer_size) {
    size_t written;

    if (!info || !buffer || buffer_size == 0) {
        return false;
    }

    if (info->type.kind == CHECKED_TYPE_NAMED &&
        info->type.name &&
        info->type.generic_arg_count == 1 &&
        info->has_first_generic_arg) {
        char generic_text[64];
        size_t i;

        if (!checked_type_to_string(info->first_generic_arg_type,
                                    generic_text,
                                    sizeof(generic_text))) {
            return false;
        }

        written = (size_t)snprintf(buffer,
                                   buffer_size,
                                   "%s<%s>",
                                   info->type.name,
                                   generic_text);
        if (written >= buffer_size) {
            return false;
        }

        for (i = 0; i < info->type.array_depth; i++) {
            written += (size_t)snprintf(buffer + written,
                                        buffer_size - written,
                                        "[]");
            if (written >= buffer_size) {
                return false;
            }
        }
        return true;
    }

    return checked_type_to_string(info->type, buffer, buffer_size);
}

#include "hir_lower_expr_p2.inc"
