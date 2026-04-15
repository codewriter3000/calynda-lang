#include "ast_dump_internal.h"

bool ast_dump_qualified_name(AstDumpBuilder *builder, const AstQualifiedName *name) {
    size_t i;

    if (!name || name->count == 0) {
        return ast_dump_builder_append(builder, "<empty>");
    }

    for (i = 0; i < name->count; i++) {
        if (i > 0 && !ast_dump_builder_append_char(builder, '.')) {
            return false;
        }
        if (!ast_dump_builder_append(builder, name->segments[i] ? name->segments[i] : "<null>")) {
            return false;
        }
    }

    return true;
}

bool ast_dump_type(AstDumpBuilder *builder, const AstType *type, bool is_inferred) {
    size_t i;

    if (is_inferred) {
        return ast_dump_builder_append(builder, "var");
    }

    if (!type) {
        return ast_dump_builder_append(builder, "<null-type>");
    }

    if (type->kind == AST_TYPE_VOID) {
        if (!ast_dump_builder_append(builder, "void")) {
            return false;
        }
    } else if (type->kind == AST_TYPE_NAMED) {
        if (!ast_dump_builder_append(builder, type->name ? type->name : "?")) {
            return false;
        }
    } else if (type->kind == AST_TYPE_ARR) {
        if (!ast_dump_builder_append(builder, "arr")) {
            return false;
        }
    } else {
        if (!ast_dump_builder_append(builder, ast_dump_primitive_type_name(type->primitive))) {
            return false;
        }
    }

    if (type->generic_args.count > 0) {
        size_t g;
        if (!ast_dump_builder_append_char(builder, '<')) {
            return false;
        }
        for (g = 0; g < type->generic_args.count; g++) {
            if (g > 0 && !ast_dump_builder_append(builder, ", ")) {
                return false;
            }
            if (type->generic_args.items[g].kind == AST_GENERIC_ARG_WILDCARD) {
                if (!ast_dump_builder_append_char(builder, '?')) {
                    return false;
                }
            } else if (type->generic_args.items[g].type) {
                if (!ast_dump_type(builder, type->generic_args.items[g].type, false)) {
                    return false;
                }
            }
        }
        if (!ast_dump_builder_append_char(builder, '>')) {
            return false;
        }
    }

    for (i = 0; i < type->dimension_count; i++) {
        if (!ast_dump_builder_append_char(builder, '[')) {
            return false;
        }
        if (type->dimensions[i].has_size &&
            !ast_dump_builder_append(builder,
                            type->dimensions[i].size_literal
                                ? type->dimensions[i].size_literal
                                : "<null>")) {
            return false;
        }
        if (!ast_dump_builder_append_char(builder, ']')) {
            return false;
        }
    }

    return true;
}

bool ast_dump_modifiers(AstDumpBuilder *builder, const AstModifier *modifiers,
                        size_t count) {
    size_t i;

    if (!ast_dump_builder_append_char(builder, '[')) {
        return false;
    }

    for (i = 0; i < count; i++) {
        if (i > 0 && !ast_dump_builder_append(builder, ", ")) {
            return false;
        }
        if (!ast_dump_builder_append(builder, ast_dump_modifier_name(modifiers[i]))) {
            return false;
        }
    }

    return ast_dump_builder_append_char(builder, ']');
}

bool ast_dump_parameter_list(AstDumpBuilder *builder,
                             const AstParameterList *parameters, int indent) {
    size_t i;

    if (!parameters || parameters->count == 0) {
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "Parameters: []") &&
               ast_dump_builder_finish_line(builder);
    }

    if (!(ast_dump_builder_start_line(builder, indent) &&
          ast_dump_builder_append(builder, "Parameters:") &&
          ast_dump_builder_finish_line(builder))) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        if (!(ast_dump_builder_start_line(builder, indent + 1) &&
              ast_dump_builder_append(builder, "Parameter name=") &&
              ast_dump_builder_append(builder,
                             parameters->items[i].name ? parameters->items[i].name : "<null>") &&
              ast_dump_builder_append(builder, " type=") &&
              ast_dump_type(builder, &parameters->items[i].type, false) &&
              ast_dump_builder_finish_line(builder))) {
            return false;
        }
    }

    return true;
}

bool ast_dump_expression_label(AstDumpBuilder *builder, int indent,
                               const char *label,
                               const AstExpression *expression) {
    if (!(ast_dump_builder_start_line(builder, indent) &&
          ast_dump_builder_append(builder, label) &&
          ast_dump_builder_append_char(builder, ':') &&
          ast_dump_builder_finish_line(builder))) {
        return false;
    }

    return ast_dump_expression_node(builder, expression, indent + 1);
}

bool ast_dump_block_label(AstDumpBuilder *builder, int indent,
                          const char *label, const AstBlock *block) {
    if (!(ast_dump_builder_start_line(builder, indent) &&
          ast_dump_builder_append(builder, label) &&
          ast_dump_builder_append_char(builder, ':') &&
          ast_dump_builder_finish_line(builder))) {
        return false;
    }

    return ast_dump_block(builder, block, indent + 1);
}

bool ast_dump_literal(AstDumpBuilder *builder, const AstLiteral *literal, int indent) {
    size_t i;

    if (!literal) {
        return ast_dump_builder_start_line(builder, indent) &&
               ast_dump_builder_append(builder, "<null literal>") &&
               ast_dump_builder_finish_line(builder);
    }

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
