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
#define RUN_TEST(fn) do {                                               \
    printf("  %s ...\n", #fn);                                          \
    fn();                                                               \
} while (0)


/* ------------------------------------------------------------------ */
/*  Test: template literals                                           */
/* ------------------------------------------------------------------ */

void test_template_literals(void) {
    Tokenizer t;
    Token tok;

    /* Full template with no interpolation */
    tokenizer_init(&t, "`hello world`");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_TEMPLATE_FULL, tok.type, "template full");

    /* Template with one interpolation: `hi ${name}!` */
    tokenizer_init(&t, "`hi ${name}!`");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_TEMPLATE_START, tok.type, "template start");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_IDENTIFIER, "name");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_TEMPLATE_END, tok.type, "template end");

    /* Template with two interpolations: `${a} and ${b}` */
    tokenizer_init(&t, "`${a} and ${b}`");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_TEMPLATE_START, tok.type, "tmpl start 2");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_IDENTIFIER, "a");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_TEMPLATE_MIDDLE, tok.type, "tmpl middle");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_IDENTIFIER, "b");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_TEMPLATE_END, tok.type, "tmpl end 2");
}


/* ------------------------------------------------------------------ */
/*  Test: template interpolation with nested braces                   */
/* ------------------------------------------------------------------ */

void test_template_interpolation_braces(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "`x ${(() -> { return 1; })()} y`");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_TEMPLATE_START, tok.type, "template start with nested braces");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_LPAREN, tok.type, "grouping lparen");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_LPAREN, tok.type, "lambda parameter lparen");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_RPAREN, tok.type, "lambda parameter rparen");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_ARROW, tok.type, "lambda arrow");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_LBRACE, tok.type, "lambda body lbrace");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_RETURN, tok.type, "return keyword in interpolation");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_INT_LIT, "1");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_SEMICOLON, tok.type, "semicolon in lambda body");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_RBRACE, tok.type, "lambda body rbrace");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_RPAREN, tok.type, "grouping rparen");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_LPAREN, tok.type, "call lparen");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_RPAREN, tok.type, "call rparen");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_TEMPLATE_END, tok.type, "template end after nested braces");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_EOF, tok.type, "EOF after nested-brace template");
}


/* ------------------------------------------------------------------ */
/*  Test: operators and punctuation                                   */
/* ------------------------------------------------------------------ */

void test_operators(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t,
        "( ) { } [ ] ; , . -> ? : "
        "= += -= *= /= %= &= |= ^= <<= >>= "
        "== != < > <= >= "
        "|| && "
        "| & ^ ~ ~& ~^ "
        "<< >> "
        "+ - * / % !");

    TokenType expected[] = {
        TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
        TOK_LBRACKET, TOK_RBRACKET, TOK_SEMICOLON, TOK_COMMA,
        TOK_DOT, TOK_ARROW, TOK_QUESTION, TOK_COLON,
        TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,
        TOK_STAR_ASSIGN, TOK_SLASH_ASSIGN, TOK_PERCENT_ASSIGN,
        TOK_AMP_ASSIGN, TOK_PIPE_ASSIGN, TOK_CARET_ASSIGN,
        TOK_LSHIFT_ASSIGN, TOK_RSHIFT_ASSIGN,
        TOK_EQ, TOK_NEQ, TOK_LT, TOK_GT, TOK_LTE, TOK_GTE,
        TOK_LOGIC_OR, TOK_LOGIC_AND,
        TOK_PIPE, TOK_AMP, TOK_CARET, TOK_TILDE,
        TOK_TILDE_AMP, TOK_TILDE_CARET,
        TOK_LSHIFT, TOK_RSHIFT,
        TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
        TOK_BANG,
        TOK_EOF
    };

    for (int i = 0; expected[i] != TOK_EOF; i++) {
        tok = tokenizer_next(&t);
        ASSERT_EQ_INT(expected[i], tok.type, token_type_name(expected[i]));
    }
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_EOF, tok.type, "EOF after operators");
}


/* ------------------------------------------------------------------ */
/*  Test: comments are skipped                                        */
/* ------------------------------------------------------------------ */

void test_comments(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "foo // line comment\nbar /* block */ baz");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_IDENTIFIER, "foo");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_IDENTIFIER, "bar");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_IDENTIFIER, "baz");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_EOF, tok.type, "EOF after comments");
}


/* ------------------------------------------------------------------ */
/*  Test: line and column tracking                                    */
/* ------------------------------------------------------------------ */

void test_positions(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "foo\nbar");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(1, tok.line, "foo line");
    ASSERT_EQ_INT(1, tok.column, "foo column");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(2, tok.line, "bar line");
    ASSERT_EQ_INT(1, tok.column, "bar column");
}

