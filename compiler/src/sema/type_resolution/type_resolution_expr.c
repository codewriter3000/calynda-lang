#include "type_resolution_internal.h"

bool tr_resolve_expression(TypeResolver *resolver, const AstExpression *expression) {
    size_t i;

    if (!expression) {
        return true;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                const AstTemplatePart *part = &expression->as.literal.as.template_parts.items[i];

                if (part->kind == AST_TEMPLATE_PART_EXPRESSION &&
                    !tr_resolve_expression(resolver, part->as.expression)) {
                    return false;
                }
            }
        }
        return true;

    case AST_EXPR_IDENTIFIER:
        return true;

    case AST_EXPR_LAMBDA:
        for (i = 0; i < expression->as.lambda.parameters.count; i++) {
            if (!tr_resolve_parameter(resolver, &expression->as.lambda.parameters.items[i])) {
                return false;
            }
        }

        if (expression->as.lambda.body.kind == AST_LAMBDA_BODY_BLOCK) {
            return tr_resolve_block(resolver, expression->as.lambda.body.as.block);
        }

        return tr_resolve_expression(resolver, expression->as.lambda.body.as.expression);

    case AST_EXPR_ASSIGNMENT:
        return tr_resolve_expression(resolver, expression->as.assignment.target) &&
               tr_resolve_expression(resolver, expression->as.assignment.value);

    case AST_EXPR_TERNARY:
        return tr_resolve_expression(resolver, expression->as.ternary.condition) &&
               tr_resolve_expression(resolver, expression->as.ternary.then_branch) &&
               tr_resolve_expression(resolver, expression->as.ternary.else_branch);

    case AST_EXPR_BINARY:
        return tr_resolve_expression(resolver, expression->as.binary.left) &&
               tr_resolve_expression(resolver, expression->as.binary.right);

    case AST_EXPR_UNARY:
        return tr_resolve_expression(resolver, expression->as.unary.operand);

    case AST_EXPR_CALL:
        if (!tr_resolve_expression(resolver, expression->as.call.callee)) {
            return false;
        }
        for (i = 0; i < expression->as.call.arguments.count; i++) {
            if (!tr_resolve_expression(resolver, expression->as.call.arguments.items[i])) {
                return false;
            }
        }
        return true;

    case AST_EXPR_INDEX:
        return tr_resolve_expression(resolver, expression->as.index.target) &&
               tr_resolve_expression(resolver, expression->as.index.index);

    case AST_EXPR_MEMBER:
        return tr_resolve_expression(resolver, expression->as.member.target);

    case AST_EXPR_CAST:
        if (!tr_append_cast_entry(resolver,
                                  expression,
                                  tr_resolved_type_value(expression->as.cast.target_type, 0))) {
            return false;
        }
        return tr_resolve_expression(resolver, expression->as.cast.expression);

    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            if (!tr_resolve_expression(resolver, expression->as.array_literal.elements.items[i])) {
                return false;
            }
        }
        return true;

    case AST_EXPR_GROUPING:
        return tr_resolve_expression(resolver, expression->as.grouping.inner);

    case AST_EXPR_DISCARD:
        return true;

    case AST_EXPR_POST_INCREMENT:
        return tr_resolve_expression(resolver, expression->as.post_increment.operand);

    case AST_EXPR_POST_DECREMENT:
        return tr_resolve_expression(resolver, expression->as.post_decrement.operand);

    case AST_EXPR_MEMORY_OP:
        for (i = 0; i < expression->as.memory_op.arguments.count; i++) {
            if (!tr_resolve_expression(resolver, expression->as.memory_op.arguments.items[i])) {
                return false;
            }
        }
        return true;
    case AST_EXPR_SPAWN:
        return tr_resolve_expression(resolver, expression->as.spawn.callable);

    case AST_EXPR_NONLOCAL_RETURN:
        if (expression->as.nonlocal_return_value) {
            return tr_resolve_expression(resolver, expression->as.nonlocal_return_value);
        }
        return true;
    }

    return false;
}
