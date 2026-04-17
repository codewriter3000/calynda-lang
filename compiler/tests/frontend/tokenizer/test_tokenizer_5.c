#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern int tests_run;
extern int tests_passed;
extern int tests_failed;

#define ASSERT_EQ_INT(expected, actual, msg) do {                       \
    tests_run++;                                                        \
    if ((expected) == (actual)) {                                       \
        tests_passed++;                                                 \
    } else {                                                            \
        tests_failed++;                                                 \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %d, got %d\n",    \
                __FILE__, __LINE__, (msg), (int)(expected), (int)(actual)); \
    }                                                                   \
} while (0)
#define ASSERT_EQ_STR(expected, actual, len, msg) do {                  \
    tests_run++;                                                        \
    if ((len) == strlen(expected) &&                                    \
        memcmp((expected), (actual), (len)) == 0) {                     \
        tests_passed++;                                                 \
    } else {                                                            \
        tests_failed++;                                                 \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected \"%s\", got \"%.*s\"\n", \
                __FILE__, __LINE__, (msg), (expected), (int)(len), (actual)); \
    }                                                                   \
} while (0)
#define ASSERT_TOKEN(tok, exp_type, exp_text) do {                      \
    ASSERT_EQ_INT((exp_type), (tok).type,                               \
                  "token type for \"" exp_text "\"");                    \
    ASSERT_EQ_STR((exp_text), (tok).start, (tok).length,                \
                  "token text");                                        \
} while (0)


/* ------------------------------------------------------------------ */
/*  G-TOK-5: Unterminated string produces TOK_ERROR with line info    */
/* ------------------------------------------------------------------ */
void test_unterminated_string_error(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "\"hello world");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_ERROR, tok.type, "unterminated string yields TOK_ERROR");
    ASSERT_EQ_INT(1, tok.line, "unterminated string error on line 1");

    /* Unterminated string after valid tokens */
    tokenizer_init(&t, "var x = \"open");
    do { tok = tokenizer_next(&t); } while (tok.type != TOK_ERROR && tok.type != TOK_EOF);
    ASSERT_EQ_INT(TOK_ERROR, tok.type, "unterminated string after assignment");
}

void test_structured_tokenizer_error(void) {
    Tokenizer t;
    Token tok;
    const TokenizerError *error;
    char formatted[256];

    tokenizer_init(&t, "\"hello world");
    ASSERT_EQ_INT(0, tokenizer_get_error(&t) != NULL,
                  "no structured tokenizer error before scanning");

    tok = tokenizer_next(&t);
    error = tokenizer_get_error(&t);

    ASSERT_EQ_INT(TOK_ERROR, tok.type, "unterminated string still yields TOK_ERROR");
    ASSERT_EQ_INT(1, error != NULL, "structured tokenizer error recorded");
    if (!error) {
        return;
    }
    ASSERT_EQ_INT(TOK_ERROR, error->token.type, "structured tokenizer error keeps token type");
    ASSERT_EQ_INT(1, error->token.line, "structured tokenizer error line");
    ASSERT_EQ_INT(1, error->token.column, "structured tokenizer error column");
    ASSERT_EQ_STR("Unterminated string literal",
                  error->message,
                  strlen(error->message),
                  "structured tokenizer error message");
    ASSERT_EQ_INT(1,
                  tokenizer_format_error(error, formatted, sizeof(formatted)),
                  "structured tokenizer error formats");
    ASSERT_EQ_STR("1:1: Unterminated string literal",
                  formatted,
                  strlen(formatted),
                  "formatted tokenizer error");
}


/* ------------------------------------------------------------------ */
/*  G-TOK-1: ~& and ~^ operator tokens                               */
/* ------------------------------------------------------------------ */
void test_tilde_operators(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "~& ~^");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_TILDE_AMP, "~&");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_TILDE_CARET, "~^");
}


/* ------------------------------------------------------------------ */
/*  G-TOK-2: ++ and -- tokens                                        */
/* ------------------------------------------------------------------ */
void test_increment_decrement_tokens(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "++ --");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_PLUS_PLUS, "++");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_MINUS_MINUS, "--");
}


/* ------------------------------------------------------------------ */
/*  G-TOK-3: All compound-assignment tokens                           */
/* ------------------------------------------------------------------ */
void test_compound_assignment_tokens(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "+= -= *= /= %= &= |= ^= <<= >>=");
    tok = tokenizer_next(&t); ASSERT_TOKEN(tok, TOK_PLUS_ASSIGN, "+=");
    tok = tokenizer_next(&t); ASSERT_TOKEN(tok, TOK_MINUS_ASSIGN, "-=");
    tok = tokenizer_next(&t); ASSERT_TOKEN(tok, TOK_STAR_ASSIGN, "*=");
    tok = tokenizer_next(&t); ASSERT_TOKEN(tok, TOK_SLASH_ASSIGN, "/=");
    tok = tokenizer_next(&t); ASSERT_TOKEN(tok, TOK_PERCENT_ASSIGN, "%=");
    tok = tokenizer_next(&t); ASSERT_TOKEN(tok, TOK_AMP_ASSIGN, "&=");
    tok = tokenizer_next(&t); ASSERT_TOKEN(tok, TOK_PIPE_ASSIGN, "|=");
    tok = tokenizer_next(&t); ASSERT_TOKEN(tok, TOK_CARET_ASSIGN, "^=");
    tok = tokenizer_next(&t); ASSERT_TOKEN(tok, TOK_LSHIFT_ASSIGN, "<<=");
    tok = tokenizer_next(&t); ASSERT_TOKEN(tok, TOK_RSHIFT_ASSIGN, ">>=");
}


/* ------------------------------------------------------------------ */
/*  G-TOK-12: Memory operation keywords tokenize correctly            */
/* ------------------------------------------------------------------ */
void test_memory_op_keywords(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "malloc calloc realloc free deref addr offset store stackalloc layout checked ptr arr manual");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_MALLOC, tok.type, "malloc keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_CALLOC, tok.type, "calloc keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_REALLOC, tok.type, "realloc keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_FREE, tok.type, "free keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_DEREF, tok.type, "deref keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_ADDR, tok.type, "addr keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_OFFSET, tok.type, "offset keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_STORE, tok.type, "store keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_STACKALLOC, tok.type, "stackalloc keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_LAYOUT, tok.type, "layout keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_CHECKED, tok.type, "checked keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_PTR, tok.type, "ptr keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_ARR, tok.type, "arr keyword");
    tok = tokenizer_next(&t); ASSERT_EQ_INT(TOK_MANUAL, tok.type, "manual keyword");
}


/* ------------------------------------------------------------------ */
/*  G-TOK-10: Ellipsis token                                         */
/* ------------------------------------------------------------------ */
void test_ellipsis_token(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "...");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_ELLIPSIS, "...");
}

void test_type_and_spawn_keywords(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "type spawn thread_local");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_TYPE, tok.type, "type keyword");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_SPAWN, tok.type, "spawn keyword");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_THREAD_LOCAL, tok.type, "thread_local keyword");
}


/* ------------------------------------------------------------------ */
/*  G-TOK-11: CRLF line endings advance line counter correctly        */
/* ------------------------------------------------------------------ */
void test_crlf_line_tracking(void) {
    Tokenizer t;
    Token tok;

    /* Simple two-line CRLF source: single identifier per line */
    tokenizer_init(&t, "a\r\nb");
    tok = tokenizer_next(&t); /* 'a' identifier on line 1 */
    ASSERT_EQ_INT(1, tok.line, "CRLF: first identifier is on line 1");
    tok = tokenizer_next(&t); /* 'b' identifier on line 2 */
    ASSERT_EQ_INT(2, tok.line, "CRLF: identifier after CRLF is on line 2");

    /* Three lines separated by CRLF */
    tokenizer_init(&t, "x\r\ny\r\nz");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(1, tok.line, "CRLF: three-line first token is on line 1");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(2, tok.line, "CRLF: three-line second token is on line 2");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(3, tok.line, "CRLF: three-line third token is on line 3");
}
