#include "hir_dump.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_indent(FILE *out, int indent);
static bool write_checked_type(FILE *out, CheckedType type);
static bool write_callable_signature(FILE *out, const HirCallableSignature *signature);
static void write_span(FILE *out, AstSourceSpan span);
static const char *binary_operator_name_text(AstBinaryOperator operator);
static bool dump_block(FILE *out, const HirBlock *block, int indent);
static bool dump_statement(FILE *out, const HirStatement *statement, int indent);
static bool dump_expression(FILE *out, const HirExpression *expression, int indent);

bool hir_dump_program(FILE *out, const HirProgram *program) {
    size_t i;

    if (!out || !program) {
        return false;
    }

    fprintf(out, "HirProgram\n");
    if (program->has_package) {
        fprintf(out, "  Package: name=%s qualified=%s span=", program->package.name,
                program->package.qualified_name);
        write_span(out, program->package.source_span);
        fputc('\n', out);
    } else {
        fprintf(out, "  Package: <none>\n");
    }

    if (program->import_count == 0) {
        fprintf(out, "  Imports: []\n");
    } else {
        fprintf(out, "  Imports:\n");
        for (i = 0; i < program->import_count; i++) {
            fprintf(out, "    Import name=%s qualified=%s span=",
                    program->imports[i].name,
                    program->imports[i].qualified_name);
            write_span(out, program->imports[i].source_span);
            fputc('\n', out);
        }
    }

    fprintf(out, "  TopLevel:\n");
    for (i = 0; i < program->top_level_count; i++) {
        const HirTopLevelDecl *decl = program->top_level_decls[i];

        if (decl->kind == HIR_TOP_LEVEL_BINDING) {
            fprintf(out, "    Binding name=%s type=", decl->as.binding.name);
            if (!write_checked_type(out, decl->as.binding.type)) {
                return false;
            }
            fprintf(out, " final=%s", decl->as.binding.is_final ? "true" : "false");
            if (decl->as.binding.is_callable) {
                fprintf(out, " callable=");
                if (!write_callable_signature(out, &decl->as.binding.callable_signature)) {
                    return false;
                }
            }
            fprintf(out, " span=");
            write_span(out, decl->as.binding.source_span);
            fputc('\n', out);
            fprintf(out, "      Init:\n");
            if (!dump_expression(out, decl->as.binding.initializer, 8)) {
                return false;
            }
        } else {
            size_t j;

            fprintf(out, "    Start span=");
            write_span(out, decl->as.start.source_span);
            fputc('\n', out);
            fprintf(out, "      Parameters:\n");
            for (j = 0; j < decl->as.start.parameters.count; j++) {
                fprintf(out, "        Param name=%s type=", decl->as.start.parameters.items[j].name);
                if (!write_checked_type(out, decl->as.start.parameters.items[j].type)) {
                    return false;
                }
                fprintf(out, " span=");
                write_span(out, decl->as.start.parameters.items[j].source_span);
                fputc('\n', out);
            }
            fprintf(out, "      Body:\n");
            if (!dump_block(out, decl->as.start.body, 8)) {
                return false;
            }
        }
    }

    return !ferror(out);
}

char *hir_dump_program_to_string(const HirProgram *program) {
    FILE *stream;
    char *buffer;
    long size;
    size_t read_size;

    if (!program) {
        return NULL;
    }

    stream = tmpfile();
    if (!stream) {
        return NULL;
    }

    if (!hir_dump_program(stream, program) || fflush(stream) != 0 ||
        fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return NULL;
    }

    size = ftell(stream);
    if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(stream);
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, stream);
    fclose(stream);
    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

static void write_indent(FILE *out, int indent) {
    int i;

    for (i = 0; i < indent; i++) {
        fputc(' ', out);
    }
}

static bool write_checked_type(FILE *out, CheckedType type) {
    char buffer[64];

    if (!checked_type_to_string(type, buffer, sizeof(buffer))) {
        return false;
    }

    fputs(buffer, out);
    return true;
}

static bool write_callable_signature(FILE *out, const HirCallableSignature *signature) {
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
            if (!write_checked_type(out, signature->parameter_types[i])) {
                return false;
            }
        }
    } else {
        fputs("...", out);
    }
    fputs(") -> ", out);
    return write_checked_type(out, signature->return_type);
}

static void write_span(FILE *out, AstSourceSpan span) {
    if (span.start_line > 0 && span.start_column > 0) {
        fprintf(out, "%d:%d", span.start_line, span.start_column);
    } else {
        fputs("<unknown>", out);
    }
}

static const char *binary_operator_name_text(AstBinaryOperator operator) {
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

static bool dump_block(FILE *out, const HirBlock *block, int indent) {
    size_t i;

    if (!block) {
        write_indent(out, indent);
        fprintf(out, "Block statements=0\n");
        return true;
    }

    write_indent(out, indent);
    fprintf(out, "Block statements=%zu\n", block->statement_count);
    for (i = 0; i < block->statement_count; i++) {
        if (!dump_statement(out, block->statements[i], indent + 2)) {
            return false;
        }
    }
    return true;
}

static bool dump_statement(FILE *out, const HirStatement *statement, int indent) {
    if (!statement) {
        return false;
    }

    switch (statement->kind) {
    case HIR_STMT_LOCAL_BINDING:
        write_indent(out, indent);
        fprintf(out, "Local name=%s type=", statement->as.local_binding.name);
        if (!write_checked_type(out, statement->as.local_binding.type)) {
            return false;
        }
        if (statement->as.local_binding.is_callable) {
            fprintf(out, " callable=");
            if (!write_callable_signature(out, &statement->as.local_binding.callable_signature)) {
                return false;
            }
        }
        fprintf(out, " final=%s span=", statement->as.local_binding.is_final ? "true" : "false");
        write_span(out, statement->as.local_binding.source_span);
        fputc('\n', out);
        write_indent(out, indent + 2);
        fprintf(out, "Init:\n");
        return dump_expression(out, statement->as.local_binding.initializer, indent + 4);
    case HIR_STMT_RETURN:
        write_indent(out, indent);
        fprintf(out, "Return span=");
        write_span(out, statement->source_span);
        fputc('\n', out);
        if (statement->as.return_expression) {
            return dump_expression(out, statement->as.return_expression, indent + 2);
        }
        return true;
    case HIR_STMT_EXIT:
        write_indent(out, indent);
        fprintf(out, "InvalidExit span=");
        write_span(out, statement->source_span);
        fputc('\n', out);
        return true;
    case HIR_STMT_THROW:
        write_indent(out, indent);
        fprintf(out, "Throw span=");
        write_span(out, statement->source_span);
        fputc('\n', out);
        return dump_expression(out, statement->as.throw_expression, indent + 2);
    case HIR_STMT_EXPRESSION:
        write_indent(out, indent);
        fprintf(out, "Expression span=");
        write_span(out, statement->source_span);
        fputc('\n', out);
        return dump_expression(out, statement->as.expression, indent + 2);
    }

    return false;
}

static bool dump_expression(FILE *out, const HirExpression *expression, int indent) {
    size_t i;

    if (!expression) {
        return false;
    }

    write_indent(out, indent);
    switch (expression->kind) {
    case HIR_EXPR_LITERAL:
        fprintf(out, "Literal kind=%d type=", expression->as.literal.kind);
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        if (expression->as.literal.kind == AST_LITERAL_BOOL) {
            fprintf(out, " value=%s", expression->as.literal.as.bool_value ? "true" : "false");
        } else if (expression->as.literal.kind != AST_LITERAL_NULL) {
            fprintf(out, " text=%s", expression->as.literal.as.text);
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        return true;
    case HIR_EXPR_TEMPLATE:
        fprintf(out, "Template type=");
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        for (i = 0; i < expression->as.template_parts.count; i++) {
            write_indent(out, indent + 2);
            if (expression->as.template_parts.items[i].kind == AST_TEMPLATE_PART_TEXT) {
                fprintf(out, "TextPart text=%s\n", expression->as.template_parts.items[i].as.text);
            } else {
                fprintf(out, "ExpressionPart\n");
                if (!dump_expression(out,
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
        if (!write_checked_type(out, expression->as.symbol.type)) {
            return false;
        }
        if (expression->is_callable) {
            fprintf(out, " callable=");
            if (!write_callable_signature(out, &expression->callable_signature)) {
                return false;
            }
        }
        fprintf(out, " span=");
        write_span(out, expression->as.symbol.source_span);
        fputc('\n', out);
        return true;
    case HIR_EXPR_LAMBDA:
        fprintf(out, "Lambda type=");
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " callable=");
        if (!write_callable_signature(out, &expression->callable_signature)) {
            return false;
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        write_indent(out, indent + 2);
        fprintf(out, "Parameters:\n");
        for (i = 0; i < expression->as.lambda.parameters.count; i++) {
            write_indent(out, indent + 4);
            fprintf(out, "Param name=%s type=", expression->as.lambda.parameters.items[i].name);
            if (!write_checked_type(out, expression->as.lambda.parameters.items[i].type)) {
                return false;
            }
            fprintf(out, " span=");
            write_span(out, expression->as.lambda.parameters.items[i].source_span);
            fputc('\n', out);
        }
        write_indent(out, indent + 2);
        fprintf(out, "Body:\n");
        return dump_block(out, expression->as.lambda.body, indent + 4);
    case HIR_EXPR_ASSIGNMENT:
        fprintf(out, "Assignment op=%d type=", expression->as.assignment.operator);
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        write_indent(out, indent + 2);
        fprintf(out, "Target:\n");
        if (!dump_expression(out, expression->as.assignment.target, indent + 4)) {
            return false;
        }
        write_indent(out, indent + 2);
        fprintf(out, "Value:\n");
        return dump_expression(out, expression->as.assignment.value, indent + 4);
    case HIR_EXPR_TERNARY:
        fprintf(out, "Ternary type=");
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        write_indent(out, indent + 2);
        fprintf(out, "Condition:\n");
        if (!dump_expression(out, expression->as.ternary.condition, indent + 4)) {
            return false;
        }
        write_indent(out, indent + 2);
        fprintf(out, "Then:\n");
        if (!dump_expression(out, expression->as.ternary.then_branch, indent + 4)) {
            return false;
        }
        write_indent(out, indent + 2);
        fprintf(out, "Else:\n");
        return dump_expression(out, expression->as.ternary.else_branch, indent + 4);
    case HIR_EXPR_BINARY:
        fprintf(out, "Binary op=%s type=", binary_operator_name_text(expression->as.binary.operator));
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        write_indent(out, indent + 2);
        fprintf(out, "Left:\n");
        if (!dump_expression(out, expression->as.binary.left, indent + 4)) {
            return false;
        }
        write_indent(out, indent + 2);
        fprintf(out, "Right:\n");
        return dump_expression(out, expression->as.binary.right, indent + 4);
    case HIR_EXPR_UNARY:
        fprintf(out, "Unary op=%d type=", expression->as.unary.operator);
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        return dump_expression(out, expression->as.unary.operand, indent + 2);
    case HIR_EXPR_CALL:
        fprintf(out, "Call type=");
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        write_indent(out, indent + 2);
        fprintf(out, "Callee:\n");
        if (!dump_expression(out, expression->as.call.callee, indent + 4)) {
            return false;
        }
        for (i = 0; i < expression->as.call.argument_count; i++) {
            write_indent(out, indent + 2);
            fprintf(out, "Arg %zu:\n", i + 1);
            if (!dump_expression(out, expression->as.call.arguments[i], indent + 4)) {
                return false;
            }
        }
        return true;
    case HIR_EXPR_INDEX:
        fprintf(out, "Index type=");
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        write_indent(out, indent + 2);
        fprintf(out, "Target:\n");
        if (!dump_expression(out, expression->as.index.target, indent + 4)) {
            return false;
        }
        write_indent(out, indent + 2);
        fprintf(out, "Index:\n");
        return dump_expression(out, expression->as.index.index, indent + 4);
    case HIR_EXPR_MEMBER:
        fprintf(out, "Member name=%s type=", expression->as.member.member);
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        if (expression->is_callable) {
            fprintf(out, " callable=");
            if (!write_callable_signature(out, &expression->callable_signature)) {
                return false;
            }
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        return dump_expression(out, expression->as.member.target, indent + 2);
    case HIR_EXPR_CAST:
        fprintf(out, "Cast target=");
        if (!write_checked_type(out, expression->as.cast.target_type)) {
            return false;
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        return dump_expression(out, expression->as.cast.expression, indent + 2);
    case HIR_EXPR_ARRAY_LITERAL:
        fprintf(out, "ArrayLiteral type=");
        if (!write_checked_type(out, expression->type)) {
            return false;
        }
        fprintf(out, " span=");
        write_span(out, expression->source_span);
        fputc('\n', out);
        for (i = 0; i < expression->as.array_literal.element_count; i++) {
            write_indent(out, indent + 2);
            fprintf(out, "Element %zu:\n", i + 1);
            if (!dump_expression(out, expression->as.array_literal.elements[i], indent + 4)) {
                return false;
            }
        }
        return true;
    }

    return false;
}