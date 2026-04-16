#include "hir_internal.h"

#include <stdlib.h>
#include <string.h>

static void hr_lower_memory_pointer_metadata(HirBuildContext *context,
                                             const AstExpression *expression,
                                             HirExpression *hir_expression) {
    const AstExpression *first_argument;
    const SymbolResolution *resolution;
    const AstType *declared_type;
    size_t ptr_type_arg_count;
    bool second_is_checked;

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
    if (first_argument->kind != AST_EXPR_IDENTIFIER) {
        return;
    }

    resolution = symbol_table_find_resolution(context->symbols, first_argument);
    if (!resolution || !resolution->symbol || !resolution->symbol->declared_type) {
        return;
    }

    declared_type = resolution->symbol->declared_type;
    ptr_type_arg_count = declared_type->generic_args.count;
    second_is_checked = ptr_type_arg_count == 2 &&
        declared_type->generic_args.items[1].kind == AST_GENERIC_ARG_TYPE &&
        declared_type->generic_args.items[1].type != NULL &&
        declared_type->generic_args.items[1].type->kind == AST_TYPE_NAMED &&
        declared_type->generic_args.items[1].type->name != NULL &&
        strcmp(declared_type->generic_args.items[1].type->name, "checked") == 0;

    if (declared_type->kind == AST_TYPE_PTR &&
        (ptr_type_arg_count == 1 || second_is_checked) &&
        declared_type->generic_args.items[0].kind == AST_GENERIC_ARG_TYPE &&
        declared_type->generic_args.items[0].type != NULL) {
        const AstType *inner = declared_type->generic_args.items[0].type;

        if (inner->kind == AST_TYPE_PRIMITIVE) {
            hir_expression->as.memory_op.element_size =
                hr_primitive_byte_size(inner->primitive);
        } else if (inner->kind == AST_TYPE_NAMED && inner->name != NULL) {
            hir_expression->as.memory_op.element_size =
                hr_layout_byte_size(context->symbols, inner->name);
        }
    }

    if (second_is_checked) {
        hir_expression->as.memory_op.is_checked_ptr = true;
    }
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