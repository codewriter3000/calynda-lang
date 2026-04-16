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
/*  Test: keywords                                                    */
/* ------------------------------------------------------------------ */

void test_keywords(void) {
    const char *src =
        "package import start boot var public private final void "
        "return exit throw true false null "
        "int8 int16 int32 int64 uint8 uint16 uint32 uint64 "
        "float32 float64 bool char string "
        "export as internal static union manual arr ptr "
        "malloc calloc realloc free deref addr offset store cleanup stackalloc layout";

    TokenType expected[] = {
        TOK_PACKAGE, TOK_IMPORT, TOK_START, TOK_BOOT, TOK_VAR,
        TOK_PUBLIC, TOK_PRIVATE, TOK_FINAL, TOK_VOID,
        TOK_RETURN, TOK_EXIT, TOK_THROW, TOK_TRUE, TOK_FALSE, TOK_NULL,
        TOK_INT8, TOK_INT16, TOK_INT32, TOK_INT64,
        TOK_UINT8, TOK_UINT16, TOK_UINT32, TOK_UINT64,
        TOK_FLOAT32, TOK_FLOAT64, TOK_BOOL, TOK_CHAR, TOK_STRING,
        TOK_EXPORT, TOK_AS, TOK_INTERNAL, TOK_STATIC, TOK_UNION, TOK_MANUAL, TOK_ARR, TOK_PTR,
        TOK_MALLOC, TOK_CALLOC, TOK_REALLOC, TOK_FREE,
        TOK_DEREF, TOK_ADDR, TOK_OFFSET, TOK_STORE, TOK_CLEANUP, TOK_STACKALLOC, TOK_LAYOUT,
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

void test_identifiers(void) {
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

void test_integer_literals(void) {
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

void test_float_literals(void) {
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

void test_char_literals(void) {
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

void test_string_literals(void) {
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

