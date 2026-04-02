#include "ast_dump.h"

#include <ctype.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char   *data;
    size_t length;
    size_t capacity;
} AstDumpBuilder;

static bool builder_reserve(AstDumpBuilder *builder, size_t needed);
static bool builder_append_n(AstDumpBuilder *builder, const char *text, size_t length);
static bool builder_append(AstDumpBuilder *builder, const char *text);
static bool builder_append_char(AstDumpBuilder *builder, char value);
static bool builder_appendf(AstDumpBuilder *builder, const char *format, ...);
static bool builder_indent(AstDumpBuilder *builder, int indent);
static bool builder_start_line(AstDumpBuilder *builder, int indent);
static bool builder_finish_line(AstDumpBuilder *builder);
static bool builder_append_quoted(AstDumpBuilder *builder, const char *text);

static const char *modifier_name(AstModifier modifier);
static const char *primitive_type_name(AstPrimitiveType primitive);
static const char *assignment_operator_name(AstAssignmentOperator operator);
static const char *binary_operator_name(AstBinaryOperator operator);
static const char *unary_operator_name(AstUnaryOperator operator);

static bool dump_program_node(AstDumpBuilder *builder, const AstProgram *program, int indent);
static bool dump_top_level_decl(AstDumpBuilder *builder, const AstTopLevelDecl *decl,
                                int indent);
static bool dump_block(AstDumpBuilder *builder, const AstBlock *block, int indent);
static bool dump_statement(AstDumpBuilder *builder, const AstStatement *statement,
                           int indent);
static bool dump_expression_node(AstDumpBuilder *builder,
                                 const AstExpression *expression, int indent);
static bool dump_literal(AstDumpBuilder *builder, const AstLiteral *literal, int indent);
static bool dump_parameter_list(AstDumpBuilder *builder,
                                const AstParameterList *parameters, int indent);
static bool dump_expression_label(AstDumpBuilder *builder, int indent,
                                  const char *label,
                                  const AstExpression *expression);
static bool dump_block_label(AstDumpBuilder *builder, int indent,
                             const char *label, const AstBlock *block);
static bool dump_qualified_name(AstDumpBuilder *builder, const AstQualifiedName *name);
static bool dump_type(AstDumpBuilder *builder, const AstType *type, bool is_inferred);
static bool dump_modifiers(AstDumpBuilder *builder, const AstModifier *modifiers,
                           size_t count);

static bool builder_reserve(AstDumpBuilder *builder, size_t needed) {
    char *resized;
    size_t new_capacity;

    if (needed <= builder->capacity) {
        return true;
    }

    new_capacity = (builder->capacity == 0) ? 64 : builder->capacity;
    while (new_capacity < needed) {
        if (new_capacity > SIZE_MAX / 2) {
            new_capacity = needed;
            break;
        }
        new_capacity *= 2;
    }

    resized = realloc(builder->data, new_capacity);
    if (!resized) {
        return false;
    }

    builder->data = resized;
    builder->capacity = new_capacity;
    return true;
}

static bool builder_append_n(AstDumpBuilder *builder, const char *text, size_t length) {
    if (!builder || !text) {
        return false;
    }

    if (!builder_reserve(builder, builder->length + length + 1)) {
        return false;
    }

    if (length > 0) {
        memcpy(builder->data + builder->length, text, length);
    }
    builder->length += length;
    builder->data[builder->length] = '\0';
    return true;
}

static bool builder_append(AstDumpBuilder *builder, const char *text) {
    return builder_append_n(builder, text, strlen(text));
}

static bool builder_append_char(AstDumpBuilder *builder, char value) {
    return builder_append_n(builder, &value, 1);
}

static bool builder_appendf(AstDumpBuilder *builder, const char *format, ...) {
    int written;
    va_list args;
    va_list copied_args;

    va_start(args, format);
    va_copy(copied_args, args);
    written = vsnprintf(NULL, 0, format, copied_args);
    va_end(copied_args);

    if (written < 0) {
        va_end(args);
        return false;
    }

    if (!builder_reserve(builder, builder->length + (size_t)written + 1)) {
        va_end(args);
        return false;
    }

    vsnprintf(builder->data + builder->length,
              builder->capacity - builder->length, format, args);
    builder->length += (size_t)written;
    va_end(args);
    return true;
}

static bool builder_indent(AstDumpBuilder *builder, int indent) {
    int i;

    for (i = 0; i < indent; i++) {
        if (!builder_append(builder, "  ")) {
            return false;
        }
    }

    return true;
}

static bool builder_start_line(AstDumpBuilder *builder, int indent) {
    return builder_indent(builder, indent);
}

static bool builder_finish_line(AstDumpBuilder *builder) {
    return builder_append_char(builder, '\n');
}

static bool builder_append_quoted(AstDumpBuilder *builder, const char *text) {
    const unsigned char *current = (const unsigned char *)text;

    if (!builder_append_char(builder, '"')) {
        return false;
    }

    while (current && *current != '\0') {
        switch (*current) {
        case '\\':
            if (!builder_append(builder, "\\\\")) {
                return false;
            }
            break;
        case '"':
            if (!builder_append(builder, "\\\"")) {
                return false;
            }
            break;
        case '\n':
            if (!builder_append(builder, "\\n")) {
                return false;
            }
            break;
        case '\r':
            if (!builder_append(builder, "\\r")) {
                return false;
            }
            break;
        case '\t':
            if (!builder_append(builder, "\\t")) {
                return false;
            }
            break;
        default:
            if (isprint(*current)) {
                if (!builder_append_char(builder, (char)*current)) {
                    return false;
                }
            } else if (!builder_appendf(builder, "\\x%02X", *current)) {
                return false;
            }
            break;
        }

        current++;
    }

    return builder_append_char(builder, '"');
}

static const char *modifier_name(AstModifier modifier) {
    switch (modifier) {
    case AST_MODIFIER_PUBLIC:
        return "public";
    case AST_MODIFIER_PRIVATE:
        return "private";
    case AST_MODIFIER_FINAL:
        return "final";
    }

    return "unknown";
}

static const char *primitive_type_name(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_INT8:
        return "int8";
    case AST_PRIMITIVE_INT16:
        return "int16";
    case AST_PRIMITIVE_INT32:
        return "int32";
    case AST_PRIMITIVE_INT64:
        return "int64";
    case AST_PRIMITIVE_UINT8:
        return "uint8";
    case AST_PRIMITIVE_UINT16:
        return "uint16";
    case AST_PRIMITIVE_UINT32:
        return "uint32";
    case AST_PRIMITIVE_UINT64:
        return "uint64";
    case AST_PRIMITIVE_FLOAT32:
        return "float32";
    case AST_PRIMITIVE_FLOAT64:
        return "float64";
    case AST_PRIMITIVE_BOOL:
        return "bool";
    case AST_PRIMITIVE_CHAR:
        return "char";
    case AST_PRIMITIVE_STRING:
        return "string";
    }

    return "unknown";
}

static const char *assignment_operator_name(AstAssignmentOperator operator) {
    switch (operator) {
    case AST_ASSIGN_OP_ASSIGN:
        return "=";
    case AST_ASSIGN_OP_ADD:
        return "+=";
    case AST_ASSIGN_OP_SUBTRACT:
        return "-=";
    case AST_ASSIGN_OP_MULTIPLY:
        return "*=";
    case AST_ASSIGN_OP_DIVIDE:
        return "/=";
    case AST_ASSIGN_OP_MODULO:
        return "%=";
    case AST_ASSIGN_OP_BIT_AND:
        return "&=";
    case AST_ASSIGN_OP_BIT_OR:
        return "|=";
    case AST_ASSIGN_OP_BIT_XOR:
        return "^=";
    case AST_ASSIGN_OP_SHIFT_LEFT:
        return "<<=";
    case AST_ASSIGN_OP_SHIFT_RIGHT:
        return ">>=";
    }

    return "?=";
}

static const char *binary_operator_name(AstBinaryOperator operator) {
    switch (operator) {
    case AST_BINARY_OP_LOGICAL_OR:
        return "||";
    case AST_BINARY_OP_LOGICAL_AND:
        return "&&";
    case AST_BINARY_OP_BIT_OR:
        return "|";
    case AST_BINARY_OP_BIT_NAND:
        return "~&";
    case AST_BINARY_OP_BIT_XOR:
        return "^";
    case AST_BINARY_OP_BIT_XNOR:
        return "~^";
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

static const char *unary_operator_name(AstUnaryOperator operator) {
    switch (operator) {
    case AST_UNARY_OP_LOGICAL_NOT:
        return "!";
    case AST_UNARY_OP_BITWISE_NOT:
        return "~";
    case AST_UNARY_OP_NEGATE:
        return "-";
    case AST_UNARY_OP_PLUS:
        return "+";
    }

    return "?";
}

static bool dump_qualified_name(AstDumpBuilder *builder, const AstQualifiedName *name) {
    size_t i;

    if (!name || name->count == 0) {
        return builder_append(builder, "<empty>");
    }

    for (i = 0; i < name->count; i++) {
        if (i > 0 && !builder_append_char(builder, '.')) {
            return false;
        }
        if (!builder_append(builder, name->segments[i] ? name->segments[i] : "<null>")) {
            return false;
        }
    }

    return true;
}

static bool dump_type(AstDumpBuilder *builder, const AstType *type, bool is_inferred) {
    size_t i;

    if (is_inferred) {
        return builder_append(builder, "var");
    }

    if (!type) {
        return builder_append(builder, "<null-type>");
    }

    if (type->kind == AST_TYPE_VOID) {
        if (!builder_append(builder, "void")) {
            return false;
        }
    } else {
        if (!builder_append(builder, primitive_type_name(type->primitive))) {
            return false;
        }
    }

    for (i = 0; i < type->dimension_count; i++) {
        if (!builder_append_char(builder, '[')) {
            return false;
        }
        if (type->dimensions[i].has_size &&
            !builder_append(builder,
                            type->dimensions[i].size_literal
                                ? type->dimensions[i].size_literal
                                : "<null>")) {
            return false;
        }
        if (!builder_append_char(builder, ']')) {
            return false;
        }
    }

    return true;
}

static bool dump_modifiers(AstDumpBuilder *builder, const AstModifier *modifiers,
                           size_t count) {
    size_t i;

    if (!builder_append_char(builder, '[')) {
        return false;
    }

    for (i = 0; i < count; i++) {
        if (i > 0 && !builder_append(builder, ", ")) {
            return false;
        }
        if (!builder_append(builder, modifier_name(modifiers[i]))) {
            return false;
        }
    }

    return builder_append_char(builder, ']');
}

static bool dump_parameter_list(AstDumpBuilder *builder,
                                const AstParameterList *parameters, int indent) {
    size_t i;

    if (!parameters || parameters->count == 0) {
        return builder_start_line(builder, indent) &&
               builder_append(builder, "Parameters: []") &&
               builder_finish_line(builder);
    }

    if (!(builder_start_line(builder, indent) && builder_append(builder, "Parameters:") &&
          builder_finish_line(builder))) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        if (!(builder_start_line(builder, indent + 1) &&
              builder_append(builder, "Parameter name=") &&
              builder_append(builder,
                             parameters->items[i].name ? parameters->items[i].name : "<null>") &&
              builder_append(builder, " type=") &&
              dump_type(builder, &parameters->items[i].type, false) &&
              builder_finish_line(builder))) {
            return false;
        }
    }

    return true;
}

static bool dump_expression_label(AstDumpBuilder *builder, int indent,
                                  const char *label,
                                  const AstExpression *expression) {
    if (!(builder_start_line(builder, indent) && builder_append(builder, label) &&
          builder_append_char(builder, ':') && builder_finish_line(builder))) {
        return false;
    }

    return dump_expression_node(builder, expression, indent + 1);
}

static bool dump_block_label(AstDumpBuilder *builder, int indent,
                             const char *label, const AstBlock *block) {
    if (!(builder_start_line(builder, indent) && builder_append(builder, label) &&
          builder_append_char(builder, ':') && builder_finish_line(builder))) {
        return false;
    }

    return dump_block(builder, block, indent + 1);
}

static bool dump_literal(AstDumpBuilder *builder, const AstLiteral *literal, int indent) {
    size_t i;

    if (!literal) {
        return builder_start_line(builder, indent) &&
               builder_append(builder, "<null literal>") &&
               builder_finish_line(builder);
    }

    switch (literal->kind) {
    case AST_LITERAL_INTEGER:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "IntegerLiteral value=") &&
               builder_append(builder, literal->as.text ? literal->as.text : "<null>") &&
               builder_finish_line(builder);
    case AST_LITERAL_FLOAT:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "FloatLiteral value=") &&
               builder_append(builder, literal->as.text ? literal->as.text : "<null>") &&
               builder_finish_line(builder);
    case AST_LITERAL_BOOL:
        return builder_start_line(builder, indent) &&
               builder_append(builder,
                              literal->as.bool_value ? "BoolLiteral value=true"
                                                     : "BoolLiteral value=false") &&
               builder_finish_line(builder);
    case AST_LITERAL_CHAR:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "CharLiteral value=") &&
               builder_append(builder, literal->as.text ? literal->as.text : "<null>") &&
               builder_finish_line(builder);
    case AST_LITERAL_STRING:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "StringLiteral value=") &&
               builder_append(builder, literal->as.text ? literal->as.text : "<null>") &&
               builder_finish_line(builder);
    case AST_LITERAL_TEMPLATE:
        if (!(builder_start_line(builder, indent) && builder_append(builder, "TemplateLiteral") &&
              builder_finish_line(builder))) {
            return false;
        }

        for (i = 0; i < literal->as.template_parts.count; i++) {
            const AstTemplatePart *part = &literal->as.template_parts.items[i];

            if (part->kind == AST_TEMPLATE_PART_TEXT) {
                if (!(builder_start_line(builder, indent + 1) &&
                      builder_append(builder, "TextPart ") &&
                      builder_append_quoted(builder,
                                            part->as.text ? part->as.text : "") &&
                      builder_finish_line(builder))) {
                    return false;
                }
            } else if (!dump_expression_label(builder, indent + 1, "ExpressionPart",
                                              part->as.expression)) {
                return false;
            }
        }

        return true;
    case AST_LITERAL_NULL:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "NullLiteral") && builder_finish_line(builder);
    }

    return false;
}

static bool dump_expression_node(AstDumpBuilder *builder,
                                 const AstExpression *expression, int indent) {
    size_t i;

    if (!expression) {
        return builder_start_line(builder, indent) &&
               builder_append(builder, "<null expression>") &&
               builder_finish_line(builder);
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        return dump_literal(builder, &expression->as.literal, indent);

    case AST_EXPR_IDENTIFIER:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "IdentifierExpr name=") &&
               builder_append(builder,
                              expression->as.identifier ? expression->as.identifier : "<null>") &&
               builder_finish_line(builder);

    case AST_EXPR_LAMBDA:
        if (!(builder_start_line(builder, indent) && builder_append(builder, "LambdaExpr") &&
              builder_finish_line(builder) &&
              dump_parameter_list(builder, &expression->as.lambda.parameters, indent + 1))) {
            return false;
        }

        if (expression->as.lambda.body.kind == AST_LAMBDA_BODY_BLOCK) {
            return dump_block_label(builder, indent + 1, "BodyBlock",
                                    expression->as.lambda.body.as.block);
        }

        return dump_expression_label(builder, indent + 1, "BodyExpr",
                                     expression->as.lambda.body.as.expression);

    case AST_EXPR_ASSIGNMENT:
        if (!(builder_start_line(builder, indent) &&
              builder_append(builder, "AssignmentExpr op=") &&
              builder_append_quoted(builder,
                                    assignment_operator_name(
                                        expression->as.assignment.operator)) &&
              builder_finish_line(builder))) {
            return false;
        }

        return dump_expression_label(builder, indent + 1, "Target",
                                     expression->as.assignment.target) &&
               dump_expression_label(builder, indent + 1, "Value",
                                     expression->as.assignment.value);

    case AST_EXPR_TERNARY:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "TernaryExpr") && builder_finish_line(builder) &&
               dump_expression_label(builder, indent + 1, "Condition",
                                     expression->as.ternary.condition) &&
               dump_expression_label(builder, indent + 1, "Then",
                                     expression->as.ternary.then_branch) &&
               dump_expression_label(builder, indent + 1, "Else",
                                     expression->as.ternary.else_branch);

    case AST_EXPR_BINARY:
        if (!(builder_start_line(builder, indent) && builder_append(builder, "BinaryExpr op=") &&
              builder_append_quoted(builder,
                                    binary_operator_name(expression->as.binary.operator)) &&
              builder_finish_line(builder))) {
            return false;
        }

        return dump_expression_label(builder, indent + 1, "Left",
                                     expression->as.binary.left) &&
               dump_expression_label(builder, indent + 1, "Right",
                                     expression->as.binary.right);

    case AST_EXPR_UNARY:
        if (!(builder_start_line(builder, indent) && builder_append(builder, "UnaryExpr op=") &&
              builder_append_quoted(builder,
                                    unary_operator_name(expression->as.unary.operator)) &&
              builder_finish_line(builder))) {
            return false;
        }

        return dump_expression_label(builder, indent + 1, "Operand",
                                     expression->as.unary.operand);

    case AST_EXPR_CALL:
        if (!(builder_start_line(builder, indent) && builder_append(builder, "CallExpr") &&
              builder_finish_line(builder) &&
              dump_expression_label(builder, indent + 1, "Callee",
                                    expression->as.call.callee))) {
            return false;
        }

        if (expression->as.call.arguments.count == 0) {
            return builder_start_line(builder, indent + 1) &&
                   builder_append(builder, "Arguments: []") &&
                   builder_finish_line(builder);
        }

        if (!(builder_start_line(builder, indent + 1) && builder_append(builder, "Arguments:") &&
              builder_finish_line(builder))) {
            return false;
        }

        for (i = 0; i < expression->as.call.arguments.count; i++) {
            if (!dump_expression_node(builder, expression->as.call.arguments.items[i],
                                      indent + 2)) {
                return false;
            }
        }

        return true;

    case AST_EXPR_INDEX:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "IndexExpr") && builder_finish_line(builder) &&
               dump_expression_label(builder, indent + 1, "Target",
                                     expression->as.index.target) &&
               dump_expression_label(builder, indent + 1, "Index",
                                     expression->as.index.index);

    case AST_EXPR_MEMBER:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "MemberExpr name=") &&
               builder_append(builder,
                              expression->as.member.member ? expression->as.member.member : "<null>") &&
               builder_finish_line(builder) &&
               dump_expression_label(builder, indent + 1, "Target",
                                     expression->as.member.target);

    case AST_EXPR_CAST:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "CastExpr type=") &&
               builder_append(builder,
                              primitive_type_name(expression->as.cast.target_type)) &&
               builder_finish_line(builder) &&
               dump_expression_label(builder, indent + 1, "Expression",
                                     expression->as.cast.expression);

    case AST_EXPR_ARRAY_LITERAL:
        if (!(builder_start_line(builder, indent) && builder_append(builder, "ArrayLiteralExpr") &&
              builder_finish_line(builder))) {
            return false;
        }

        if (expression->as.array_literal.elements.count == 0) {
            return builder_start_line(builder, indent + 1) &&
                   builder_append(builder, "Elements: []") &&
                   builder_finish_line(builder);
        }

        if (!(builder_start_line(builder, indent + 1) && builder_append(builder, "Elements:") &&
              builder_finish_line(builder))) {
            return false;
        }

        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            if (!dump_expression_node(builder,
                                      expression->as.array_literal.elements.items[i],
                                      indent + 2)) {
                return false;
            }
        }

        return true;

    case AST_EXPR_GROUPING:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "GroupingExpr") && builder_finish_line(builder) &&
               dump_expression_label(builder, indent + 1, "Expression",
                                     expression->as.grouping.inner);
    }

    return false;
}

static bool dump_statement(AstDumpBuilder *builder, const AstStatement *statement,
                           int indent) {
    if (!statement) {
        return builder_start_line(builder, indent) &&
               builder_append(builder, "<null statement>") &&
               builder_finish_line(builder);
    }

    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "LocalBindingStmt name=") &&
               builder_append(builder,
                              statement->as.local_binding.name
                                  ? statement->as.local_binding.name
                                  : "<null>") &&
               builder_append(builder, " type=") &&
               dump_type(builder, &statement->as.local_binding.declared_type,
                         statement->as.local_binding.is_inferred_type) &&
               builder_append(builder,
                              statement->as.local_binding.is_final
                                  ? " final=true"
                                  : " final=false") &&
               builder_finish_line(builder) &&
               dump_expression_label(builder, indent + 1, "Initializer",
                                     statement->as.local_binding.initializer);

    case AST_STMT_RETURN:
        if (!(builder_start_line(builder, indent) && builder_append(builder, "ReturnStmt") &&
              builder_finish_line(builder))) {
            return false;
        }

        if (statement->as.return_expression) {
            return dump_expression_label(builder, indent + 1, "Value",
                                         statement->as.return_expression);
        }

        return true;

    case AST_STMT_EXIT:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "ExitStmt") && builder_finish_line(builder);

    case AST_STMT_THROW:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "ThrowStmt") && builder_finish_line(builder) &&
               dump_expression_label(builder, indent + 1, "Value",
                                     statement->as.throw_expression);

    case AST_STMT_EXPRESSION:
        return builder_start_line(builder, indent) &&
               builder_append(builder, "ExpressionStmt") && builder_finish_line(builder) &&
               dump_expression_label(builder, indent + 1, "Expression",
                                     statement->as.expression);
    }

    return false;
}

static bool dump_block(AstDumpBuilder *builder, const AstBlock *block, int indent) {
    size_t i;

    if (!block) {
        return builder_start_line(builder, indent) &&
               builder_append(builder, "<null block>") && builder_finish_line(builder);
    }

    if (!(builder_start_line(builder, indent) && builder_append(builder, "Block") &&
          builder_finish_line(builder))) {
        return false;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (!dump_statement(builder, block->statements[i], indent + 1)) {
            return false;
        }
    }

    return true;
}

static bool dump_top_level_decl(AstDumpBuilder *builder, const AstTopLevelDecl *decl,
                                int indent) {
    if (!decl) {
        return builder_start_line(builder, indent) &&
               builder_append(builder, "<null top-level declaration>") &&
               builder_finish_line(builder);
    }

    switch (decl->kind) {
    case AST_TOP_LEVEL_START:
        if (!(builder_start_line(builder, indent) && builder_append(builder, "StartDecl") &&
              builder_finish_line(builder) &&
              dump_parameter_list(builder, &decl->as.start_decl.parameters, indent + 1))) {
            return false;
        }

        if (decl->as.start_decl.body.kind == AST_LAMBDA_BODY_BLOCK) {
            return dump_block_label(builder, indent + 1, "BodyBlock",
                                    decl->as.start_decl.body.as.block);
        }

        return dump_expression_label(builder, indent + 1, "BodyExpr",
                                     decl->as.start_decl.body.as.expression);

    case AST_TOP_LEVEL_BINDING:
        if (!(builder_start_line(builder, indent) &&
              builder_append(builder, "BindingDecl name=") &&
              builder_append(builder,
                             decl->as.binding_decl.name ? decl->as.binding_decl.name : "<null>") &&
              builder_append(builder, " type=") &&
              dump_type(builder, &decl->as.binding_decl.declared_type,
                        decl->as.binding_decl.is_inferred_type) &&
              builder_append(builder, " modifiers=") &&
              dump_modifiers(builder, decl->as.binding_decl.modifiers,
                             decl->as.binding_decl.modifier_count) &&
              builder_finish_line(builder))) {
            return false;
        }

        return dump_expression_label(builder, indent + 1, "Initializer",
                                     decl->as.binding_decl.initializer);
    }

    return false;
}

static bool dump_program_node(AstDumpBuilder *builder, const AstProgram *program, int indent) {
    size_t i;

    if (!program) {
        return builder_start_line(builder, indent) &&
               builder_append(builder, "<null program>") && builder_finish_line(builder);
    }

    if (!(builder_start_line(builder, indent) && builder_append(builder, "Program") &&
          builder_finish_line(builder))) {
        return false;
    }

    if (program->has_package) {
        if (!(builder_start_line(builder, indent + 1) &&
              builder_append(builder, "PackageDecl: ") &&
              dump_qualified_name(builder, &program->package_name) &&
              builder_finish_line(builder))) {
            return false;
        }
    }

    for (i = 0; i < program->import_count; i++) {
        if (!(builder_start_line(builder, indent + 1) &&
              builder_append(builder, "ImportDecl: ") &&
              dump_qualified_name(builder, &program->imports[i]) &&
              builder_finish_line(builder))) {
            return false;
        }
    }

    for (i = 0; i < program->top_level_count; i++) {
        if (!dump_top_level_decl(builder, program->top_level_decls[i], indent + 1)) {
            return false;
        }
    }

    return true;
}

bool ast_dump_program(FILE *out, const AstProgram *program) {
    char *dump;
    int status;

    if (!out) {
        return false;
    }

    dump = ast_dump_program_to_string(program);
    if (!dump) {
        return false;
    }

    status = fputs(dump, out);
    free(dump);
    return status >= 0;
}

bool ast_dump_expression(FILE *out, const AstExpression *expression) {
    char *dump;
    int status;

    if (!out) {
        return false;
    }

    dump = ast_dump_expression_to_string(expression);
    if (!dump) {
        return false;
    }

    status = fputs(dump, out);
    free(dump);
    return status >= 0;
}

char *ast_dump_program_to_string(const AstProgram *program) {
    AstDumpBuilder builder;

    memset(&builder, 0, sizeof(builder));
    if (!builder_reserve(&builder, 1)) {
        return NULL;
    }
    builder.data[0] = '\0';

    if (!dump_program_node(&builder, program, 0)) {
        free(builder.data);
        return NULL;
    }

    return builder.data;
}

char *ast_dump_expression_to_string(const AstExpression *expression) {
    AstDumpBuilder builder;

    memset(&builder, 0, sizeof(builder));
    if (!builder_reserve(&builder, 1)) {
        return NULL;
    }
    builder.data[0] = '\0';

    if (!dump_expression_node(&builder, expression, 0)) {
        free(builder.data);
        return NULL;
    }

    return builder.data;
}