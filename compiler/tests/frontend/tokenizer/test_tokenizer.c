#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int tests_run    = 0;
int tests_passed = 0;
int tests_failed = 0;

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

void test_keywords(void);
void test_identifiers(void);
void test_integer_literals(void);
void test_float_literals(void);
void test_char_literals(void);
void test_string_literals(void);
void test_template_literals(void);
void test_template_interpolation_braces(void);
void test_operators(void);
void test_comments(void);
void test_positions(void);
void test_realistic_snippet(void);
void test_errors(void);
void test_asm_tokens(void);
void test_unterminated_string_error(void);
void test_tilde_operators(void);
void test_increment_decrement_tokens(void);
void test_compound_assignment_tokens(void);
void test_memory_op_keywords(void);
void test_ellipsis_token(void);


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
    RUN_TEST(test_asm_tokens);
    RUN_TEST(test_unterminated_string_error);
    RUN_TEST(test_tilde_operators);
    RUN_TEST(test_increment_decrement_tokens);
    RUN_TEST(test_compound_assignment_tokens);
    RUN_TEST(test_memory_op_keywords);
    RUN_TEST(test_ellipsis_token);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
