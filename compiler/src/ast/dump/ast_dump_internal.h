#ifndef CALYNDA_AST_DUMP_INTERNAL_H
#define CALYNDA_AST_DUMP_INTERNAL_H

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

/* Builder utilities */
bool ast_dump_builder_reserve(AstDumpBuilder *builder, size_t needed);
bool ast_dump_builder_append_n(AstDumpBuilder *builder, const char *text, size_t length);
bool ast_dump_builder_append(AstDumpBuilder *builder, const char *text);
bool ast_dump_builder_append_char(AstDumpBuilder *builder, char value);
bool ast_dump_builder_appendf(AstDumpBuilder *builder, const char *format, ...);
bool ast_dump_builder_indent(AstDumpBuilder *builder, int indent);
bool ast_dump_builder_start_line(AstDumpBuilder *builder, int indent);
bool ast_dump_builder_finish_line(AstDumpBuilder *builder);
bool ast_dump_builder_append_quoted(AstDumpBuilder *builder, const char *text);

/* Name lookup tables */
const char *ast_dump_modifier_name(AstModifier modifier);
const char *ast_dump_primitive_type_name(AstPrimitiveType primitive);
const char *ast_dump_assignment_operator_name(AstAssignmentOperator op);
const char *ast_dump_binary_operator_name(AstBinaryOperator op);
const char *ast_dump_unary_operator_name(AstUnaryOperator op);

/* Node dump helpers */
bool ast_dump_qualified_name(AstDumpBuilder *builder, const AstQualifiedName *name);
bool ast_dump_type(AstDumpBuilder *builder, const AstType *type, bool is_inferred);
bool ast_dump_modifiers(AstDumpBuilder *builder, const AstModifier *modifiers, size_t count);
bool ast_dump_parameter_list(AstDumpBuilder *builder, const AstParameterList *parameters, int indent);
bool ast_dump_expression_label(AstDumpBuilder *builder, int indent, const char *label, const AstExpression *expression);
bool ast_dump_block_label(AstDumpBuilder *builder, int indent, const char *label, const AstBlock *block);
bool ast_dump_literal(AstDumpBuilder *builder, const AstLiteral *literal, int indent);
bool ast_dump_expression_node(AstDumpBuilder *builder, const AstExpression *expression, int indent);
bool ast_dump_statement(AstDumpBuilder *builder, const AstStatement *statement, int indent);
bool ast_dump_block(AstDumpBuilder *builder, const AstBlock *block, int indent);
bool ast_dump_top_level_decl(AstDumpBuilder *builder, const AstTopLevelDecl *decl, int indent);
bool ast_dump_program_node(AstDumpBuilder *builder, const AstProgram *program, int indent);

#endif
