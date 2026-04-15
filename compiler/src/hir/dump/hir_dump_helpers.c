#include "hir_dump_internal.h"

void hir_dump_write_indent(FILE *out, int indent) {
    int i;

    for (i = 0; i < indent; i++) {
        fputc(' ', out);
    }
}

bool hir_dump_write_checked_type(FILE *out, CheckedType type) {
    char buffer[64];

    if (!checked_type_to_string(type, buffer, sizeof(buffer))) {
        return false;
    }

    fputs(buffer, out);
    return true;
}

bool hir_dump_write_callable_signature(FILE *out, const HirCallableSignature *signature) {
    size_t i;

    if (!signature) {
        return false;
    }

    fputc('(', out);
    if (signature->has_parameter_types) {
        for (i = 0; i < signature->parameter_count; i++) {
            if (i > 0) {
                fputs(", ", out);
            }
            if (!hir_dump_write_checked_type(out, signature->parameter_types[i])) {
                return false;
            }
        }
    } else {
        fputs("...", out);
    }
    fputs(") -> ", out);
    return hir_dump_write_checked_type(out, signature->return_type);
}

void hir_dump_write_span(FILE *out, AstSourceSpan span) {
    if (span.start_line > 0 && span.start_column > 0) {
        fprintf(out, "%d:%d", span.start_line, span.start_column);
    } else {
        fputs("<unknown>", out);
    }
}

const char *hir_dump_binary_operator_name_text(AstBinaryOperator operator) {
    switch (operator) {
    case AST_BINARY_OP_LOGICAL_OR:
        return "||";
    case AST_BINARY_OP_LOGICAL_AND:
        return "&&";
    case AST_BINARY_OP_BIT_OR:
        return "|";
    case AST_BINARY_OP_BIT_NAND:
        return "!&";
    case AST_BINARY_OP_BIT_XOR:
        return "^";
    case AST_BINARY_OP_BIT_XNOR:
        return "!^";
    case AST_BINARY_OP_BIT_AND:
        return "&";
    case AST_BINARY_OP_EQUAL:
        return "==";
    case AST_BINARY_OP_NOT_EQUAL:
        return "!=";
    case AST_BINARY_OP_LESS:
        return "<";
    case AST_BINARY_OP_GREATER:
        return ">";
    case AST_BINARY_OP_LESS_EQUAL:
        return "<=";
    case AST_BINARY_OP_GREATER_EQUAL:
        return ">=";
    case AST_BINARY_OP_SHIFT_LEFT:
        return "<<";
    case AST_BINARY_OP_SHIFT_RIGHT:
        return ">>";
    case AST_BINARY_OP_ADD:
        return "+";
    case AST_BINARY_OP_SUBTRACT:
        return "-";
    case AST_BINARY_OP_MULTIPLY:
        return "*";
    case AST_BINARY_OP_DIVIDE:
        return "/";
    case AST_BINARY_OP_MODULO:
        return "%";
    }

    return "?";
}

bool hir_dump_block(FILE *out, const HirBlock *block, int indent) {
    size_t i;

    if (!block) {
        hir_dump_write_indent(out, indent);
        fprintf(out, "Block statements=0\n");
        return true;
    }

    hir_dump_write_indent(out, indent);
    fprintf(out, "Block statements=%zu\n", block->statement_count);
    for (i = 0; i < block->statement_count; i++) {
        if (!hir_dump_statement(out, block->statements[i], indent + 2)) {
            return false;
        }
    }
    return true;
}

bool hir_dump_statement(FILE *out, const HirStatement *statement, int indent) {
    if (!statement) {
        return false;
    }

    switch (statement->kind) {
    case HIR_STMT_LOCAL_BINDING:
        hir_dump_write_indent(out, indent);
        fprintf(out, "Local name=%s type=", statement->as.local_binding.name);
        if (!hir_dump_write_checked_type(out, statement->as.local_binding.type)) {
            return false;
        }
        if (statement->as.local_binding.is_callable) {
            fprintf(out, " callable=");
            if (!hir_dump_write_callable_signature(out,
                    &statement->as.local_binding.callable_signature)) {
                return false;
            }
        }
        fprintf(out, " final=%s span=",
                statement->as.local_binding.is_final ? "true" : "false");
        hir_dump_write_span(out, statement->as.local_binding.source_span);
        fputc('\n', out);
        hir_dump_write_indent(out, indent + 2);
        fprintf(out, "Init:\n");
        return hir_dump_expression(out, statement->as.local_binding.initializer, indent + 4);
    case HIR_STMT_RETURN:
        hir_dump_write_indent(out, indent);
        fprintf(out, "Return span=");
        hir_dump_write_span(out, statement->source_span);
        fputc('\n', out);
        if (statement->as.return_expression) {
            return hir_dump_expression(out, statement->as.return_expression, indent + 2);
        }
        return true;
    case HIR_STMT_EXIT:
        hir_dump_write_indent(out, indent);
        fprintf(out, "InvalidExit span=");
        hir_dump_write_span(out, statement->source_span);
        fputc('\n', out);
        return true;
    case HIR_STMT_THROW:
        hir_dump_write_indent(out, indent);
        fprintf(out, "Throw span=");
        hir_dump_write_span(out, statement->source_span);
        fputc('\n', out);
        return hir_dump_expression(out, statement->as.throw_expression, indent + 2);
    case HIR_STMT_EXPRESSION:
        hir_dump_write_indent(out, indent);
        fprintf(out, "Expression span=");
        hir_dump_write_span(out, statement->source_span);
        fputc('\n', out);
        return hir_dump_expression(out, statement->as.expression, indent + 2);
    case HIR_STMT_MANUAL:
        hir_dump_write_indent(out, indent);
        fprintf(out, "Manual span=");
        hir_dump_write_span(out, statement->source_span);
        fputc('\n', out);
        if (statement->as.manual_body) {
            return hir_dump_block(out, statement->as.manual_body, indent + 2);
        }
        return true;
    }

    return false;
}
