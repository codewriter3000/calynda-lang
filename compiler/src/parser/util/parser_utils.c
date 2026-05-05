#include "parser_internal.h"

void parser_set_error(Parser *parser, Token token, const char *format, ...) {
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

void parser_set_oom_error(Parser *parser) {
    parser_set_error(parser, *parser_current_token(parser),
                     "Out of memory while building the AST.");
}

bool parser_append_token(Parser *parser, Token token) {
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

const Token *parser_token_at(const Parser *parser, size_t index) {
    static const Token eof_token = {TOK_EOF, "", 0, 1, 1};

    if (!parser || parser->token_count == 0) {
        return &eof_token;
    }

    if (index >= parser->token_count) {
        return &parser->tokens[parser->token_count - 1];
    }

    return &parser->tokens[index];
}

const Token *parser_current_token(const Parser *parser) {
    return parser_token_at(parser, parser ? parser->current : 0);
}

const Token *parser_previous_token(const Parser *parser) {
    if (!parser || parser->current == 0) {
        return parser_current_token(parser);
    }
    return parser_token_at(parser, parser->current - 1);
}

bool parser_is_at_end(const Parser *parser) {
    return parser_current_token(parser)->type == TOK_EOF;
}

const Token *parser_advance(Parser *parser) {
    if (!parser_is_at_end(parser)) {
        parser->current++;
    }
    return parser_previous_token(parser);
}

bool parser_check(const Parser *parser, TokenType type) {
    return parser_current_token(parser)->type == type;
}

bool parser_match(Parser *parser, TokenType type) {
    if (!parser_check(parser, type)) {
        return false;
    }
    parser_advance(parser);
    return true;
}

bool parser_consume(Parser *parser, TokenType type, const char *message) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }

    parser_set_error(parser, *parser_current_token(parser), "%s", message);
    return false;
}

bool parser_consume_gt(Parser *parser, const char *message) {
    if (parser_match(parser, TOK_GT)) {
        return true;
    }

    if (parser_check(parser, TOK_RSHIFT)) {
        Token fixed = *parser_current_token(parser);

        fixed.type = TOK_GT;
        fixed.length = 1;
        parser->tokens[parser->current] = fixed;

        {
            Token remainder = fixed;
            remainder.start = fixed.start + 1;
            remainder.column = fixed.column + 1;
            parser->tokens[parser->current] = remainder;
        }

        return true;
    }

    parser_set_error(parser, *parser_current_token(parser), "%s", message);
    return false;
}

AstSourceSpan parser_source_span(const Token *token) {
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

void parser_extend_source_span(AstSourceSpan *span, const Token *token) {
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

char *parser_copy_token_text(const Token *token) {
    if (!token || !token->start) {
        return NULL;
    }
    return ast_copy_text_n(token->start, token->length);
}

char *parser_consume_identifier(Parser *parser, const char *message) {
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

bool parser_add_parameter(Parser *parser, AstParameterList *list,
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

bool parser_add_expression(Parser *parser, AstExpressionList *list,
                           AstExpression *expression) {
    if (ast_expression_list_append(list, expression)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_expression_free(expression);
    return false;
}

bool parser_add_top_level_decl(Parser *parser, AstProgram *program,
                               AstTopLevelDecl *decl) {
    if (ast_program_add_top_level_decl(program, decl)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_top_level_decl_free(decl);
    return false;
}

bool parser_add_import(Parser *parser, AstProgram *program,
                       AstImportDecl *import_decl) {
    if (ast_program_add_import(program, import_decl)) {
        return true;
    }

    parser_set_oom_error(parser);
    ast_import_decl_free(import_decl);
    return false;
}

bool parser_add_modifier(Parser *parser, AstBindingDecl *decl,
                         AstModifier modifier) {
    if (ast_binding_decl_add_modifier(decl, modifier)) {
        return true;
    }

    parser_set_oom_error(parser);
    return false;
}
