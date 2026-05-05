    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        ast_literal_free_internal(&expression->as.literal);
        break;
    case AST_EXPR_IDENTIFIER:
        free(expression->as.identifier);
        break;
    case AST_EXPR_LAMBDA:
        ast_parameter_list_free(&expression->as.lambda.parameters);
        ast_lambda_body_free_internal(&expression->as.lambda.body);
        break;
    case AST_EXPR_ASSIGNMENT:
        ast_expression_free(expression->as.assignment.target);
        ast_expression_free(expression->as.assignment.value);
        break;
    case AST_EXPR_TERNARY:
        ast_expression_free(expression->as.ternary.condition);
        ast_expression_free(expression->as.ternary.then_branch);
        ast_expression_free(expression->as.ternary.else_branch);
        break;
    case AST_EXPR_BINARY:
        ast_expression_free(expression->as.binary.left);
        ast_expression_free(expression->as.binary.right);
        break;
    case AST_EXPR_UNARY:
        ast_expression_free(expression->as.unary.operand);
        break;
    case AST_EXPR_CALL:
        ast_expression_free(expression->as.call.callee);
        ast_expression_list_free_internal(&expression->as.call.arguments);
        break;
    case AST_EXPR_INDEX:
        ast_expression_free(expression->as.index.target);
        ast_expression_free(expression->as.index.index);
        break;
    case AST_EXPR_MEMBER:
        ast_expression_free(expression->as.member.target);
        free(expression->as.member.member);
        break;
    case AST_EXPR_CAST:
        ast_expression_free(expression->as.cast.expression);
        break;
    case AST_EXPR_ARRAY_LITERAL:
        ast_expression_list_free_internal(&expression->as.array_literal.elements);
        break;
    case AST_EXPR_GROUPING:
        ast_expression_free(expression->as.grouping.inner);
        break;
    case AST_EXPR_DISCARD:
        break;
    case AST_EXPR_POST_INCREMENT:
        ast_expression_free(expression->as.post_increment.operand);
        break;
    case AST_EXPR_POST_DECREMENT:
        ast_expression_free(expression->as.post_decrement.operand);
        break;
    case AST_EXPR_MEMORY_OP:
        ast_expression_list_free_internal(&expression->as.memory_op.arguments);
        break;
    case AST_EXPR_SPAWN:
        ast_expression_free(expression->as.spawn.callable);
        break;
    case AST_EXPR_NONLOCAL_RETURN:
        ast_expression_free(expression->as.nonlocal_return_value);
        break;
    }

    free(expression);
}
