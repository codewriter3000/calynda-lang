    switch (literal->kind) {
    case AST_LITERAL_INTEGER:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "IntegerLiteral value=") &&
               ast_dump_builder_append(builder, literal->as.text ? literal->as.text : "<null>") &&
               ast_dump_builder_finish_line(builder);
    case AST_LITERAL_FLOAT:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "FloatLiteral value=") &&
               ast_dump_builder_append(builder, literal->as.text ? literal->as.text : "<null>") &&
               ast_dump_builder_finish_line(builder);
    case AST_LITERAL_BOOL:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder,
                              literal->as.bool_value ? "BoolLiteral value=true"
                                                     : "BoolLiteral value=false") &&
               ast_dump_builder_finish_line(builder);
    case AST_LITERAL_CHAR:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "CharLiteral value=") &&
               ast_dump_builder_append(builder, literal->as.text ? literal->as.text : "<null>") &&
               ast_dump_builder_finish_line(builder);
    case AST_LITERAL_STRING:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "StringLiteral value=") &&
               ast_dump_builder_append(builder, literal->as.text ? literal->as.text : "<null>") &&
               ast_dump_builder_finish_line(builder);
    case AST_LITERAL_TEMPLATE:
        if (!(ast_dump_builder_start_line(builder, indent) &&
              ast_dump_builder_append(builder, "TemplateLiteral") &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }

        for (i = 0; i < literal->as.template_parts.count; i++) {
            const AstTemplatePart *part = &literal->as.template_parts.items[i];

            if (part->kind == AST_TEMPLATE_PART_TEXT) {
                if (!(ast_dump_builder_start_line(builder, indent + 1) &&
                      ast_dump_builder_append(builder, "TextPart ") &&
                      ast_dump_builder_append_quoted(builder,
                                            part->as.text ? part->as.text : "") &&
                      ast_dump_builder_finish_line(builder))) {
                    return false;
                }
            } else if (!ast_dump_expression_label(builder, indent + 1, "ExpressionPart",
                                              part->as.expression)) {
                return false;
            }
        }

        return true;
    case AST_LITERAL_NULL:
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "NullLiteral") &&
               ast_dump_builder_finish_line(builder);
    }

    return false;
}
