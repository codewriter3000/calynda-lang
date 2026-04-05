#include "tokenizer_internal.h"

/* ------------------------------------------------------------------ */
/*  Skip whitespace and comments                                      */
/* ------------------------------------------------------------------ */

static void skip_whitespace(Tokenizer *t) {
    for (;;) {
        char c = peek(t);
        switch (c) {
        case ' ': case '\t': case '\r': case '\n':
            advance_char(t);
            break;
        case '/':
            if (peek_next(t) == '/') {
                /* single-line comment */
                while (!is_at_end(t) && peek(t) != '\n')
                    advance_char(t);
                break;
            }
            if (peek_next(t) == '*') {
                /* multi-line comment */
                advance_char(t); /* / */
                advance_char(t); /* * */
                while (!is_at_end(t)) {
                    if (peek(t) == '*' && peek_next(t) == '/') {
                        advance_char(t); /* * */
                        advance_char(t); /* / */
                        break;
                    }
                    advance_char(t);
                }
                break;
            }
            return;
        default:
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Main scanning function                                            */
/* ------------------------------------------------------------------ */

Token tokenizer_next(Tokenizer *t) {
    skip_whitespace(t);

    /* If we're inside a template interpolation and see the matching },
       resume template body scanning. Nested braces inside the
       interpolation are tracked separately per template depth. */
    if (t->template_depth > 0 &&
        t->interpolation_brace_depth[t->template_depth - 1] == 0 &&
        peek(t) == '}') {
        const char *start = t->current;
        int line = t->line, col = t->column;
        advance_char(t); /* consume } */
        t->template_depth--;
        return tokenizer_scan_template_body(t, start, line, col,
                                  TOK_TEMPLATE_MIDDLE, TOK_TEMPLATE_END);
    }

    /* If we just saw `asm` and the next char is '{', switch to raw
       asm body scanning — capture everything between { and matching }. */
    if (t->asm_body_pending && !is_at_end(t) && peek(t) == '{') {
        int line = t->line, col = t->column;
        int depth;
        const char *body_start;
        const char *body_end;
        Token tok;

        t->asm_body_pending = 0;
        advance_char(t); /* consume '{' */
        body_start = t->current;
        depth = 1;

        while (!is_at_end(t) && depth > 0) {
            char c = peek(t);
            if (c == '{') depth++;
            else if (c == '}') {
                depth--;
                if (depth == 0) break;
            }
            advance_char(t);
        }

        body_end = t->current;
        if (!is_at_end(t)) {
            advance_char(t); /* consume matching '}' */
        }

        tok.type   = TOK_ASM_BODY;
        tok.start  = body_start;
        tok.length = (size_t)(body_end - body_start);
        tok.line   = line;
        tok.column = col;
        return tok;
    }

    if (is_at_end(t)) {
        return make_token(t, TOK_EOF, t->current, t->line, t->column);
    }

    const char *start = t->current;
    int line = t->line;
    int col  = t->column;
    char c = advance_char(t);

    /* Identifiers and keywords */
    if (isalpha((unsigned char)c) || c == '_') {
        Token tok = tokenizer_scan_identifier(t, start, line, col);
        if (tok.type == TOK_ASM)
            t->asm_body_pending = 1;
        return tok;
    }

    /* Numbers */
    if (isdigit((unsigned char)c))
        return tokenizer_scan_number(t, start, line, col);

    return tokenizer_scan_punctuation(t, c, start, line, col);
}

/* ------------------------------------------------------------------ */
/*  Init                                                              */
/* ------------------------------------------------------------------ */

void tokenizer_init(Tokenizer *t, const char *source) {
    t->source         = source;
    t->current        = source;
    t->line           = 1;
    t->column         = 1;
    t->template_depth = 0;
    t->asm_body_pending = 0;
    memset(t->interpolation_brace_depth, 0,
           sizeof(t->interpolation_brace_depth));
}

/* ------------------------------------------------------------------ */
/*  token_type_name                                                   */
/* ------------------------------------------------------------------ */

const char *token_type_name(TokenType type) {
    switch (type) {
    case TOK_EOF:            return "EOF";
    case TOK_ERROR:          return "ERROR";
    case TOK_INT_LIT:        return "INT_LIT";
    case TOK_FLOAT_LIT:      return "FLOAT_LIT";
    case TOK_CHAR_LIT:       return "CHAR_LIT";
    case TOK_STRING_LIT:     return "STRING_LIT";
    case TOK_TEMPLATE_START: return "TEMPLATE_START";
    case TOK_TEMPLATE_MIDDLE:return "TEMPLATE_MIDDLE";
    case TOK_TEMPLATE_END:   return "TEMPLATE_END";
    case TOK_TEMPLATE_FULL:  return "TEMPLATE_FULL";
    case TOK_IDENTIFIER:     return "IDENTIFIER";
    case TOK_PACKAGE:        return "PACKAGE";
    case TOK_IMPORT:         return "IMPORT";
    case TOK_START:          return "START";
    case TOK_BOOT:           return "BOOT";
    case TOK_VAR:            return "VAR";
    case TOK_PUBLIC:         return "PUBLIC";
    case TOK_PRIVATE:        return "PRIVATE";
    case TOK_FINAL:          return "FINAL";
    case TOK_VOID:           return "VOID";
    case TOK_RETURN:         return "RETURN";
    case TOK_EXIT:           return "EXIT";
    case TOK_THROW:          return "THROW";
    case TOK_TRUE:           return "TRUE";
    case TOK_FALSE:          return "FALSE";
    case TOK_NULL:           return "NULL";
    case TOK_EXPORT:         return "EXPORT";
    case TOK_AS:             return "AS";
    case TOK_INTERNAL:       return "INTERNAL";
    case TOK_STATIC:         return "STATIC";
    case TOK_UNION:          return "UNION";
    case TOK_MANUAL:         return "MANUAL";
    case TOK_ARR:            return "ARR";
    case TOK_MALLOC:         return "MALLOC";
    case TOK_CALLOC:         return "CALLOC";
    case TOK_REALLOC:        return "REALLOC";
    case TOK_FREE:           return "FREE";
    case TOK_ASM:            return "ASM";
    case TOK_ASM_BODY:       return "ASM_BODY";
    case TOK_INT8:           return "INT8";
    case TOK_INT16:          return "INT16";
    case TOK_INT32:          return "INT32";
    case TOK_INT64:          return "INT64";
    case TOK_UINT8:          return "UINT8";
    case TOK_UINT16:         return "UINT16";
    case TOK_UINT32:         return "UINT32";
    case TOK_UINT64:         return "UINT64";
    case TOK_FLOAT32:        return "FLOAT32";
    case TOK_FLOAT64:        return "FLOAT64";
    case TOK_BOOL:           return "BOOL";
    case TOK_CHAR:           return "CHAR";
    case TOK_STRING:         return "STRING";
    case TOK_BYTE:           return "BYTE";
    case TOK_SBYTE:          return "SBYTE";
    case TOK_SHORT:          return "SHORT";
    case TOK_INT:            return "INT";
    case TOK_LONG:           return "LONG";
    case TOK_ULONG:          return "ULONG";
    case TOK_UINT:           return "UINT";
    case TOK_FLOAT:          return "FLOAT";
    case TOK_DOUBLE:         return "DOUBLE";
    case TOK_LPAREN:         return "LPAREN";
    case TOK_RPAREN:         return "RPAREN";
    case TOK_LBRACE:         return "LBRACE";
    case TOK_RBRACE:         return "RBRACE";
    case TOK_LBRACKET:       return "LBRACKET";
    case TOK_RBRACKET:       return "RBRACKET";
    case TOK_SEMICOLON:      return "SEMICOLON";
    case TOK_COMMA:          return "COMMA";
    case TOK_DOT:            return "DOT";
    case TOK_ELLIPSIS:       return "ELLIPSIS";
    case TOK_ARROW:          return "ARROW";
    case TOK_QUESTION:       return "QUESTION";
    case TOK_COLON:          return "COLON";
    case TOK_UNDERSCORE:     return "UNDERSCORE";
    case TOK_DOLLAR_LBRACE:  return "DOLLAR_LBRACE";
    case TOK_ASSIGN:         return "ASSIGN";
    case TOK_PLUS_ASSIGN:    return "PLUS_ASSIGN";
    case TOK_MINUS_ASSIGN:   return "MINUS_ASSIGN";
    case TOK_STAR_ASSIGN:    return "STAR_ASSIGN";
    case TOK_SLASH_ASSIGN:   return "SLASH_ASSIGN";
    case TOK_PERCENT_ASSIGN: return "PERCENT_ASSIGN";
    case TOK_AMP_ASSIGN:     return "AMP_ASSIGN";
    case TOK_PIPE_ASSIGN:    return "PIPE_ASSIGN";
    case TOK_CARET_ASSIGN:   return "CARET_ASSIGN";
    case TOK_LSHIFT_ASSIGN:  return "LSHIFT_ASSIGN";
    case TOK_RSHIFT_ASSIGN:  return "RSHIFT_ASSIGN";
    case TOK_EQ:             return "EQ";
    case TOK_NEQ:            return "NEQ";
    case TOK_LT:             return "LT";
    case TOK_GT:             return "GT";
    case TOK_LTE:            return "LTE";
    case TOK_GTE:            return "GTE";
    case TOK_LOGIC_OR:       return "LOGIC_OR";
    case TOK_LOGIC_AND:      return "LOGIC_AND";
    case TOK_PIPE:           return "PIPE";
    case TOK_AMP:            return "AMP";
    case TOK_CARET:          return "CARET";
    case TOK_TILDE:          return "TILDE";
    case TOK_TILDE_AMP:      return "TILDE_AMP";
    case TOK_TILDE_CARET:    return "TILDE_CARET";
    case TOK_LSHIFT:         return "LSHIFT";
    case TOK_RSHIFT:         return "RSHIFT";
    case TOK_PLUS:           return "PLUS";
    case TOK_PLUS_PLUS:      return "PLUS_PLUS";
    case TOK_MINUS:          return "MINUS";
    case TOK_MINUS_MINUS:    return "MINUS_MINUS";
    case TOK_STAR:           return "STAR";
    case TOK_SLASH:          return "SLASH";
    case TOK_PERCENT:        return "PERCENT";
    case TOK_BANG:           return "BANG";
    case TOK_COUNT:          return "COUNT";
    }
    return "UNKNOWN";
}
