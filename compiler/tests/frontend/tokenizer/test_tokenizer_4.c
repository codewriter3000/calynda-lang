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
/*  Test: a realistic Calynda snippet                                 */
/* ------------------------------------------------------------------ */

void test_realistic_snippet(void) {
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

void test_errors(void) {
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


void test_asm_tokens(void) {
    Tokenizer t;
    Token tok;

    /* TOK_ASM is recognized as a keyword */
    tokenizer_init(&t, "asm");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_ASM, tok.type, "asm keyword recognized");

    /* TOK_ASM followed by '(' doesn't trigger asm body mode */
    tokenizer_init(&t, "asm(");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_ASM, tok.type, "asm keyword before paren");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_LPAREN, tok.type, "paren after asm keyword");

    /* TOK_ASM_BODY captures raw text between { and } once asm_body_pending is set */
    tokenizer_init(&t, "asm");
    tok = tokenizer_next(&t);
    ASSERT_EQ_INT(TOK_ASM, tok.type, "asm keyword sets pending flag");
    /* Simulate what the parser would see: the tokenizer has asm_body_pending=1 */
    /* Push a raw body */
    tokenizer_init(&t, "asm { mov eax, 1\n ret\n }");
    tok = tokenizer_next(&t);  /* TOK_ASM, sets pending */
    ASSERT_EQ_INT(TOK_ASM, tok.type, "asm keyword in body sequence");
    tok = tokenizer_next(&t);  /* should be TOK_ASM_BODY */
    ASSERT_EQ_INT(TOK_ASM_BODY, tok.type, "asm body token captured");
    ASSERT_EQ_INT(1, tok.length > 0 ? 1 : 0, "asm body token has non-zero length");

    /* Nested braces in the body are handled correctly */
    tokenizer_init(&t, "asm { if { inner } outer }");
    tok = tokenizer_next(&t);  /* TOK_ASM */
    ASSERT_EQ_INT(TOK_ASM, tok.type, "asm keyword for nested brace test");
    tok = tokenizer_next(&t);  /* TOK_ASM_BODY */
    ASSERT_EQ_INT(TOK_ASM_BODY, tok.type, "nested brace asm body captured");
}

