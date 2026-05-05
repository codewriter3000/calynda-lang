    tok = tokenizer_next(&t);
    ASSERT_TOKEN(tok, TOK_FLOAT_LIT, "1_e10");
    normalized = copy_token_text_without_underscores(tok);
    value = normalized ? strtod(normalized, NULL) : 0.0;
    tests_run++;
    if (normalized && value == 1e10) {
        tests_passed++;
    } else {
        tests_failed++;
        fprintf(stderr, "  FAIL [%s:%d] underscored exponent value parsed incorrectly\n",
                __FILE__, __LINE__);
    }
    free(normalized);
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
