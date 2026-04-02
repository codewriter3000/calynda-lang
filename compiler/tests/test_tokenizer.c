#include "../src/tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Minimal test harness                                              */
/* ------------------------------------------------------------------ */

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

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
/*  Test: keywords                                                    */
/* ------------------------------------------------------------------ */

static void test_keywords(void) {
    const char *src =
        "package import start var public private final void "
        "return exit throw true false null "
        "int8 int16 int32 int64 uint8 uint16 uint32 uint64 "
        "float32 float64 bool char string";

    TokenType expected[] = {
        TOK_PACKAGE, TOK_IMPORT, TOK_START, TOK_VAR,
        TOK_PUBLIC, TOK_PRIVATE, TOK_FINAL, TOK_VOID,
        TOK_RETURN, TOK_EXIT, TOK_THROW, TOK_TRUE, TOK_FALSE, TOK_NULL,
        TOK_INT8, TOK_INT16, TOK_INT32, TOK_INT64,
        TOK_UINT8, TOK_UINT16, TOK_UINT32, TOK_UINT64,
        TOK_FLOAT32, TOK_FLOAT64, TOK_BOOL, TOK_CHAR, TOK_STRING,
        TOK_EOF
    };

    Tokenizer t;
    tokenizer_init(&t, src);
    for (int i = 0; expected[i] != TOK_EOF; i++) {
        Token tok = tokenizer_next(&t);
        ASSERT_EQ_INT(expected[i], tok.type, "keyword type");
    }
    Token eof = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_EOF, eof.type, "EOF after keywords");
}

/* ------------------------------------------------------------------ */
/*  Test: identifiers                                                 */
/* ------------------------------------------------------------------ */

static void test_identifiers(void) {
    Tokenizer t;
    tokenizer_init(&t, "foo _bar baz123 _0");
    Token tok;

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_IDENTIFIER, "foo");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_IDENTIFIER, "_bar");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_IDENTIFIER, "baz123");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_IDENTIFIER, "_0");

    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_EOF, tok.type, "EOF");
}

/* ------------------------------------------------------------------ */
/*  Test: integer literals                                            */
/* ------------------------------------------------------------------ */

static void test_integer_literals(void) {
    Tokenizer t;

    /* Decimal */
    tokenizer_init(&t, "0 42 1234567890");
    Token tok;

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_INT_LIT, "0");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_INT_LIT, "42");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_INT_LIT, "1234567890");

    /* Binary */
    tokenizer_init(&t, "0b1010 0B11");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_INT_LIT, "0b1010");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_INT_LIT, "0B11");

    /* Hex */
    tokenizer_init(&t, "0xFF 0X1A");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_INT_LIT, "0xFF");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_INT_LIT, "0X1A");

    /* Octal */
    tokenizer_init(&t, "0o77 0O01");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_INT_LIT, "0o77");
    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_INT_LIT, "0O01");
}

/* ------------------------------------------------------------------ */
/*  Test: float literals                                              */
/* ------------------------------------------------------------------ */

static void test_float_literals(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "3.14 1.0e10 2.5E-3 42e2");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_FLOAT_LIT, "3.14");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_FLOAT_LIT, "1.0e10");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_FLOAT_LIT, "2.5E-3");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_FLOAT_LIT, "42e2");
}

/* ------------------------------------------------------------------ */
/*  Test: char literals                                               */
/* ------------------------------------------------------------------ */

static void test_char_literals(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "'a' '\\n' '\\\\' '\\u0041'");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_CHAR_LIT, "'a'");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_CHAR_LIT, "'\\n'");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_CHAR_LIT, "'\\\\'");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_CHAR_LIT, "'\\u0041'");
}

/* ------------------------------------------------------------------ */
/*  Test: string literals                                             */
/* ------------------------------------------------------------------ */

static void test_string_literals(void) {
    Tokenizer t;
    Token tok;

    tokenizer_init(&t, "\"hello\" \"with\\nescapes\" \"\"");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_STRING_LIT, "\"hello\"");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_STRING_LIT, "\"with\\nescapes\"");

    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_STRING_LIT, "\"\"");
}

/* ------------------------------------------------------------------ */
/*  Test: template literals                                           */
/* ------------------------------------------------------------------ */

static void test_template_literals(void) {
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

static void test_template_interpolation_braces(void) {
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

static void test_operators(void) {
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

static void test_comments(void) {
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

static void test_positions(void) {
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

/* ------------------------------------------------------------------ */
/*  Test: a realistic Calynda snippet                                 */
/* ------------------------------------------------------------------ */

static void test_realistic_snippet(void) {
    const char *src =
        "package com.example;\n"
        "import io.stdlib;\n"
        "\n"
        "public int32 add = (int32 a, int32 b) -> a + b;\n"
        "\n"
        "start(string[] args) -> {\n"
        "    var result = add(1, 2);\n"
        "    return 0;\n"
        "};\n";

    Tokenizer t;
    tokenizer_init(&t, src);

    /* Just verify we can tokenize without errors and reach EOF */
    Token tok;
    int error_count = 0;
    int token_count = 0;
    do {
        tok = tokenizer_next(&t);
        if (tok.type == TOK_ERROR) error_count++;
        token_count++;
    } while (tok.type != TOK_EOF);

    ASSERT_EQ_INT(0, error_count, "no errors in realistic snippet");
    tests_run++;
    if (token_count > 10) {
        tests_passed++;
    } else {
        tests_failed++;
        fprintf(stderr, "  FAIL: expected many tokens, got %d\n", token_count);
    }
}

/* ------------------------------------------------------------------ */
/*  Test: error cases                                                 */
/* ------------------------------------------------------------------ */

static void test_errors(void) {
    Tokenizer t;
    Token tok;

    /* Unterminated string */
    tokenizer_init(&t, "\"hello");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_ERROR, tok.type, "unterminated string");

    /* Unterminated char */
    tokenizer_init(&t, "'a");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_ERROR, tok.type, "unterminated char");

    /* Empty char */
    tokenizer_init(&t, "''");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_ERROR, tok.type, "empty char");

    /* Bad binary literal */
    tokenizer_init(&t, "0b");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_ERROR, tok.type, "bad binary literal");

    /* Bad hex literal */
    tokenizer_init(&t, "0xG");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_ERROR, tok.type, "bad hex literal");
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int main(void) {
    printf("Running tokenizer tests...\n\n");

    RUN_TEST(test_keywords);
    RUN_TEST(test_identifiers);
    RUN_TEST(test_integer_literals);
    RUN_TEST(test_float_literals);
    RUN_TEST(test_char_literals);
    RUN_TEST(test_string_literals);
    RUN_TEST(test_template_literals);
    RUN_TEST(test_template_interpolation_braces);
    RUN_TEST(test_operators);
    RUN_TEST(test_comments);
    RUN_TEST(test_positions);
    RUN_TEST(test_realistic_snippet);
    RUN_TEST(test_errors);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
