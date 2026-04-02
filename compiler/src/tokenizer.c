#include "tokenizer.h"
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/*  Helpers                                                           */
/* ------------------------------------------------------------------ */

static int is_at_end(const Tokenizer *t) {
    return *t->current == '\0';
}

static char peek(const Tokenizer *t) {
    return *t->current;
}

static char peek_next(const Tokenizer *t) {
    if (is_at_end(t)) return '\0';
    return t->current[1];
}

static char advance(Tokenizer *t) {
    char c = *t->current++;
    if (c == '\n') {
        t->line++;
        t->column = 1;
    } else {
        t->column++;
    }
    return c;
}

static int match(Tokenizer *t, char expected) {
    if (is_at_end(t) || *t->current != expected) return 0;
    advance(t);
    return 1;
}

static Token make_token(const Tokenizer *t, TokenType type,
                        const char *start, int line, int col) {
    Token tok;
    tok.type   = type;
    tok.start  = start;
    tok.length = (size_t)(t->current - start);
    tok.line   = line;
    tok.column = col;
    return tok;
}

static Token error_token(const Tokenizer *t, const char *msg,
                         int line, int col) {
    Token tok;
    (void)t;
    tok.type   = TOK_ERROR;
    tok.start  = msg;
    tok.length = strlen(msg);
    tok.line   = line;
    tok.column = col;
    return tok;
}

/* ------------------------------------------------------------------ */
/*  Skip whitespace and comments                                      */
/* ------------------------------------------------------------------ */

static void skip_whitespace(Tokenizer *t) {
    for (;;) {
        char c = peek(t);
        switch (c) {
        case ' ': case '\t': case '\r': case '\n':
            advance(t);
            break;
        case '/':
            if (peek_next(t) == '/') {
                /* single-line comment */
                while (!is_at_end(t) && peek(t) != '\n')
                    advance(t);
                break;
            }
            if (peek_next(t) == '*') {
                /* multi-line comment */
                advance(t); /* / */
                advance(t); /* * */
                while (!is_at_end(t)) {
                    if (peek(t) == '*' && peek_next(t) == '/') {
                        advance(t); /* * */
                        advance(t); /* / */
                        break;
                    }
                    advance(t);
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
/*  Keyword table                                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *text;
    TokenType   type;
} Keyword;

static const Keyword keywords[] = {
    {"package",  TOK_PACKAGE},
    {"import",   TOK_IMPORT},
    {"start",    TOK_START},
    {"var",      TOK_VAR},
    {"public",   TOK_PUBLIC},
    {"private",  TOK_PRIVATE},
    {"final",    TOK_FINAL},
    {"void",     TOK_VOID},
    {"return",   TOK_RETURN},
    {"exit",     TOK_EXIT},
    {"throw",    TOK_THROW},
    {"true",     TOK_TRUE},
    {"false",    TOK_FALSE},
    {"null",     TOK_NULL},
    {"int8",     TOK_INT8},
    {"int16",    TOK_INT16},
    {"int32",    TOK_INT32},
    {"int64",    TOK_INT64},
    {"uint8",    TOK_UINT8},
    {"uint16",   TOK_UINT16},
    {"uint32",   TOK_UINT32},
    {"uint64",   TOK_UINT64},
    {"float32",  TOK_FLOAT32},
    {"float64",  TOK_FLOAT64},
    {"bool",     TOK_BOOL},
    {"char",     TOK_CHAR},
    {"string",   TOK_STRING},
    {NULL, TOK_EOF}
};

static TokenType check_keyword(const char *start, size_t len) {
    for (int i = 0; keywords[i].text != NULL; i++) {
        if (strlen(keywords[i].text) == len &&
            memcmp(keywords[i].text, start, len) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENTIFIER;
}

/* ------------------------------------------------------------------ */
/*  Identifier / keyword                                              */
/* ------------------------------------------------------------------ */

static Token scan_identifier(Tokenizer *t, const char *start,
                             int line, int col) {
    while (!is_at_end(t) && (isalnum((unsigned char)peek(t)) || peek(t) == '_'))
        advance(t);
    size_t len = (size_t)(t->current - start);
    TokenType type = check_keyword(start, len);
    return make_token(t, type, start, line, col);
}

/* ------------------------------------------------------------------ */
/*  Number literals                                                   */
/* ------------------------------------------------------------------ */

static Token scan_number(Tokenizer *t, const char *start,
                         int line, int col) {
    /* Check for 0b, 0o, 0x prefixes */
    if (start[0] == '0' && !is_at_end(t)) {
        char next = peek(t);
        if (next == 'b' || next == 'B') {
            advance(t); /* consume b/B */
            if (is_at_end(t) || (peek(t) != '0' && peek(t) != '1'))
                return error_token(t, "Expected binary digit after 0b", line, col);
            while (!is_at_end(t) && (peek(t) == '0' || peek(t) == '1'))
                advance(t);
            return make_token(t, TOK_INT_LIT, start, line, col);
        }
        if (next == 'o' || next == 'O') {
            advance(t); /* consume o/O */
            if (is_at_end(t) || peek(t) < '0' || peek(t) > '7')
                return error_token(t, "Expected octal digit after 0o", line, col);
            while (!is_at_end(t) && peek(t) >= '0' && peek(t) <= '7')
                advance(t);
            return make_token(t, TOK_INT_LIT, start, line, col);
        }
        if (next == 'x' || next == 'X') {
            advance(t); /* consume x/X */
            if (is_at_end(t) || !isxdigit((unsigned char)peek(t)))
                return error_token(t, "Expected hex digit after 0x", line, col);
            while (!is_at_end(t) && isxdigit((unsigned char)peek(t)))
                advance(t);
            return make_token(t, TOK_INT_LIT, start, line, col);
        }
    }

    /* Decimal digits */
    while (!is_at_end(t) && isdigit((unsigned char)peek(t)))
        advance(t);

    int is_float = 0;

    /* Fractional part */
    if (!is_at_end(t) && peek(t) == '.' && isdigit((unsigned char)peek_next(t))) {
        is_float = 1;
        advance(t); /* consume . */
        while (!is_at_end(t) && isdigit((unsigned char)peek(t)))
            advance(t);
    }

    /* Exponent */
    if (!is_at_end(t) && (peek(t) == 'e' || peek(t) == 'E')) {
        is_float = 1;
        advance(t); /* consume e/E */
        if (!is_at_end(t) && (peek(t) == '+' || peek(t) == '-'))
            advance(t);
        if (is_at_end(t) || !isdigit((unsigned char)peek(t)))
            return error_token(t, "Expected digit in exponent", line, col);
        while (!is_at_end(t) && isdigit((unsigned char)peek(t)))
            advance(t);
    }

    return make_token(t, is_float ? TOK_FLOAT_LIT : TOK_INT_LIT,
                      start, line, col);
}

/* ------------------------------------------------------------------ */
/*  Escape sequences (shared by char, string, template)               */
/* ------------------------------------------------------------------ */

/* Advances past a single escape sequence starting after the backslash.
   Returns 0 on error. */
static int scan_escape(Tokenizer *t) {
    if (is_at_end(t)) return 0;
    char c = advance(t);
    switch (c) {
    case 'n': case 't': case 'r': case '\\': case '\'':
    case '"': case '`': case '$': case '0':
        return 1;
    case 'u':
        for (int i = 0; i < 4; i++) {
            if (is_at_end(t) || !isxdigit((unsigned char)peek(t)))
                return 0;
            advance(t);
        }
        return 1;
    default:
        return 0;
    }
}

/* ------------------------------------------------------------------ */
/*  Char literal                                                      */
/* ------------------------------------------------------------------ */

static Token scan_char(Tokenizer *t, const char *start,
                       int line, int col) {
    if (is_at_end(t) || peek(t) == '\n')
        return error_token(t, "Unterminated char literal", line, col);

    if (peek(t) == '\\') {
        advance(t); /* consume \ */
        if (!scan_escape(t))
            return error_token(t, "Invalid escape in char literal", line, col);
    } else if (peek(t) == '\'') {
        return error_token(t, "Empty char literal", line, col);
    } else {
        advance(t); /* the character */
    }

    if (is_at_end(t) || peek(t) != '\'')
        return error_token(t, "Unterminated char literal", line, col);
    advance(t); /* closing ' */
    return make_token(t, TOK_CHAR_LIT, start, line, col);
}

/* ------------------------------------------------------------------ */
/*  String literal                                                    */
/* ------------------------------------------------------------------ */

static Token scan_string(Tokenizer *t, const char *start,
                         int line, int col) {
    while (!is_at_end(t) && peek(t) != '"') {
        if (peek(t) == '\n')
            return error_token(t, "Unterminated string literal", line, col);
        if (peek(t) == '\\') {
            advance(t); /* consume \ */
            if (!scan_escape(t))
                return error_token(t, "Invalid escape in string", line, col);
        } else {
            advance(t);
        }
    }
    if (is_at_end(t))
        return error_token(t, "Unterminated string literal", line, col);
    advance(t); /* closing " */
    return make_token(t, TOK_STRING_LIT, start, line, col);
}

/* ------------------------------------------------------------------ */
/*  Template literal                                                  */
/*                                                                    */
/*  Scanning works as follows:                                        */
/*    - On seeing `, scan until ${ or `. If ${, emit TEMPLATE_START   */
/*      and increment template_depth. If `, emit TEMPLATE_FULL.       */
/*    - When template_depth > 0 and we see }, we re-enter the         */
/*      template: scan until next ${ or `. If ${, emit                */
/*      TEMPLATE_MIDDLE. If `, emit TEMPLATE_END and decrement depth. */
/* ------------------------------------------------------------------ */

static Token scan_template_body(Tokenizer *t, const char *start,
                                int line, int col, TokenType on_interp,
                                TokenType on_end) {
    while (!is_at_end(t)) {
        if (peek(t) == '\\') {
            advance(t);
            if (!scan_escape(t))
                return error_token(t, "Invalid escape in template literal",
                                   line, col);
            continue;
        }
        if (peek(t) == '$' && peek_next(t) == '{') {
            Token tok = make_token(t, on_interp, start, line, col);
            if (t->template_depth >= TOKENIZER_MAX_TEMPLATE_DEPTH)
                return error_token(t, "Template interpolation nesting too deep",
                                   line, col);
            advance(t); /* $ */
            advance(t); /* { */
            t->interpolation_brace_depth[t->template_depth] = 0;
            t->template_depth++;
            return tok;
        }
        if (peek(t) == '`') {
            advance(t); /* closing ` */
            return make_token(t, on_end, start, line, col);
        }
        advance(t);
    }
    return error_token(t, "Unterminated template literal", line, col);
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
        advance(t); /* consume } */
        t->template_depth--;
        return scan_template_body(t, start, line, col,
                                  TOK_TEMPLATE_MIDDLE, TOK_TEMPLATE_END);
    }

    if (is_at_end(t)) {
        return make_token(t, TOK_EOF, t->current, t->line, t->column);
    }

    const char *start = t->current;
    int line = t->line;
    int col  = t->column;
    char c = advance(t);

    /* Identifiers and keywords */
    if (isalpha((unsigned char)c) || c == '_')
        return scan_identifier(t, start, line, col);

    /* Numbers */
    if (isdigit((unsigned char)c))
        return scan_number(t, start, line, col);

    switch (c) {
    /* Single-character punctuation */
    case '(': return make_token(t, TOK_LPAREN,    start, line, col);
    case ')': return make_token(t, TOK_RPAREN,    start, line, col);
    case '{':
        if (t->template_depth > 0)
            t->interpolation_brace_depth[t->template_depth - 1]++;
        return make_token(t, TOK_LBRACE,    start, line, col);
    case '}':
        if (t->template_depth > 0 &&
            t->interpolation_brace_depth[t->template_depth - 1] > 0) {
            t->interpolation_brace_depth[t->template_depth - 1]--;
        }
        return make_token(t, TOK_RBRACE,    start, line, col);
    case '[': return make_token(t, TOK_LBRACKET,  start, line, col);
    case ']': return make_token(t, TOK_RBRACKET,  start, line, col);
    case ';': return make_token(t, TOK_SEMICOLON, start, line, col);
    case ',': return make_token(t, TOK_COMMA,     start, line, col);
    case '.': return make_token(t, TOK_DOT,       start, line, col);
    case '?': return make_token(t, TOK_QUESTION,  start, line, col);
    case ':': return make_token(t, TOK_COLON,     start, line, col);

    /* - and -> */
    case '-':
        if (match(t, '>')) return make_token(t, TOK_ARROW,        start, line, col);
        if (match(t, '=')) return make_token(t, TOK_MINUS_ASSIGN, start, line, col);
        return make_token(t, TOK_MINUS, start, line, col);

    /* + */
    case '+':
        if (match(t, '=')) return make_token(t, TOK_PLUS_ASSIGN, start, line, col);
        return make_token(t, TOK_PLUS, start, line, col);

    /* * */
    case '*':
        if (match(t, '=')) return make_token(t, TOK_STAR_ASSIGN, start, line, col);
        return make_token(t, TOK_STAR, start, line, col);

    /* / */
    case '/':
        if (match(t, '=')) return make_token(t, TOK_SLASH_ASSIGN, start, line, col);
        return make_token(t, TOK_SLASH, start, line, col);

    /* % */
    case '%':
        if (match(t, '=')) return make_token(t, TOK_PERCENT_ASSIGN, start, line, col);
        return make_token(t, TOK_PERCENT, start, line, col);

    /* = and == */
    case '=':
        if (match(t, '=')) return make_token(t, TOK_EQ,     start, line, col);
        return make_token(t, TOK_ASSIGN, start, line, col);

    /* ! and != */
    case '!':
        if (match(t, '=')) return make_token(t, TOK_NEQ,  start, line, col);
        return make_token(t, TOK_BANG, start, line, col);

    /* < <= << <<= */
    case '<':
        if (match(t, '<')) {
            if (match(t, '=')) return make_token(t, TOK_LSHIFT_ASSIGN, start, line, col);
            return make_token(t, TOK_LSHIFT, start, line, col);
        }
        if (match(t, '=')) return make_token(t, TOK_LTE, start, line, col);
        return make_token(t, TOK_LT, start, line, col);

    /* > >= >> >>= */
    case '>':
        if (match(t, '>')) {
            if (match(t, '=')) return make_token(t, TOK_RSHIFT_ASSIGN, start, line, col);
            return make_token(t, TOK_RSHIFT, start, line, col);
        }
        if (match(t, '=')) return make_token(t, TOK_GTE, start, line, col);
        return make_token(t, TOK_GT, start, line, col);

    /* & && &= */
    case '&':
        if (match(t, '&')) return make_token(t, TOK_LOGIC_AND,  start, line, col);
        if (match(t, '=')) return make_token(t, TOK_AMP_ASSIGN, start, line, col);
        return make_token(t, TOK_AMP, start, line, col);

    /* | || |= */
    case '|':
        if (match(t, '|')) return make_token(t, TOK_LOGIC_OR,    start, line, col);
        if (match(t, '=')) return make_token(t, TOK_PIPE_ASSIGN, start, line, col);
        return make_token(t, TOK_PIPE, start, line, col);

    /* ^ ^= */
    case '^':
        if (match(t, '=')) return make_token(t, TOK_CARET_ASSIGN, start, line, col);
        return make_token(t, TOK_CARET, start, line, col);

    /* ~ ~& ~^ */
    case '~':
        if (match(t, '&')) return make_token(t, TOK_TILDE_AMP,   start, line, col);
        if (match(t, '^')) return make_token(t, TOK_TILDE_CARET, start, line, col);
        return make_token(t, TOK_TILDE, start, line, col);

    /* $ (standalone — ${  is handled inside templates) */
    case '$':
        if (match(t, '{')) return make_token(t, TOK_DOLLAR_LBRACE, start, line, col);
        return error_token(t, "Unexpected character '$'", line, col);

    /* Char literal */
    case '\'':
        return scan_char(t, start, line, col);

    /* String literal */
    case '"':
        return scan_string(t, start, line, col);

    /* Template literal */
    case '`':
        return scan_template_body(t, start, line, col,
                                  TOK_TEMPLATE_START, TOK_TEMPLATE_FULL);

    default:
        return error_token(t, "Unexpected character", line, col);
    }
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
    case TOK_LPAREN:         return "LPAREN";
    case TOK_RPAREN:         return "RPAREN";
    case TOK_LBRACE:         return "LBRACE";
    case TOK_RBRACE:         return "RBRACE";
    case TOK_LBRACKET:       return "LBRACKET";
    case TOK_RBRACKET:       return "RBRACKET";
    case TOK_SEMICOLON:      return "SEMICOLON";
    case TOK_COMMA:          return "COMMA";
    case TOK_DOT:            return "DOT";
    case TOK_ARROW:          return "ARROW";
    case TOK_QUESTION:       return "QUESTION";
    case TOK_COLON:          return "COLON";
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
    case TOK_MINUS:          return "MINUS";
    case TOK_STAR:           return "STAR";
    case TOK_SLASH:          return "SLASH";
    case TOK_PERCENT:        return "PERCENT";
    case TOK_BANG:           return "BANG";
    case TOK_COUNT:          return "COUNT";
    }
    return "UNKNOWN";
}
