#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

static bool hr_checked_type_is_pointer_like(CheckedType type) {
    return type.kind == CHECKED_TYPE_NAMED &&
        type.array_depth == 0 &&
        type.name != NULL &&
        (strcmp(type.name, "ptr") == 0 || strcmp(type.name, "mmio") == 0);
}

static size_t hr_checked_type_byte_size(HirBuildContext *context, CheckedType type) {
    if (!context) {
        return 0;
    }

    if (type.kind == CHECKED_TYPE_VALUE && type.array_depth == 0) {
        return hr_primitive_byte_size(type.primitive);
    }

    if (type.kind == CHECKED_TYPE_NAMED && type.array_depth == 0 && type.name != NULL) {
        return hr_layout_byte_size(context->symbols, type.name);
    }

    return 0;
}

static void hr_lower_memory_pointer_metadata(HirBuildContext *context,
                                             const AstExpression *expression,
                                             HirExpression *hir_expression) {
    const AstExpression *first_argument;
    const TypeCheckInfo *first_argument_info;
    CheckedType first_argument_type;

    if (!context || !expression || !hir_expression ||
        expression->as.memory_op.arguments.count == 0) {
        return;
    }

    if (hir_expression->as.memory_op.kind != HIR_MEMORY_DEREF &&
        hir_expression->as.memory_op.kind != HIR_MEMORY_OFFSET &&
        hir_expression->as.memory_op.kind != HIR_MEMORY_STORE &&
        hir_expression->as.memory_op.kind != HIR_MEMORY_ADDR) {
        return;
    }

    first_argument = expression->as.memory_op.arguments.items[0];
    first_argument_info = type_checker_get_expression_info(context->checker, first_argument);
    if (!first_argument_info) {
        return;
    }

    first_argument_type = first_argument_info->type;
    if (!hr_checked_type_is_pointer_like(first_argument_type)) {
        return;
    }

    if (first_argument_info->has_first_generic_arg) {
        hir_expression->as.memory_op.element_size =
            hr_checked_type_byte_size(context, first_argument_info->first_generic_arg_type);
    }

    if (first_argument_type.name != NULL &&
        strcmp(first_argument_type.name, "mmio") == 0) {
        hir_expression->as.memory_op.is_mmio = true;
    }

    if (first_argument_type.name != NULL &&
        strcmp(first_argument_type.name, "ptr") == 0 &&
        first_argument_type.is_bounds_checked) {
        hir_expression->as.memory_op.is_checked_ptr = true;
    }
}

HirExpression *hr_lower_mmio_value_expression(HirBuildContext *context,
                                              const AstExpression *expression,
                                              const TypeCheckInfo *info) {
    HirExpression *hir_expression;
    const TypeCheckInfo *target_info;

    if (!context || !expression || !info || expression->kind != AST_EXPR_MEMBER) {
        return NULL;
    }

    hir_expression = hr_expression_new(HIR_EXPR_MEMORY_OP);
    if (!hir_expression) {
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering HIR MMIO access.");
        return NULL;
    }

    hir_expression->type = info->type;
    hir_expression->is_callable = info->is_callable;
    hir_expression->source_span = expression->source_span;
    hir_expression->as.memory_op.kind = HIR_MEMORY_DEREF;
    hir_expression->as.memory_op.argument_count = 1;
    hir_expression->as.memory_op.element_size = 0;
    hir_expression->as.memory_op.is_checked_ptr = false;
    hir_expression->as.memory_op.is_mmio = true;
    hir_expression->as.memory_op.arguments = calloc(1, sizeof(*hir_expression->as.memory_op.arguments));
    if (!hir_expression->as.memory_op.arguments) {
        hir_expression_free(hir_expression);
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering HIR MMIO access.");
        return NULL;
    }

    hir_expression->as.memory_op.arguments[0] =
        hr_lower_expression(context, expression->as.member.target);
    if (!hir_expression->as.memory_op.arguments[0]) {
        hir_expression_free(hir_expression);
        return NULL;
    }

    target_info = type_checker_get_expression_info(context->checker,
                                                   expression->as.member.target);
    if (target_info && target_info->has_first_generic_arg) {
        hir_expression->as.memory_op.element_size =
            hr_checked_type_byte_size(context, target_info->first_generic_arg_type);
    }

    return hir_expression;
}

HirExpression *hr_lower_memory_expression(HirBuildContext *context,
                                          const AstExpression *expression,
                                          const TypeCheckInfo *info) {
    HirExpression *hir_expression;
    size_t memory_index;

    hir_expression = hr_expression_new(HIR_EXPR_MEMORY_OP);
    if (!hir_expression) {
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering HIR expressions.");
        return NULL;
    }

    hir_expression->type = info->type;
    hir_expression->is_callable = info->is_callable;
    hir_expression->source_span = expression->source_span;
    hir_expression->as.memory_op.kind = (HirMemoryOpKind)expression->as.memory_op.kind;
    hir_expression->as.memory_op.element_size = 0;
    hir_expression->as.memory_op.is_checked_ptr = false;
    hir_expression->as.memory_op.is_mmio = false;
    hr_lower_memory_pointer_metadata(context, expression, hir_expression);

    hir_expression->as.memory_op.argument_count = expression->as.memory_op.arguments.count;
    if (expression->as.memory_op.arguments.count == 0) {
        return hir_expression;
    }

    hir_expression->as.memory_op.arguments =
        calloc(expression->as.memory_op.arguments.count,
               sizeof(*hir_expression->as.memory_op.arguments));
    if (!hir_expression->as.memory_op.arguments) {
        hir_expression_free(hir_expression);
        hr_set_error(context,
                     expression->source_span,
                     NULL,
                     "Out of memory while lowering HIR memory operation.");
        return NULL;
    }

    for (memory_index = 0;
         memory_index < expression->as.memory_op.arguments.count;
         memory_index++) {
        hir_expression->as.memory_op.arguments[memory_index] =
            hr_lower_expression(context, expression->as.memory_op.arguments.items[memory_index]);
        if (!hir_expression->as.memory_op.arguments[memory_index]) {
            hir_expression_free(hir_expression);
            return NULL;
        }
    }

    return hir_expression;
}