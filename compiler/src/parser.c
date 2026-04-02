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

static AstTopLevelDecl *parse_top_level_decl(Parser *parser);
static AstTopLevelDecl *parse_start_decl(Parser *parser);
static AstTopLevelDecl *parse_binding_decl(Parser *parser);
static AstBlock *parse_block(Parser *parser);
static AstStatement *parse_statement(Parser *parser);
static AstExpression *parse_expression_node(Parser *parser);
static AstExpression *parse_lambda_expression(Parser *parser);
static AstExpression *parse_assignment_expression(Parser *parser);
static AstExpression *parse_ternary_expression(Parser *parser);
static AstExpression *parse_logical_or_expression(Parser *parser);
static AstExpression *parse_logical_and_expression(Parser *parser);
static AstExpression *parse_bitwise_or_expression(Parser *parser);
static AstExpression *parse_bitwise_nand_expression(Parser *parser);
static AstExpression *parse_bitwise_xor_expression(Parser *parser);
static AstExpression *parse_bitwise_xnor_expression(Parser *parser);
static AstExpression *parse_bitwise_and_expression(Parser *parser);
static AstExpression *parse_equality_expression(Parser *parser);
static AstExpression *parse_relational_expression(Parser *parser);
static AstExpression *parse_shift_expression(Parser *parser);
static AstExpression *parse_additive_expression(Parser *parser);
static AstExpression *parse_multiplicative_expression(Parser *parser);
static AstExpression *parse_unary_expression(Parser *parser);
static AstExpression *parse_postfix_expression(Parser *parser);
static AstExpression *parse_primary_expression(Parser *parser);
static AstExpression *parse_array_literal_expression(Parser *parser);
static AstExpression *parse_cast_expression(Parser *parser);
static AstExpression *parse_template_literal_expression(Parser *parser);

static void parser_set_error(Parser *parser, Token token, const char *format, ...);
static void parser_set_oom_error(Parser *parser);
static bool parser_append_token(Parser *parser, Token token);
static const Token *parser_token_at(const Parser *parser, size_t index);
static const Token *parser_current_token(const Parser *parser);
static const Token *parser_previous_token(const Parser *parser);
static bool parser_is_at_end(const Parser *parser);
static const Token *parser_advance(Parser *parser);
static bool parser_check(const Parser *parser, TokenType type);
static bool parser_match(Parser *parser, TokenType type);
static bool parser_consume(Parser *parser, TokenType type, const char *message);
static AstSourceSpan parser_source_span(const Token *token);
static void parser_extend_source_span(AstSourceSpan *span, const Token *token);
static char *parser_copy_token_text(const Token *token);
static char *parser_consume_identifier(Parser *parser, const char *message);
static bool parser_add_parameter(Parser *parser, AstParameterList *list,
                                 AstParameter *parameter);
static bool parser_add_expression(Parser *parser, AstExpressionList *list,
                                  AstExpression *expression);
static bool parser_add_statement(Parser *parser, AstBlock *block,
                                 AstStatement *statement);
static bool parser_add_top_level_decl(Parser *parser, AstProgram *program,
                                      AstTopLevelDecl *decl);
static bool parser_add_import(Parser *parser, AstProgram *program,
                              AstQualifiedName *import_name);
static bool parser_add_modifier(Parser *parser, AstBindingDecl *decl,
                                AstModifier modifier);
static bool parser_add_template_text(Parser *parser, AstLiteral *literal,
                                     const Token *token);
static bool parser_add_template_expression(Parser *parser, AstLiteral *literal,
                                           AstExpression *expression);
static bool parser_parse_qualified_name(Parser *parser, AstQualifiedName *name);
static bool parser_parse_type(Parser *parser, AstType *type);
static bool parser_parse_parameter_list(Parser *parser, AstParameterList *list,
                                        bool allow_empty);
static bool parser_parse_lambda_body(Parser *parser, AstLambdaBody *body);
static bool parser_parse_block_or_expression_body(Parser *parser,
                                                  AstLambdaBody *body);
static AstExpression *parser_parse_binary_level(Parser *parser,
                                                ParseExpressionFn operand_parser,
                                                const BinaryOperatorMapping *mappings,
                                                size_t mapping_count);
static bool is_primitive_type_token(TokenType type);
static bool is_type_start_token(TokenType type);
static bool scan_type_pattern(const Parser *parser, size_t *index);
static bool looks_like_lambda_expression(const Parser *parser);
static bool looks_like_local_binding_statement(const Parser *parser);
static AstPrimitiveType primitive_type_from_token(TokenType type);
static char *copy_template_segment_text(const Token *token);

static const AssignmentOperatorMapping assignment_operators[] = {
    {TOK_ASSIGN, AST_ASSIGN_OP_ASSIGN},
    {TOK_PLUS_ASSIGN, AST_ASSIGN_OP_ADD},
    {TOK_MINUS_ASSIGN, AST_ASSIGN_OP_SUBTRACT},
    {TOK_STAR_ASSIGN, AST_ASSIGN_OP_MULTIPLY},
    {TOK_SLASH_ASSIGN, AST_ASSIGN_OP_DIVIDE},
    {TOK_PERCENT_ASSIGN, AST_ASSIGN_OP_MODULO},
    {TOK_AMP_ASSIGN, AST_ASSIGN_OP_BIT_AND},
    {TOK_PIPE_ASSIGN, AST_ASSIGN_OP_BIT_OR},
    {TOK_CARET_ASSIGN, AST_ASSIGN_OP_BIT_XOR},
    {TOK_LSHIFT_ASSIGN, AST_ASSIGN_OP_SHIFT_LEFT},
    {TOK_RSHIFT_ASSIGN, AST_ASSIGN_OP_SHIFT_RIGHT}
};

static const BinaryOperatorMapping logical_or_operators[] = {
    {TOK_LOGIC_OR, AST_BINARY_OP_LOGICAL_OR}
};

static const BinaryOperatorMapping logical_and_operators[] = {
    {TOK_LOGIC_AND, AST_BINARY_OP_LOGICAL_AND}
};

static const BinaryOperatorMapping bitwise_or_operators[] = {
    {TOK_PIPE, AST_BINARY_OP_BIT_OR}
};

static const BinaryOperatorMapping bitwise_nand_operators[] = {
    {TOK_TILDE_AMP, AST_BINARY_OP_BIT_NAND}
};

static const BinaryOperatorMapping bitwise_xor_operators[] = {
    {TOK_CARET, AST_BINARY_OP_BIT_XOR}
};

static const BinaryOperatorMapping bitwise_xnor_operators[] = {
    {TOK_TILDE_CARET, AST_BINARY_OP_BIT_XNOR}
};

static const BinaryOperatorMapping bitwise_and_operators[] = {
    {TOK_AMP, AST_BINARY_OP_BIT_AND}
};

static const BinaryOperatorMapping equality_operators[] = {
    {TOK_EQ, AST_BINARY_OP_EQUAL},
    {TOK_NEQ, AST_BINARY_OP_NOT_EQUAL}
};

static const BinaryOperatorMapping relational_operators[] = {
    {TOK_LT, AST_BINARY_OP_LESS},
    {TOK_GT, AST_BINARY_OP_GREATER},
    {TOK_LTE, AST_BINARY_OP_LESS_EQUAL},
    {TOK_GTE, AST_BINARY_OP_GREATER_EQUAL}
};

static const BinaryOperatorMapping shift_operators[] = {
    {TOK_LSHIFT, AST_BINARY_OP_SHIFT_LEFT},
    {TOK_RSHIFT, AST_BINARY_OP_SHIFT_RIGHT}
};

static const BinaryOperatorMapping additive_operators[] = {
    {TOK_PLUS, AST_BINARY_OP_ADD},
    {TOK_MINUS, AST_BINARY_OP_SUBTRACT}
};

static const BinaryOperatorMapping multiplicative_operators[] = {
    {TOK_STAR, AST_BINARY_OP_MULTIPLY},
    {TOK_SLASH, AST_BINARY_OP_DIVIDE},
    {TOK_PERCENT, AST_BINARY_OP_MODULO}
};

static const UnaryOperatorMapping unary_operators[] = {
    {TOK_BANG, AST_UNARY_OP_LOGICAL_NOT},
    {TOK_TILDE, AST_UNARY_OP_BITWISE_NOT},
    {TOK_MINUS, AST_UNARY_OP_NEGATE},
    {TOK_PLUS, AST_UNARY_OP_PLUS}
};

void parser_init(Parser *parser, const char *source) {
    Token token;

    if (!parser) {
        return;
    }

    memset(parser, 0, sizeof(*parser));
    tokenizer_init(&parser->tokenizer, source ? source : "");

    do {
        token = tokenizer_next(&parser->tokenizer);
        if (token.type == TOK_ERROR) {
            parser_set_error(parser, token, "%.*s",
                             (int)token.length, token.start);
            break;
        }

        if (!parser_append_token(parser, token)) {
            parser_set_oom_error(parser);
            break;
        }
    } while (token.type != TOK_EOF);
}

void parser_free(Parser *parser) {
    if (!parser) {
        return;
    }

    free(parser->tokens);
    memset(parser, 0, sizeof(*parser));
}

bool parser_parse_program(Parser *parser, AstProgram *program) {
    if (!parser || !program) {
        return false;
    }

    ast_program_init(program);

    if (parser->has_error) {
        return false;
    }

    parser->current = 0;

    if (parser_match(parser, TOK_PACKAGE)) {
        AstQualifiedName package_name;

        if (!parser_parse_qualified_name(parser, &package_name)) {
            return false;
        }

        if (!parser_consume(parser, TOK_SEMICOLON,
                            "Expected ';' after package declaration.")) {
            ast_qualified_name_free(&package_name);
            return false;
        }

        ast_program_set_package(program, &package_name);
    }

    while (parser_match(parser, TOK_IMPORT)) {
        AstQualifiedName import_name;

        if (!parser_parse_qualified_name(parser, &import_name)) {
            ast_program_free(program);
            return false;
        }

        if (!parser_consume(parser, TOK_SEMICOLON,
                            "Expected ';' after import declaration.")) {
            ast_qualified_name_free(&import_name);
            ast_program_free(program);
            return false;
        }

        if (!parser_add_import(parser, program, &import_name)) {
            ast_program_free(program);
            return false;
        }
    }

    while (!parser_check(parser, TOK_EOF)) {
        AstTopLevelDecl *decl = parse_top_level_decl(parser);

        if (!decl) {
            ast_program_free(program);
            return false;
        }

        if (!parser_add_top_level_decl(parser, program, decl)) {
            ast_program_free(program);
            return false;
        }
    }

    return !parser->has_error;
}

AstExpression *parser_parse_expression(Parser *parser) {
    AstExpression *expression;

    if (!parser || parser->has_error) {
        return NULL;
    }

    parser->current = 0;
    expression = parse_expression_node(parser);
    if (!expression) {
        return NULL;
    }

    if (!parser_check(parser, TOK_EOF)) {
        parser_set_error(parser, *parser_current_token(parser),
                         "Expected end of expression.");
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

const ParserError *parser_get_error(const Parser *parser) {
    if (!parser || !parser->has_error) {
        return NULL;
    }
    return &parser->error;
}

static void parser_set_error(Parser *parser, Token token, const char *format, ...) {
    va_list args;

    if (!parser || parser->has_error) {
        return;
    }

    parser->has_error = true;
    parser->error.token = token;

    va_start(args, format);
    vsnprintf(parser->error.message, sizeof(parser->error.message), format, args);
    va_end(args);
}

static void parser_set_oom_error(Parser *parser) {
    parser_set_error(parser, *parser_current_token(parser),
                     "Out of memory while building the AST.");
}

static bool parser_append_token(Parser *parser, Token token) {
    Token *resized;
    size_t new_capacity;

    if (parser->token_count < parser->token_capacity) {
        parser->tokens[parser->token_count++] = token;
        return true;
    }

    new_capacity = (parser->token_capacity == 0) ? 32 : parser->token_capacity * 2;
    resized = realloc(parser->tokens, new_capacity * sizeof(*parser->tokens));
    if (!resized) {
        return false;
    }

    parser->tokens = resized;
    parser->token_capacity = new_capacity;
    parser->tokens[parser->token_count++] = token;
    return true;
}

static const Token *parser_token_at(const Parser *parser, size_t index) {
    static const Token eof_token = {TOK_EOF, "", 0, 1, 1};

    if (!parser || parser->token_count == 0) {
        return &eof_token;
    }

    if (index >= parser->token_count) {
        return &parser->tokens[parser->token_count - 1];
    }

    return &parser->tokens[index];
}

static const Token *parser_current_token(const Parser *parser) {
    return parser_token_at(parser, parser ? parser->current : 0);
}

static const Token *parser_previous_token(const Parser *parser) {
    if (!parser || parser->current == 0) {
        return parser_current_token(parser);
    }
    return parser_token_at(parser, parser->current - 1);
}

static bool parser_is_at_end(const Parser *parser) {
    return parser_current_token(parser)->type == TOK_EOF;
}

static const Token *parser_advance(Parser *parser) {
    if (!parser_is_at_end(parser)) {
        parser->current++;
    }
    return parser_previous_token(parser);
}

static bool parser_check(const Parser *parser, TokenType type) {
    return parser_current_token(parser)->type == type;
}

static bool parser_match(Parser *parser, TokenType type) {
    if (!parser_check(parser, type)) {
        return false;
    }

    parser_advance(parser);
    return true;
}

static bool parser_consume(Parser *parser, TokenType type, const char *message) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }

    parser_set_error(parser, *parser_current_token(parser), "%s", message);
    return false;
}

static AstSourceSpan parser_source_span(const Token *token) {
    AstSourceSpan span;
    size_t token_length = 0;

    memset(&span, 0, sizeof(span));
    if (!token) {
        return span;
    }

    token_length = token->length;
    span.start_line = token->line;
    span.start_column = token->column;
    span.end_line = token->line;
    span.end_column = token->column + (int)(token_length > 0 ? token_length - 1 : 0);
    return span;
}

static void parser_extend_source_span(AstSourceSpan *span, const Token *token) {
    AstSourceSpan token_span;

    if (!span || !token) {
        return;
    }

    token_span = parser_source_span(token);
    if (span->start_line == 0 || span->start_column == 0) {
        *span = token_span;
        return;
    }

    span->end_line = token_span.end_line;
    span->end_column = token_span.end_column;
}

static char *parser_copy_token_text(const Token *token) {
    if (!token) {
        return NULL;
    }
    return ast_copy_text_n(token->start, token->length);
}

static char *parser_consume_identifier(Parser *parser, const char *message) {
    char *identifier;

    if (!parser_check(parser, TOK_IDENTIFIER)) {
        parser_set_error(parser, *parser_current_token(parser), "%s", message);
        return NULL;
    }

    identifier = parser_copy_token_text(parser_current_token(parser));
    if (!identifier) {
        parser_set_oom_error(parser);
        return NULL;
    }

    parser_advance(parser);
    return identifier;
}

static bool parser_add_parameter(Parser *parser, AstParameterList *list,
                                 AstParameter *parameter) {
    if (ast_parameter_list_append(list, parameter)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_type_free(&parameter->type);
    free(parameter->name);
    memset(parameter, 0, sizeof(*parameter));
    return false;
}

static bool parser_add_expression(Parser *parser, AstExpressionList *list,
                                  AstExpression *expression) {
    if (ast_expression_list_append(list, expression)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_expression_free(expression);
    return false;
}

static bool parser_add_statement(Parser *parser, AstBlock *block,
                                 AstStatement *statement) {
    if (ast_block_append_statement(block, statement)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_statement_free(statement);
    return false;
}

static bool parser_add_top_level_decl(Parser *parser, AstProgram *program,
                                      AstTopLevelDecl *decl) {
    if (ast_program_add_top_level_decl(program, decl)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_top_level_decl_free(decl);
    return false;
}

static bool parser_add_import(Parser *parser, AstProgram *program,
                              AstQualifiedName *import_name) {
    if (ast_program_add_import(program, import_name)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_qualified_name_free(import_name);
    return false;
}

static bool parser_add_modifier(Parser *parser, AstBindingDecl *decl,
                                AstModifier modifier) {
    if (ast_binding_decl_add_modifier(decl, modifier)) {
        return true;
    }

    parser_set_oom_error(parser);
    return false;
}

static bool parser_add_template_text(Parser *parser, AstLiteral *literal,
                                     const Token *token) {
    char *text = copy_template_segment_text(token);
    bool ok;

    if (!text) {
        parser_set_oom_error(parser);
        return false;
    }

    ok = ast_template_literal_append_text(literal, text);
    free(text);

    if (!ok) {
        parser_set_oom_error(parser);
        return false;
    }

    return true;
}

static bool parser_add_template_expression(Parser *parser, AstLiteral *literal,
                                           AstExpression *expression) {
    if (ast_template_literal_append_expression(literal, expression)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_expression_free(expression);
    return false;
}

static bool parser_parse_qualified_name(Parser *parser, AstQualifiedName *name) {
    ast_qualified_name_init(name);

    for (;;) {
        const Token *segment_token;
        char *segment;
        bool ok;

        if (!parser_check(parser, TOK_IDENTIFIER)) {
            parser_set_error(parser, *parser_current_token(parser),
                             "Expected identifier in qualified name.");
            ast_qualified_name_free(name);
            return false;
        }

        segment_token = parser_current_token(parser);
        segment = parser_copy_token_text(segment_token);
        if (!segment) {
            parser_set_oom_error(parser);
            ast_qualified_name_free(name);
            return false;
        }

        parser_extend_source_span(&name->source_span, segment_token);
        name->tail_span = parser_source_span(segment_token);

        ok = ast_qualified_name_append(name, segment);
        free(segment);
        if (!ok) {
            parser_set_oom_error(parser);
            ast_qualified_name_free(name);
            return false;
        }

        parser_advance(parser);
        if (!parser_match(parser, TOK_DOT)) {
            break;
        }
    }

    return true;
}

static bool parser_parse_type(Parser *parser, AstType *type) {
    AstType parsed_type;

    memset(&parsed_type, 0, sizeof(parsed_type));

    if (parser_match(parser, TOK_VOID)) {
        ast_type_init_void(&parsed_type);
        *type = parsed_type;
        return true;
    }

    if (!is_primitive_type_token(parser_current_token(parser)->type)) {
        parser_set_error(parser, *parser_current_token(parser), "Expected type.");
        return false;
    }

    ast_type_init_primitive(&parsed_type,
                            primitive_type_from_token(parser_current_token(parser)->type));
    parser_advance(parser);

    while (parser_match(parser, TOK_LBRACKET)) {
        if (parser_match(parser, TOK_INT_LIT)) {
            char *size_literal = parser_copy_token_text(parser_previous_token(parser));
            bool ok;

            if (!size_literal) {
                parser_set_oom_error(parser);
                ast_type_free(&parsed_type);
                return false;
            }

            if (!parser_consume(parser, TOK_RBRACKET,
                                "Expected ']' after sized array dimension.")) {
                free(size_literal);
                ast_type_free(&parsed_type);
                return false;
            }

            ok = ast_type_add_dimension(&parsed_type, true, size_literal);
            free(size_literal);
            if (!ok) {
                parser_set_oom_error(parser);
                ast_type_free(&parsed_type);
                return false;
            }
        } else {
            if (!parser_consume(parser, TOK_RBRACKET,
                                "Expected ']' after array dimension.")) {
                ast_type_free(&parsed_type);
                return false;
            }

            if (!ast_type_add_dimension(&parsed_type, false, NULL)) {
                parser_set_oom_error(parser);
                ast_type_free(&parsed_type);
                return false;
            }
        }
    }

    *type = parsed_type;
    return true;
}

static bool parser_parse_parameter_list(Parser *parser, AstParameterList *list,
                                        bool allow_empty) {
    ast_parameter_list_init(list);

    if (parser_check(parser, TOK_RPAREN)) {
        if (allow_empty) {
            return true;
        }

        parser_set_error(parser, *parser_current_token(parser),
                         "Expected at least one parameter.");
        return false;
    }

    for (;;) {
        AstParameter parameter;
        const Token *name_token;

        memset(&parameter, 0, sizeof(parameter));
        if (!parser_parse_type(parser, &parameter.type)) {
            ast_parameter_list_free(list);
            return false;
        }

        name_token = parser_current_token(parser);
        parameter.name = parser_consume_identifier(parser, "Expected parameter name.");
        if (!parameter.name) {
            ast_type_free(&parameter.type);
            ast_parameter_list_free(list);
            return false;
        }
        parameter.name_span = parser_source_span(name_token);

        if (!parser_add_parameter(parser, list, &parameter)) {
            ast_parameter_list_free(list);
            return false;
        }

        if (!parser_match(parser, TOK_COMMA)) {
            break;
        }
    }

    return true;
}

static bool parser_parse_lambda_body(Parser *parser, AstLambdaBody *body) {
    if (parser_check(parser, TOK_LBRACE)) {
        AstBlock *block = parse_block(parser);

        if (!block) {
            return false;
        }

        body->kind = AST_LAMBDA_BODY_BLOCK;
        body->as.block = block;
        return true;
    }

    body->kind = AST_LAMBDA_BODY_EXPRESSION;
    body->as.expression = parse_assignment_expression(parser);
    return body->as.expression != NULL;
}

static bool parser_parse_block_or_expression_body(Parser *parser,
                                                  AstLambdaBody *body) {
    if (parser_check(parser, TOK_LBRACE)) {
        AstBlock *block = parse_block(parser);

        if (!block) {
            return false;
        }

        body->kind = AST_LAMBDA_BODY_BLOCK;
        body->as.block = block;
        return true;
    }

    body->kind = AST_LAMBDA_BODY_EXPRESSION;
    body->as.expression = parse_expression_node(parser);
    return body->as.expression != NULL;
}

static AstTopLevelDecl *parse_top_level_decl(Parser *parser) {
    if (parser_check(parser, TOK_START)) {
        return parse_start_decl(parser);
    }

    if (parser_check(parser, TOK_PUBLIC) || parser_check(parser, TOK_PRIVATE) ||
        parser_check(parser, TOK_FINAL) || parser_check(parser, TOK_VAR) ||
        is_type_start_token(parser_current_token(parser)->type)) {
        return parse_binding_decl(parser);
    }

    parser_set_error(parser, *parser_current_token(parser),
                     "Expected top-level declaration.");
    return NULL;
}

static AstTopLevelDecl *parse_start_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_START);

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    decl->as.start_decl.start_span = parser_source_span(parser_current_token(parser));

    if (!parser_consume(parser, TOK_START, "Expected 'start'.") ||
        !parser_consume(parser, TOK_LPAREN, "Expected '(' after 'start'.") ||
        !parser_parse_parameter_list(parser, &decl->as.start_decl.parameters, false) ||
        !parser_consume(parser, TOK_RPAREN, "Expected ')' after start parameters.") ||
        !parser_consume(parser, TOK_ARROW, "Expected '->' after start parameters.") ||
        !parser_parse_block_or_expression_body(parser, &decl->as.start_decl.body) ||
        !parser_consume(parser, TOK_SEMICOLON, "Expected ';' after start declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}

static AstTopLevelDecl *parse_binding_decl(Parser *parser) {
    AstTopLevelDecl *decl = ast_top_level_decl_new(AST_TOP_LEVEL_BINDING);
    const Token *name_token;

    if (!decl) {
        parser_set_oom_error(parser);
        return NULL;
    }

    while (parser_check(parser, TOK_PUBLIC) || parser_check(parser, TOK_PRIVATE) ||
           parser_check(parser, TOK_FINAL)) {
        AstModifier modifier;

        switch (parser_current_token(parser)->type) {
        case TOK_PUBLIC:
            modifier = AST_MODIFIER_PUBLIC;
            break;
        case TOK_PRIVATE:
            modifier = AST_MODIFIER_PRIVATE;
            break;
        default:
            modifier = AST_MODIFIER_FINAL;
            break;
        }

        parser_advance(parser);
        if (!parser_add_modifier(parser, &decl->as.binding_decl, modifier)) {
            ast_top_level_decl_free(decl);
            return NULL;
        }
    }

    if (parser_match(parser, TOK_VAR)) {
        decl->as.binding_decl.is_inferred_type = true;
    } else if (!parser_parse_type(parser, &decl->as.binding_decl.declared_type)) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    name_token = parser_current_token(parser);
    decl->as.binding_decl.name =
        parser_consume_identifier(parser, "Expected binding name.");
    if (!decl->as.binding_decl.name) {
        ast_top_level_decl_free(decl);
        return NULL;
    }
    decl->as.binding_decl.name_span = parser_source_span(name_token);

    if (!parser_consume(parser, TOK_ASSIGN, "Expected '=' after binding name.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    decl->as.binding_decl.initializer = parse_expression_node(parser);
    if (!decl->as.binding_decl.initializer) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after binding declaration.")) {
        ast_top_level_decl_free(decl);
        return NULL;
    }

    return decl;
}

static AstBlock *parse_block(Parser *parser) {
    AstBlock *block = ast_block_new();

    if (!block) {
        parser_set_oom_error(parser);
        return NULL;
    }

    if (!parser_consume(parser, TOK_LBRACE, "Expected '{' to start block.")) {
        ast_block_free(block);
        return NULL;
    }

    while (!parser_check(parser, TOK_RBRACE) && !parser_check(parser, TOK_EOF)) {
        AstStatement *statement = parse_statement(parser);

        if (!statement) {
            ast_block_free(block);
            return NULL;
        }

        if (!parser_add_statement(parser, block, statement)) {
            ast_block_free(block);
            return NULL;
        }
    }

    if (!parser_consume(parser, TOK_RBRACE, "Expected '}' after block.")) {
        ast_block_free(block);
        return NULL;
    }

    return block;
}

static AstStatement *parse_statement(Parser *parser) {
    if (parser_check(parser, TOK_RETURN)) {
        AstStatement *statement = ast_statement_new(AST_STMT_RETURN);

        if (!statement) {
            parser_set_oom_error(parser);
            return NULL;
        }

        statement->source_span = parser_source_span(parser_current_token(parser));

        parser_advance(parser);
        if (!parser_check(parser, TOK_SEMICOLON)) {
            statement->as.return_expression = parse_expression_node(parser);
            if (!statement->as.return_expression) {
                ast_statement_free(statement);
                return NULL;
            }
        }

        if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after return statement.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }

    if (parser_check(parser, TOK_EXIT)) {
        AstStatement *statement = ast_statement_new(AST_STMT_EXIT);

        if (!statement) {
            parser_set_oom_error(parser);
            return NULL;
        }

        statement->source_span = parser_source_span(parser_current_token(parser));

        parser_advance(parser);
        if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after exit statement.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }

    if (parser_check(parser, TOK_THROW)) {
        AstStatement *statement = ast_statement_new(AST_STMT_THROW);

        if (!statement) {
            parser_set_oom_error(parser);
            return NULL;
        }

        statement->source_span = parser_source_span(parser_current_token(parser));

        parser_advance(parser);
        statement->as.throw_expression = parse_expression_node(parser);
        if (!statement->as.throw_expression) {
            ast_statement_free(statement);
            return NULL;
        }

        if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after throw statement.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }

    if (parser_check(parser, TOK_FINAL) || parser_check(parser, TOK_VAR) ||
        (is_type_start_token(parser_current_token(parser)->type) &&
         looks_like_local_binding_statement(parser))) {
        AstStatement *statement = ast_statement_new(AST_STMT_LOCAL_BINDING);
        const Token *name_token;
        const Token *start_token = parser_current_token(parser);

        if (!statement) {
            parser_set_oom_error(parser);
            return NULL;
        }

        statement->source_span = parser_source_span(start_token);

        if (parser_match(parser, TOK_FINAL)) {
            statement->as.local_binding.is_final = true;
        }

        if (parser_match(parser, TOK_VAR)) {
            statement->as.local_binding.is_inferred_type = true;
        } else if (!parser_parse_type(parser, &statement->as.local_binding.declared_type)) {
            ast_statement_free(statement);
            return NULL;
        }

        name_token = parser_current_token(parser);
        statement->as.local_binding.name =
            parser_consume_identifier(parser, "Expected local binding name.");
        if (!statement->as.local_binding.name) {
            ast_statement_free(statement);
            return NULL;
        }
        statement->as.local_binding.name_span = parser_source_span(name_token);

        if (!parser_consume(parser, TOK_ASSIGN, "Expected '=' after local binding name.")) {
            ast_statement_free(statement);
            return NULL;
        }

        statement->as.local_binding.initializer = parse_expression_node(parser);
        if (!statement->as.local_binding.initializer) {
            ast_statement_free(statement);
            return NULL;
        }

        if (!parser_consume(parser, TOK_SEMICOLON,
                            "Expected ';' after local binding statement.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }

    {
        AstStatement *statement = ast_statement_new(AST_STMT_EXPRESSION);

        if (!statement) {
            parser_set_oom_error(parser);
            return NULL;
        }

        statement->as.expression = parse_expression_node(parser);
        if (!statement->as.expression) {
            ast_statement_free(statement);
            return NULL;
        }

        statement->source_span = statement->as.expression->source_span;

        if (!parser_consume(parser, TOK_SEMICOLON, "Expected ';' after expression statement.")) {
            ast_statement_free(statement);
            return NULL;
        }

        return statement;
    }
}

static AstExpression *parse_expression_node(Parser *parser) {
    if (looks_like_lambda_expression(parser)) {
        return parse_lambda_expression(parser);
    }
    return parse_assignment_expression(parser);
}

static AstExpression *parse_lambda_expression(Parser *parser) {
    AstExpression *expression = ast_expression_new(AST_EXPR_LAMBDA);

    if (!expression) {
        parser_set_oom_error(parser);
        return NULL;
    }

    expression->source_span = parser_source_span(parser_current_token(parser));

    if (!parser_consume(parser, TOK_LPAREN, "Expected '(' to start lambda parameters.") ||
        !parser_parse_parameter_list(parser, &expression->as.lambda.parameters, true) ||
        !parser_consume(parser, TOK_RPAREN, "Expected ')' after lambda parameters.") ||
        !parser_consume(parser, TOK_ARROW, "Expected '->' after lambda parameters.") ||
        !parser_parse_lambda_body(parser, &expression->as.lambda.body)) {
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

static AstExpression *parse_assignment_expression(Parser *parser) {
    AstExpression *expression = parse_ternary_expression(parser);
    size_t i;

    if (!expression) {
        return NULL;
    }

    for (i = 0; i < sizeof(assignment_operators) / sizeof(assignment_operators[0]); i++) {
        if (parser_check(parser, assignment_operators[i].token_type)) {
            AstExpression *assignment = ast_expression_new(AST_EXPR_ASSIGNMENT);
            AstExpression *value;

            if (!assignment) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            parser_advance(parser);
            value = parse_assignment_expression(parser);
            if (!value) {
                ast_expression_free(assignment);
                ast_expression_free(expression);
                return NULL;
            }

            assignment->source_span = expression->source_span;
            assignment->as.assignment.operator = assignment_operators[i].operator;
            assignment->as.assignment.target = expression;
            assignment->as.assignment.value = value;
            return assignment;
        }
    }

    return expression;
}

static AstExpression *parse_ternary_expression(Parser *parser) {
    AstExpression *condition = parse_logical_or_expression(parser);

    if (!condition) {
        return NULL;
    }

    if (parser_match(parser, TOK_QUESTION)) {
        AstExpression *expression = ast_expression_new(AST_EXPR_TERNARY);
        AstExpression *then_branch;
        AstExpression *else_branch;

        if (!expression) {
            parser_set_oom_error(parser);
            ast_expression_free(condition);
            return NULL;
        }

        then_branch = parse_expression_node(parser);
        if (!then_branch) {
            ast_expression_free(condition);
            ast_expression_free(expression);
            return NULL;
        }

        if (!parser_consume(parser, TOK_COLON,
                            "Expected ':' after ternary true branch.")) {
            ast_expression_free(condition);
            ast_expression_free(then_branch);
            ast_expression_free(expression);
            return NULL;
        }

        else_branch = parse_ternary_expression(parser);
        if (!else_branch) {
            ast_expression_free(condition);
            ast_expression_free(then_branch);
            ast_expression_free(expression);
            return NULL;
        }

        expression->source_span = condition->source_span;
        expression->as.ternary.condition = condition;
        expression->as.ternary.then_branch = then_branch;
        expression->as.ternary.else_branch = else_branch;
        return expression;
    }

    return condition;
}

static AstExpression *parse_logical_or_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_logical_and_expression,
                                     logical_or_operators,
                                     sizeof(logical_or_operators) /
                                         sizeof(logical_or_operators[0]));
}

static AstExpression *parse_logical_and_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_bitwise_or_expression,
                                     logical_and_operators,
                                     sizeof(logical_and_operators) /
                                         sizeof(logical_and_operators[0]));
}

static AstExpression *parse_bitwise_or_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_bitwise_nand_expression,
                                     bitwise_or_operators,
                                     sizeof(bitwise_or_operators) /
                                         sizeof(bitwise_or_operators[0]));
}

static AstExpression *parse_bitwise_nand_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_bitwise_xor_expression,
                                     bitwise_nand_operators,
                                     sizeof(bitwise_nand_operators) /
                                         sizeof(bitwise_nand_operators[0]));
}

static AstExpression *parse_bitwise_xor_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_bitwise_xnor_expression,
                                     bitwise_xor_operators,
                                     sizeof(bitwise_xor_operators) /
                                         sizeof(bitwise_xor_operators[0]));
}

static AstExpression *parse_bitwise_xnor_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_bitwise_and_expression,
                                     bitwise_xnor_operators,
                                     sizeof(bitwise_xnor_operators) /
                                         sizeof(bitwise_xnor_operators[0]));
}

static AstExpression *parse_bitwise_and_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_equality_expression,
                                     bitwise_and_operators,
                                     sizeof(bitwise_and_operators) /
                                         sizeof(bitwise_and_operators[0]));
}

static AstExpression *parse_equality_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_relational_expression,
                                     equality_operators,
                                     sizeof(equality_operators) /
                                         sizeof(equality_operators[0]));
}

static AstExpression *parse_relational_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_shift_expression,
                                     relational_operators,
                                     sizeof(relational_operators) /
                                         sizeof(relational_operators[0]));
}

static AstExpression *parse_shift_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_additive_expression,
                                     shift_operators,
                                     sizeof(shift_operators) /
                                         sizeof(shift_operators[0]));
}

static AstExpression *parse_additive_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_multiplicative_expression,
                                     additive_operators,
                                     sizeof(additive_operators) /
                                         sizeof(additive_operators[0]));
}

static AstExpression *parse_multiplicative_expression(Parser *parser) {
    return parser_parse_binary_level(parser, parse_unary_expression,
                                     multiplicative_operators,
                                     sizeof(multiplicative_operators) /
                                         sizeof(multiplicative_operators[0]));
}

static AstExpression *parse_unary_expression(Parser *parser) {
    size_t i;

    for (i = 0; i < sizeof(unary_operators) / sizeof(unary_operators[0]); i++) {
        if (parser_match(parser, unary_operators[i].token_type)) {
            AstExpression *expression = ast_expression_new(AST_EXPR_UNARY);
            AstExpression *operand;
            const Token *operator_token = parser_previous_token(parser);

            if (!expression) {
                parser_set_oom_error(parser);
                return NULL;
            }

            operand = parse_unary_expression(parser);
            if (!operand) {
                ast_expression_free(expression);
                return NULL;
            }

            expression->source_span = parser_source_span(operator_token);
            expression->as.unary.operator = unary_operators[i].operator;
            expression->as.unary.operand = operand;
            return expression;
        }
    }

    return parse_postfix_expression(parser);
}

static AstExpression *parse_postfix_expression(Parser *parser) {
    AstExpression *expression = parse_primary_expression(parser);

    if (!expression) {
        return NULL;
    }

    for (;;) {
        if (parser_match(parser, TOK_LPAREN)) {
            AstExpression *call = ast_expression_new(AST_EXPR_CALL);

            if (!call) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            call->source_span = expression->source_span;
            call->as.call.callee = expression;
            if (!parser_check(parser, TOK_RPAREN)) {
                do {
                    AstExpression *argument = parse_expression_node(parser);

                    if (!argument) {
                        ast_expression_free(call);
                        return NULL;
                    }

                    if (!parser_add_expression(parser, &call->as.call.arguments,
                                               argument)) {
                        ast_expression_free(call);
                        return NULL;
                    }
                } while (parser_match(parser, TOK_COMMA));
            }

            if (!parser_consume(parser, TOK_RPAREN,
                                "Expected ')' after argument list.")) {
                ast_expression_free(call);
                return NULL;
            }

            expression = call;
            continue;
        }

        if (parser_match(parser, TOK_LBRACKET)) {
            AstExpression *index = ast_expression_new(AST_EXPR_INDEX);

            if (!index) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            index->source_span = expression->source_span;
            index->as.index.target = expression;
            index->as.index.index = parse_expression_node(parser);
            if (!index->as.index.index) {
                ast_expression_free(index);
                return NULL;
            }

            if (!parser_consume(parser, TOK_RBRACKET,
                                "Expected ']' after index expression.")) {
                ast_expression_free(index);
                return NULL;
            }

            expression = index;
            continue;
        }

        if (parser_match(parser, TOK_DOT)) {
            AstExpression *member = ast_expression_new(AST_EXPR_MEMBER);

            if (!member) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            member->source_span = expression->source_span;
            member->as.member.target = expression;
            member->as.member.member =
                parser_consume_identifier(parser, "Expected member name after '.'.");
            if (!member->as.member.member) {
                ast_expression_free(member);
                return NULL;
            }

            expression = member;
            continue;
        }

        break;
    }

    return expression;
}

static AstExpression *parse_primary_expression(Parser *parser) {
    const Token *token = parser_current_token(parser);
    AstExpression *expression;

    switch (token->type) {
    case TOK_INT_LIT:
    case TOK_FLOAT_LIT:
    case TOK_CHAR_LIT:
    case TOK_STRING_LIT:
        expression = ast_expression_new(AST_EXPR_LITERAL);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        expression->as.literal.kind =
            (token->type == TOK_INT_LIT) ? AST_LITERAL_INTEGER :
            (token->type == TOK_FLOAT_LIT) ? AST_LITERAL_FLOAT :
            (token->type == TOK_CHAR_LIT) ? AST_LITERAL_CHAR : AST_LITERAL_STRING;
        expression->as.literal.as.text = parser_copy_token_text(token);
        if (!expression->as.literal.as.text) {
            parser_set_oom_error(parser);
            ast_expression_free(expression);
            return NULL;
        }
        parser_advance(parser);
        return expression;

    case TOK_TRUE:
    case TOK_FALSE:
        expression = ast_expression_new(AST_EXPR_LITERAL);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        expression->as.literal.kind = AST_LITERAL_BOOL;
        expression->as.literal.as.bool_value = (token->type == TOK_TRUE);
        parser_advance(parser);
        return expression;

    case TOK_NULL:
        expression = ast_expression_new(AST_EXPR_LITERAL);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        expression->as.literal.kind = AST_LITERAL_NULL;
        parser_advance(parser);
        return expression;

    case TOK_TEMPLATE_FULL:
    case TOK_TEMPLATE_START:
        return parse_template_literal_expression(parser);

    case TOK_IDENTIFIER:
        expression = ast_expression_new(AST_EXPR_IDENTIFIER);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        expression->as.identifier = parser_copy_token_text(token);
        if (!expression->as.identifier) {
            parser_set_oom_error(parser);
            ast_expression_free(expression);
            return NULL;
        }
        parser_advance(parser);
        return expression;

    case TOK_LPAREN:
        expression = ast_expression_new(AST_EXPR_GROUPING);
        if (!expression) {
            parser_set_oom_error(parser);
            return NULL;
        }
        expression->source_span = parser_source_span(token);
        parser_advance(parser);
        expression->as.grouping.inner = parse_expression_node(parser);
        if (!expression->as.grouping.inner) {
            ast_expression_free(expression);
            return NULL;
        }
        if (!parser_consume(parser, TOK_RPAREN,
                            "Expected ')' after grouped expression.")) {
            ast_expression_free(expression);
            return NULL;
        }
        return expression;

    case TOK_LBRACKET:
        return parse_array_literal_expression(parser);

    default:
        if (is_primitive_type_token(token->type) &&
            parser_token_at(parser, parser->current + 1)->type == TOK_LPAREN) {
            return parse_cast_expression(parser);
        }

        parser_set_error(parser, *token, "Expected expression.");
        return NULL;
    }
}

static AstExpression *parse_array_literal_expression(Parser *parser) {
    AstExpression *expression = ast_expression_new(AST_EXPR_ARRAY_LITERAL);

    if (!expression) {
        parser_set_oom_error(parser);
        return NULL;
    }

    expression->source_span = parser_source_span(parser_current_token(parser));

    if (!parser_consume(parser, TOK_LBRACKET, "Expected '[' to start array literal.")) {
        ast_expression_free(expression);
        return NULL;
    }

    if (!parser_check(parser, TOK_RBRACKET)) {
        do {
            AstExpression *element = parse_expression_node(parser);

            if (!element) {
                ast_expression_free(expression);
                return NULL;
            }

            if (!parser_add_expression(parser, &expression->as.array_literal.elements,
                                       element)) {
                ast_expression_free(expression);
                return NULL;
            }
        } while (parser_match(parser, TOK_COMMA));
    }

    if (!parser_consume(parser, TOK_RBRACKET, "Expected ']' after array literal.")) {
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

static AstExpression *parse_cast_expression(Parser *parser) {
    AstExpression *expression = ast_expression_new(AST_EXPR_CAST);

    if (!expression) {
        parser_set_oom_error(parser);
        return NULL;
    }

    expression->source_span = parser_source_span(parser_current_token(parser));

    expression->as.cast.target_type =
        primitive_type_from_token(parser_current_token(parser)->type);
    parser_advance(parser);

    if (!parser_consume(parser, TOK_LPAREN, "Expected '(' after cast type.")) {
        ast_expression_free(expression);
        return NULL;
    }

    expression->as.cast.expression = parse_expression_node(parser);
    if (!expression->as.cast.expression) {
        ast_expression_free(expression);
        return NULL;
    }

    if (!parser_consume(parser, TOK_RPAREN, "Expected ')' after cast expression.")) {
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

static AstExpression *parse_template_literal_expression(Parser *parser) {
    AstExpression *expression = ast_expression_new(AST_EXPR_LITERAL);

    if (!expression) {
        parser_set_oom_error(parser);
        return NULL;
    }

    expression->as.literal.kind = AST_LITERAL_TEMPLATE;
    expression->source_span = parser_source_span(parser_current_token(parser));

    if (parser_match(parser, TOK_TEMPLATE_FULL)) {
        if (!parser_add_template_text(parser, &expression->as.literal,
                                      parser_previous_token(parser))) {
            ast_expression_free(expression);
            return NULL;
        }
        return expression;
    }

    if (!parser_consume(parser, TOK_TEMPLATE_START,
                        "Expected template literal start token.")) {
        ast_expression_free(expression);
        return NULL;
    }

    if (!parser_add_template_text(parser, &expression->as.literal,
                                  parser_previous_token(parser))) {
        ast_expression_free(expression);
        return NULL;
    }

    for (;;) {
        AstExpression *interpolation = parse_expression_node(parser);

        if (!interpolation) {
            ast_expression_free(expression);
            return NULL;
        }

        if (!parser_add_template_expression(parser, &expression->as.literal,
                                            interpolation)) {
            ast_expression_free(expression);
            return NULL;
        }

        if (parser_match(parser, TOK_TEMPLATE_MIDDLE)) {
            if (!parser_add_template_text(parser, &expression->as.literal,
                                          parser_previous_token(parser))) {
                ast_expression_free(expression);
                return NULL;
            }
            continue;
        }

        if (parser_match(parser, TOK_TEMPLATE_END)) {
            if (!parser_add_template_text(parser, &expression->as.literal,
                                          parser_previous_token(parser))) {
                ast_expression_free(expression);
                return NULL;
            }
            break;
        }

        parser_set_error(parser, *parser_current_token(parser),
                         "Expected template continuation after interpolation.");
        ast_expression_free(expression);
        return NULL;
    }

    return expression;
}

static AstExpression *parser_parse_binary_level(Parser *parser,
                                                ParseExpressionFn operand_parser,
                                                const BinaryOperatorMapping *mappings,
                                                size_t mapping_count) {
    AstExpression *expression = operand_parser(parser);

    if (!expression) {
        return NULL;
    }

    for (;;) {
        size_t i;
        const BinaryOperatorMapping *matched = NULL;

        for (i = 0; i < mapping_count; i++) {
            if (parser_check(parser, mappings[i].token_type)) {
                matched = &mappings[i];
                break;
            }
        }

        if (!matched) {
            break;
        }

        parser_advance(parser);

        {
            AstExpression *binary = ast_expression_new(AST_EXPR_BINARY);
            AstExpression *right;

            if (!binary) {
                parser_set_oom_error(parser);
                ast_expression_free(expression);
                return NULL;
            }

            right = operand_parser(parser);
            if (!right) {
                ast_expression_free(binary);
                ast_expression_free(expression);
                return NULL;
            }

            binary->source_span = expression->source_span;
            binary->as.binary.operator = matched->operator;
            binary->as.binary.left = expression;
            binary->as.binary.right = right;
            expression = binary;
        }
    }

    return expression;
}

static bool is_primitive_type_token(TokenType type) {
    switch (type) {
    case TOK_INT8:
    case TOK_INT16:
    case TOK_INT32:
    case TOK_INT64:
    case TOK_UINT8:
    case TOK_UINT16:
    case TOK_UINT32:
    case TOK_UINT64:
    case TOK_FLOAT32:
    case TOK_FLOAT64:
    case TOK_BOOL:
    case TOK_CHAR:
    case TOK_STRING:
        return true;
    default:
        return false;
    }
}

static bool is_type_start_token(TokenType type) {
    return type == TOK_VOID || is_primitive_type_token(type);
}

static bool scan_type_pattern(const Parser *parser, size_t *index) {
    if (parser_token_at(parser, *index)->type == TOK_VOID) {
        (*index)++;
        return true;
    }

    if (!is_primitive_type_token(parser_token_at(parser, *index)->type)) {
        return false;
    }

    (*index)++;

    while (parser_token_at(parser, *index)->type == TOK_LBRACKET) {
        (*index)++;
        if (parser_token_at(parser, *index)->type == TOK_INT_LIT) {
            (*index)++;
        }
        if (parser_token_at(parser, *index)->type != TOK_RBRACKET) {
            return false;
        }
        (*index)++;
    }

    return true;
}

static bool looks_like_lambda_expression(const Parser *parser) {
    size_t index = parser ? parser->current : 0;

    if (parser_token_at(parser, index)->type != TOK_LPAREN) {
        return false;
    }

    index++;
    if (parser_token_at(parser, index)->type == TOK_RPAREN) {
        return parser_token_at(parser, index + 1)->type == TOK_ARROW;
    }

    for (;;) {
        if (!scan_type_pattern(parser, &index)) {
            return false;
        }

        if (parser_token_at(parser, index)->type != TOK_IDENTIFIER) {
            return false;
        }

        index++;
        if (parser_token_at(parser, index)->type == TOK_COMMA) {
            index++;
            continue;
        }

        if (parser_token_at(parser, index)->type != TOK_RPAREN) {
            return false;
        }

        return parser_token_at(parser, index + 1)->type == TOK_ARROW;
    }
}

static bool looks_like_local_binding_statement(const Parser *parser) {
    size_t index = parser ? parser->current : 0;

    if (parser_token_at(parser, index)->type == TOK_FINAL) {
        index++;
    }

    if (parser_token_at(parser, index)->type == TOK_VAR) {
        index++;
    } else if (!scan_type_pattern(parser, &index)) {
        return false;
    }

    return parser_token_at(parser, index)->type == TOK_IDENTIFIER &&
           parser_token_at(parser, index + 1)->type == TOK_ASSIGN;
}

static AstPrimitiveType primitive_type_from_token(TokenType type) {
    switch (type) {
    case TOK_INT8: return AST_PRIMITIVE_INT8;
    case TOK_INT16: return AST_PRIMITIVE_INT16;
    case TOK_INT32: return AST_PRIMITIVE_INT32;
    case TOK_INT64: return AST_PRIMITIVE_INT64;
    case TOK_UINT8: return AST_PRIMITIVE_UINT8;
    case TOK_UINT16: return AST_PRIMITIVE_UINT16;
    case TOK_UINT32: return AST_PRIMITIVE_UINT32;
    case TOK_UINT64: return AST_PRIMITIVE_UINT64;
    case TOK_FLOAT32: return AST_PRIMITIVE_FLOAT32;
    case TOK_FLOAT64: return AST_PRIMITIVE_FLOAT64;
    case TOK_BOOL: return AST_PRIMITIVE_BOOL;
    case TOK_CHAR: return AST_PRIMITIVE_CHAR;
    default: return AST_PRIMITIVE_STRING;
    }
}

static char *copy_template_segment_text(const Token *token) {
    size_t start_offset = 0;
    size_t end_offset = 0;

    if (!token) {
        return NULL;
    }

    switch (token->type) {
    case TOK_TEMPLATE_FULL:
        start_offset = 1;
        end_offset = 1;
        break;
    case TOK_TEMPLATE_START:
    case TOK_TEMPLATE_MIDDLE:
        start_offset = 1;
        break;
    case TOK_TEMPLATE_END:
        start_offset = 1;
        end_offset = 1;
        break;
    default:
        return NULL;
    }

    if (token->length < start_offset + end_offset) {
        return ast_copy_text("");
    }

    return ast_copy_text_n(token->start + start_offset,
                           token->length - start_offset - end_offset);
}