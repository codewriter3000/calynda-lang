#include "ast_dump_internal.h"

bool ast_dump_expression_node(AstDumpBuilder *builder,
                              const AstExpression *expression, int indent) {
    size_t i;

    if (!expression) {
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "<null expression>") &&
               ast_dump_builder_finish_line(builder);
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        return ast_dump_literal(builder, &expression->as.literal, indent);

    case AST_EXPR_IDENTIFIER:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "IdentifierExpr name=") &&
               ast_dump_builder_append(builder,
                              expression->as.identifier ? expression->as.identifier : "<null>") &&
               ast_dump_builder_finish_line(builder);

    case AST_EXPR_LAMBDA:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "LambdaExpr") &&
              ast_dump_builder_finish_line(builder) &&
              ast_dump_parameter_list(builder, &expression->as.lambda.parameters, indent + 1))) {
            return false;
        }

        if (expression->as.lambda.body.kind == AST_LAMBDA_BODY_BLOCK) {
            return ast_dump_block_label(builder, indent + 1, "BodyBlock",
                                    expression->as.lambda.body.as.block);
        }

        return ast_dump_expression_label(builder, indent + 1, "BodyExpr",
                                     expression->as.lambda.body.as.expression);

    case AST_EXPR_ASSIGNMENT:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "AssignmentExpr op=") &&
              ast_dump_builder_append_quoted(builder,
                                    ast_dump_assignment_operator_name(
                                        expression->as.assignment.operator)) &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }

        return ast_dump_expression_label(builder, indent + 1, "Target",
                                     expression->as.assignment.target) &&
               ast_dump_expression_label(builder, indent + 1, "Value",
                                     expression->as.assignment.value);

    case AST_EXPR_TERNARY:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "TernaryExpr") &&
               ast_dump_builder_finish_line(builder) &&
               ast_dump_expression_label(builder, indent + 1, "Condition",
                                     expression->as.ternary.condition) &&
               ast_dump_expression_label(builder, indent + 1, "Then",
                                     expression->as.ternary.then_branch) &&
               ast_dump_expression_label(builder, indent + 1, "Else",
                                     expression->as.ternary.else_branch);

    case AST_EXPR_BINARY:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "BinaryExpr op=") &&
              ast_dump_builder_append_quoted(builder,
                                    ast_dump_binary_operator_name(expression->as.binary.operator)) &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }

        return ast_dump_expression_label(builder, indent + 1, "Left",
                                     expression->as.binary.left) &&
               ast_dump_expression_label(builder, indent + 1, "Right",
                                     expression->as.binary.right);

    case AST_EXPR_UNARY:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "UnaryExpr op=") &&
              ast_dump_builder_append_quoted(builder,
                                    ast_dump_unary_operator_name(expression->as.unary.operator)) &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }

        return ast_dump_expression_label(builder, indent + 1, "Operand",
                                     expression->as.unary.operand);

    case AST_EXPR_CALL:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "CallExpr") &&
              ast_dump_builder_finish_line(builder) &&
              ast_dump_expression_label(builder, indent + 1, "Callee",
                                    expression->as.call.callee))) {
            return false;
        }

        if (expression->as.call.arguments.count == 0) {
            return ast_dump_builder_start_line(builder, indent + 1) &&
                   ast_dump_builder_append(builder, "Arguments: []") &&
                   ast_dump_builder_finish_line(builder);
        }

        if (!(ast_dump_builder_start_line(builder, indent + 1) &&
              ast_dump_builder_append(builder, "Arguments:") &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }

        for (i = 0; i < expression->as.call.arguments.count; i++) {
            if (!ast_dump_expression_node(builder, expression->as.call.arguments.items[i],
                                      indent + 2)) {
                return false;
            }
        }

        return true;

    case AST_EXPR_INDEX:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "IndexExpr") &&
               ast_dump_builder_finish_line(builder) &&
               ast_dump_expression_label(builder, indent + 1, "Target",
                                     expression->as.index.target) &&
               ast_dump_expression_label(builder, indent + 1, "Index",
                                     expression->as.index.index);

    case AST_EXPR_MEMBER:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "MemberExpr name=") &&
               ast_dump_builder_append(builder,
                              expression->as.member.member ? expression->as.member.member : "<null>") &&
               ast_dump_builder_finish_line(builder) &&
               ast_dump_expression_label(builder, indent + 1, "Target",
                                     expression->as.member.target);

    case AST_EXPR_CAST:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "CastExpr type=") &&
               ast_dump_builder_append(builder,
                              ast_dump_primitive_type_name(expression->as.cast.target_type)) &&
               ast_dump_builder_finish_line(builder) &&
               ast_dump_expression_label(builder, indent + 1, "Expression",
                                     expression->as.cast.expression);

    case AST_EXPR_ARRAY_LITERAL:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "ArrayLiteralExpr") &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }

        if (expression->as.array_literal.elements.count == 0) {
            return ast_dump_builder_start_line(builder, indent + 1) &&
                   ast_dump_builder_append(builder, "Elements: []") &&
                   ast_dump_builder_finish_line(builder);
        }

        if (!(ast_dump_builder_start_line(builder, indent + 1) &&
              ast_dump_builder_append(builder, "Elements:") &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }

        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            if (!ast_dump_expression_node(builder,
                                      expression->as.array_literal.elements.items[i],
                                      indent + 2)) {
                return false;
            }
        }

        return true;

    case AST_EXPR_GROUPING:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "GroupingExpr") &&
               ast_dump_builder_finish_line(builder) &&
               ast_dump_expression_label(builder, indent + 1, "Expression",
                                     expression->as.grouping.inner);

    case AST_EXPR_DISCARD:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "DiscardExpr") &&
               ast_dump_builder_finish_line(builder);

    case AST_EXPR_POST_INCREMENT:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "PostIncrementExpr") &&
               ast_dump_builder_finish_line(builder) &&
               ast_dump_expression_label(builder, indent + 1, "Operand",
                                     expression->as.post_increment.operand);

    case AST_EXPR_POST_DECREMENT:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "PostDecrementExpr") &&
               ast_dump_builder_finish_line(builder) &&
               ast_dump_expression_label(builder, indent + 1, "Operand",
                                     expression->as.post_decrement.operand);

    case AST_EXPR_MEMORY_OP: {
        const char *op_name;
        switch (expression->as.memory_op.kind) {
        case AST_MEMORY_MALLOC:  op_name = "malloc";  break;
        case AST_MEMORY_CALLOC:  op_name = "calloc";  break;
        case AST_MEMORY_REALLOC: op_name = "realloc"; break;
        case AST_MEMORY_FREE:    op_name = "free";    break;
        default:                 op_name = "unknown"; break;
        }

        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "MemoryOpExpr op=") &&
              ast_dump_builder_append(builder, op_name) &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }

        for (i = 0; i < expression->as.memory_op.arguments.count; i++) {
            if (!ast_dump_expression_node(builder,
                                      expression->as.memory_op.arguments.items[i],
                                      indent + 1)) {
                return false;
            }
        }

        return true;
    }
    }

    return false;
}
