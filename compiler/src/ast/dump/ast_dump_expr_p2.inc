    case AST_EXPR_NONLOCAL_RETURN:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "NonlocalReturnExpr") &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }
        if (expression->as.nonlocal_return_value) {
            return ast_dump_expression_label(builder, indent + 1, "Value",
                                             expression->as.nonlocal_return_value);
        }
        return true;
    }

    return false;
}
