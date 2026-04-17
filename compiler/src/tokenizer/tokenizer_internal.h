#ifndef CALYNDA_TOKENIZER_INTERNAL_H
#define CALYNDA_TOKENIZER_INTERNAL_H

#include "tokenizer.h"
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/*  Shared inline helpers for tokenizer split files                   */
/* ------------------------------------------------------------------ */

static inline int is_at_end(const Tokenizer *t) {
    return *t->current == '\0';
}

static inline char peek(const Tokenizer *t) {
    return *t->current;
}

static inline char peek_next(const Tokenizer *t) {
    if (is_at_end(t)) return '\0';
    return t->current[1];
}

static inline char advance_char(Tokenizer *t) {
    char c = *t->current++;
    if (c == '\n') {
        t->line++;
        t->column = 1;
    } else {
        t->column++;
    }
    return c;
}

static inline int match_char(Tokenizer *t, char expected) {
    if (is_at_end(t) || *t->current != expected) return 0;
    advance_char(t);
    return 1;
}

static inline Token make_token(const Tokenizer *t, TokenType type,
                               const char *start, int line, int col) {
    Token tok;
    tok.type   = type;
    tok.start  = start;
    tok.length = (size_t)(t->current - start);
    tok.line   = line;
    tok.column = col;
    return tok;
}

static inline void tokenizer_set_error(Tokenizer *t,
                                       const Token *token,
                                       const char *msg) {
    size_t length;

    if (!t || !token || !msg) {
        return;
    }

    length = strlen(msg);
    if (length >= sizeof(t->error.message)) {
        length = sizeof(t->error.message) - 1;
    }

    t->has_error = true;
    t->error.token = *token;
    memcpy(t->error.message, msg, length);
    t->error.message[length] = '\0';
}

static inline Token error_token(Tokenizer *t, const char *msg,
                                 int line, int col) {
    Token tok;

    tok.type   = TOK_ERROR;
    tok.start  = msg;
    tok.length = strlen(msg);
    tok.line   = line;
    tok.column = col;
    tokenizer_set_error(t, &tok, msg);
    return tok;
}

/* ------------------------------------------------------------------ */
/*  Cross-file function declarations                                  */
/* ------------------------------------------------------------------ */

Token tokenizer_scan_identifier(Tokenizer *t, const char *start,
                                int line, int col);
Token tokenizer_scan_number(Tokenizer *t, const char *start,
                            int line, int col);
Token tokenizer_scan_char(Tokenizer *t, const char *start,
                          int line, int col);
Token tokenizer_scan_string(Tokenizer *t, const char *start,
                            int line, int col);
Token tokenizer_scan_template_body(Tokenizer *t, const char *start,
                                   int line, int col,
                                   TokenType on_interp,
                                   TokenType on_end);
Token tokenizer_scan_punctuation(Tokenizer *t, char c,
                                 const char *start,
                                 int line, int col);

#endif
