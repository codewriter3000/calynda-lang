#include "hir_dump_internal.h"

bool hir_dump_expression(FILE *out, const HirExpression *expression, int indent) {
    size_t i;

    if (!expression) {
        return false;
    }

    hir_dump_write_indent(out, indent);
    switch (expression->kind) {
    case HIR_EXPR_LITERAL:
        fprintf(out, "Literal kind=%d type=", expression->as.literal.kind);
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        if (expression->as.literal.kind == AST_LITERAL_BOOL) {
            fprintf(out, " value=%s", expression->as.literal.as.bool_value ? "true" : "false");
        } else if (expression->as.literal.kind != AST_LITERAL_NULL) {
            fprintf(out, " text=%s", expression->as.literal.as.text);
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        return true;
    case HIR_EXPR_TEMPLATE:
        fprintf(out, "Template type=");
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        for (i = 0; i < expression->as.template_parts.count; i++) {
            hir_dump_write_indent(out, indent + 2);
            if (expression->as.template_parts.items[i].kind == AST_TEMPLATE_PART_TEXT) {
                fprintf(out, "TextPart text=%s\n",
                        expression->as.template_parts.items[i].as.text);
            } else {
                fprintf(out, "ExpressionPart\n");
                if (!hir_dump_expression(out,
                                         expression->as.template_parts.items[i].as.expression,
                                         indent + 4)) {
                    return false;
                }
            }
        }
        return true;
    case HIR_EXPR_SYMBOL:
        fprintf(out, "Symbol name=%s kind=%s type=",
                expression->as.symbol.name,
                symbol_kind_name(expression->as.symbol.kind));
        if (!hir_dump_write_checked_type(out, expression->as.symbol.type)) {
            return false;
        }
        if (expression->is_callable) {
            fprintf(out, " callable=");
            if (!hir_dump_write_callable_signature(out, &expression->callable_signature)) {
                return false;
            }
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->as.symbol.source_span);
        fputc('\n', out);
        return true;
    case HIR_EXPR_LAMBDA:
        fprintf(out, "Lambda type=");
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " callable=");
        if (!hir_dump_write_callable_signature(out, &expression->callable_signature)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Parameters:\n");
        for (i = 0; i < expression->as.lambda.parameters.count; i++) {
            hir_dump_write_indent(out, indent + 4);
            fprintf(out, "Param name=%s type=",
                    expression->as.lambda.parameters.items[i].name);
            if (!hir_dump_write_checked_type(out,
                    expression->as.lambda.parameters.items[i].type)) {
                return false;
            }
            if (expression->as.lambda.parameters.items[i].is_varargs) {
                fprintf(out, " varargs");
            }
            fprintf(out, " span=");
            hir_dump_write_span(out, expression->as.lambda.parameters.items[i].source_span);
            fputc('\n', out);
        }
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Body:\n");
        return hir_dump_block(out, expression->as.lambda.body, indent + 4);
    case HIR_EXPR_ASSIGNMENT:
        fprintf(out, "Assignment op=%d type=", expression->as.assignment.operator);
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Target:\n");
        if (!hir_dump_expression(out, expression->as.assignment.target, indent + 4)) {
            return false;
        }
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Value:\n");
        return hir_dump_expression(out, expression->as.assignment.value, indent + 4);
    case HIR_EXPR_TERNARY:
        fprintf(out, "Ternary type=");
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Condition:\n");
        if (!hir_dump_expression(out, expression->as.ternary.condition, indent + 4)) {
            return false;
        }
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Then:\n");
        if (!hir_dump_expression(out, expression->as.ternary.then_branch, indent + 4)) {
            return false;
        }
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Else:\n");
        return hir_dump_expression(out, expression->as.ternary.else_branch, indent + 4);
    case HIR_EXPR_BINARY:
        fprintf(out, "Binary op=%s type=",
                hir_dump_binary_operator_name_text(expression->as.binary.operator));
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Left:\n");
        if (!hir_dump_expression(out, expression->as.binary.left, indent + 4)) {
            return false;
        }
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Right:\n");
        return hir_dump_expression(out, expression->as.binary.right, indent + 4);
    case HIR_EXPR_UNARY:
        fprintf(out, "Unary op=%d type=", expression->as.unary.operator);
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        return hir_dump_expression(out, expression->as.unary.operand, indent + 2);
    case HIR_EXPR_CALL:
        fprintf(out, "Call type=");
        if (!hir_dump_write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        hir_dump_write_span(out, expression->source_span);
        fputc('\n', out);
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Callee:\n");
        if (!hir_dump_expression(out, expression->as.call.callee, indent + 4)) {
            return false;
        }
        for (i = 0; i < expression->as.call.argument_count; i++) {
            hir_dump_write_indent(out, indent + 2);
            fprintf(out, "Arg %zu:\n", i + 1);
            if (!hir_dump_expression(out, expression->as.call.arguments[i], indent + 4)) {
                return false;
            }
        }
        return true;
    default:
        return hir_dump_expression_ext(out, expression, indent);
    }

    return false;
}
