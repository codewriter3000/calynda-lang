#ifndef CALYNDA_PARSER_INTERNAL_H
#define CALYNDA_PARSER_INTERNAL_H

#include "parser.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef AstExpression *(*ParseExpressionFn)(Parser *parser);

typedef struct {
    TokenType         token_type;
    AstAssignmentOperator operator;
} AssignmentOperatorMapping;

typedef struct {
    TokenType         token_type;
    AstBinaryOperator operator;
} BinaryOperatorMapping;

typedef struct {
    TokenType        token_type;
    AstUnaryOperator operator;
} UnaryOperatorMapping;

/* parser_utils.c — token buffer, matching, source spans, text */
void parser_set_error(Parser *parser, Token token, const char *format, ...);
void parser_set_oom_error(Parser *parser);
bool parser_append_token(Parser *parser, Token token);
const Token *parser_token_at(const Parser *parser, size_t index);
const Token *parser_current_token(const Parser *parser);
const Token *parser_previous_token(const Parser *parser);
bool parser_is_at_end(const Parser *parser);
const Token *parser_advance(Parser *parser);
bool parser_check(const Parser *parser, TokenType type);
bool parser_match(Parser *parser, TokenType type);
bool parser_consume(Parser *parser, TokenType type, const char *message);
bool parser_consume_gt(Parser *parser, const char *message);
AstSourceSpan parser_source_span(const Token *token);
void parser_extend_source_span(AstSourceSpan *span, const Token *token);
char *parser_copy_token_text(const Token *token);
char *parser_consume_identifier(Parser *parser, const char *message);
bool parser_add_parameter(Parser *parser, AstParameterList *list,
                          AstParameter *parameter);
bool parser_add_expression(Parser *parser, AstExpressionList *list,
                           AstExpression *expression);
bool parser_add_statement(Parser *parser, AstBlock *block,
                          AstStatement *statement);
bool parser_add_top_level_decl(Parser *parser, AstProgram *program,
                               AstTopLevelDecl *decl);
bool parser_add_import(Parser *parser, AstProgram *program,
                       AstImportDecl *import_decl);
bool parser_add_modifier(Parser *parser, AstBindingDecl *decl,
                         AstModifier modifier);
bool parser_add_template_text(Parser *parser, AstLiteral *literal,
                              const Token *token);
bool parser_add_template_expression(Parser *parser, AstLiteral *literal,
                                    AstExpression *expression);

/* parser_types.c — type, name, parameter, lambda body parsing */
bool parser_parse_qualified_name(Parser *parser, AstQualifiedName *name);
bool parser_parse_type(Parser *parser, AstType *type);
bool parser_parse_generic_args(Parser *parser, AstType *type);
bool parser_parse_parameter_list(Parser *parser, AstParameterList *list,
                                 bool allow_empty);
bool parser_parse_lambda_body(Parser *parser, AstLambdaBody *body);
bool parser_parse_block_or_expression_body(Parser *parser, AstLambdaBody *body);

/* parser_decl.c — top-level declarations */
AstTopLevelDecl *parse_top_level_decl(Parser *parser);
AstTopLevelDecl *parse_start_decl(Parser *parser);
AstTopLevelDecl *parse_boot_decl(Parser *parser);
AstTopLevelDecl *parse_binding_decl(Parser *parser);
AstTopLevelDecl *parse_type_alias_decl(Parser *parser);
AstTopLevelDecl *parse_asm_decl(Parser *parser);
AstBlock *parse_block(Parser *parser);

/* parser_union.c */
AstTopLevelDecl *parse_union_decl(Parser *parser);

/* parser_layout.c */
AstTopLevelDecl *parse_layout_decl(Parser *parser);

/* parser_stmt.c */
AstStatement *parse_statement(Parser *parser);

/* parser_expr.c — expression entry + assignment + ternary + unary */
AstExpression *parse_expression_node(Parser *parser);
AstExpression *parse_lambda_expression(Parser *parser);
AstExpression *parse_assignment_expression(Parser *parser);
AstExpression *parse_ternary_expression(Parser *parser);
AstExpression *parse_unary_expression(Parser *parser);

/* parser_binary.c — binary operator precedence chain */
AstExpression *parse_logical_or_expression(Parser *parser);
AstExpression *parse_logical_and_expression(Parser *parser);
AstExpression *parse_bitwise_or_expression(Parser *parser);
AstExpression *parse_bitwise_nand_expression(Parser *parser);
AstExpression *parse_bitwise_xor_expression(Parser *parser);
AstExpression *parse_bitwise_xnor_expression(Parser *parser);
AstExpression *parse_bitwise_and_expression(Parser *parser);
AstExpression *parse_equality_expression(Parser *parser);
AstExpression *parse_relational_expression(Parser *parser);
AstExpression *parse_shift_expression(Parser *parser);
AstExpression *parse_additive_expression(Parser *parser);
AstExpression *parse_multiplicative_expression(Parser *parser);
AstExpression *parser_parse_binary_level(Parser *parser,
                                         ParseExpressionFn operand_parser,
                                         const BinaryOperatorMapping *mappings,
                                         size_t mapping_count);

/* parser_postfix.c — postfix + primary */
AstExpression *parse_postfix_expression(Parser *parser);
AstExpression *parse_primary_expression(Parser *parser);

/* parser_literals.c — array, cast, template literals */
AstExpression *parse_array_literal_expression(Parser *parser);
AstExpression *parse_cast_expression(Parser *parser);
AstExpression *parse_template_literal_expression(Parser *parser);

/* parser_lookahead.c — lookahead, type recognition, primitive mapping */
bool is_primitive_type_token(TokenType type);
bool is_type_start_token(TokenType type);
bool scan_generic_args_pattern(const Parser *parser, size_t *index);
bool scan_type_pattern(const Parser *parser, size_t *index);
bool looks_like_lambda_expression(const Parser *parser);
bool looks_like_local_binding_statement(const Parser *parser);
AstPrimitiveType primitive_type_from_token(TokenType type);
char *copy_template_segment_text(const Token *token);

#endif
