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

