#ifndef CALYNDA_PARSER_H
#define CALYNDA_PARSER_H

#include "ast.h"
#include "tokenizer.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    Token token;
    char  message[256];
} ParserError;

typedef struct {
    Tokenizer   tokenizer;
    Token      *tokens;
    size_t      token_count;
    size_t      token_capacity;
    size_t      current;
    ParserError error;
    bool        has_error;
} Parser;

void parser_init(Parser *parser, const char *source);
void parser_free(Parser *parser);

bool parser_parse_program(Parser *parser, AstProgram *program);
AstExpression *parser_parse_expression(Parser *parser);

const ParserError *parser_get_error(const Parser *parser);

#endif /* CALYNDA_PARSER_H */