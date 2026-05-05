#include "tokenizer_internal.h"

/* ------------------------------------------------------------------ */
/*  Number literals                                                   */
/* ------------------------------------------------------------------ */

typedef int (*DigitPredicate)(char c);

static int is_binary_digit_char(char c) {
    return c == '0' || c == '1';
}

static int is_octal_digit_char(char c) {
    return c >= '0' && c <= '7';
}

static int is_decimal_digit_char(char c) {
    return isdigit((unsigned char)c);
}

static int is_hex_digit_char(char c) {
    return isxdigit((unsigned char)c);
}

static int scan_digits_with_underscores(Tokenizer *t,
                                        DigitPredicate is_digit,
                                        size_t *digit_count,
                                        int *last_was_underscore) {
    while (!is_at_end(t)) {
        char c = peek(t);
        if (c == '_') {
            if (*last_was_underscore) {
                return 0;
            }
            advance_char(t);
            *last_was_underscore = 1;
            continue;
        }
        if (!is_digit(c)) {
            break;
        }
        advance_char(t);
        (*digit_count)++;
        *last_was_underscore = 0;
    }
    return 1;
}

Token tokenizer_scan_number(Tokenizer *t, const char *start,
                            int line, int col) {
    /* Check for 0b, 0o, 0x prefixes */
    if (start[0] == '0' && !is_at_end(t)) {
        char next = peek(t);
        if (next == 'b' || next == 'B') {
            size_t digit_count = 0;
            int last_was_underscore = 0;
            advance_char(t); /* consume b/B */
            if (is_at_end(t) || (peek(t) != '0' && peek(t) != '1' && peek(t) != '_'))
                return error_token(t, "Expected binary digit after 0b", line, col);
            if (!scan_digits_with_underscores(t, is_binary_digit_char,
                                              &digit_count,
                                              &last_was_underscore)) {
                return error_token(t, "Invalid underscore in numeric literal", line, col);
            }
            if (digit_count == 0)
                return error_token(t, "Expected binary digit after 0b", line, col);
            if (last_was_underscore)
                return error_token(t, "Invalid underscore in numeric literal", line, col);
            return make_token(t, TOK_INT_LIT, start, line, col);
        }
        if (next == 'o' || next == 'O') {
            size_t digit_count = 0;
            int last_was_underscore = 0;
            advance_char(t); /* consume o/O */
            if (is_at_end(t) || (!is_octal_digit_char(peek(t)) && peek(t) != '_'))
                return error_token(t, "Expected octal digit after 0o", line, col);
            if (!scan_digits_with_underscores(t, is_octal_digit_char,
                                              &digit_count,
                                              &last_was_underscore)) {
                return error_token(t, "Invalid underscore in numeric literal", line, col);
            }
            if (digit_count == 0)
                return error_token(t, "Expected octal digit after 0o", line, col);
            if (last_was_underscore)
                return error_token(t, "Invalid underscore in numeric literal", line, col);
            return make_token(t, TOK_INT_LIT, start, line, col);
        }
        if (next == 'x' || next == 'X') {
            size_t digit_count = 0;
            int last_was_underscore = 0;
            advance_char(t); /* consume x/X */
            if (is_at_end(t) || (!is_hex_digit_char(peek(t)) && peek(t) != '_'))
                return error_token(t, "Expected hex digit after 0x", line, col);
            if (!scan_digits_with_underscores(t, is_hex_digit_char,
                                              &digit_count,
                                              &last_was_underscore)) {
                return error_token(t, "Invalid underscore in numeric literal", line, col);
            }
            if (digit_count == 0)
                return error_token(t, "Expected hex digit after 0x", line, col);
            if (last_was_underscore)
                return error_token(t, "Invalid underscore in numeric literal", line, col);
            return make_token(t, TOK_INT_LIT, start, line, col);
        }
    }

    /* Decimal digits */
    size_t digit_count = 1;
    int last_was_underscore = 0;
    if (!scan_digits_with_underscores(t, is_decimal_digit_char,
                                      &digit_count,
                                      &last_was_underscore)) {
        return error_token(t, "Invalid underscore in numeric literal", line, col);
    }

    int is_float = 0;

    /* Fractional part */
    if (!is_at_end(t) && peek(t) == '.') {
        if (last_was_underscore)
            return error_token(t, "Invalid underscore in numeric literal", line, col);
        if (peek_next(t) == '_')
            return error_token(t, "Invalid underscore in numeric literal", line, col);
    }
    if (!is_at_end(t) && peek(t) == '.' && isdigit((unsigned char)peek_next(t))) {
        size_t fractional_digit_count = 0;
        is_float = 1;
        advance_char(t); /* consume . */
        last_was_underscore = 0;
        if (!scan_digits_with_underscores(t, is_decimal_digit_char,
                                          &fractional_digit_count,
                                          &last_was_underscore)) {
            return error_token(t, "Invalid underscore in numeric literal", line, col);
        }
    }

    /* Exponent */
    if (!is_at_end(t) && (peek(t) == 'e' || peek(t) == 'E')) {
        size_t exponent_digit_count = 0;
        is_float = 1;
        advance_char(t); /* consume e/E */
        if (!is_at_end(t) && (peek(t) == '+' || peek(t) == '-'))
            advance_char(t);
        last_was_underscore = 0;
        if (is_at_end(t) || (!isdigit((unsigned char)peek(t)) && peek(t) != '_'))
            return error_token(t, "Expected digit in exponent", line, col);
        if (!scan_digits_with_underscores(t, is_decimal_digit_char,
                                          &exponent_digit_count,
                                          &last_was_underscore)) {
            return error_token(t, "Invalid underscore in numeric literal", line, col);
        }
        if (exponent_digit_count == 0)
            return error_token(t, "Expected digit in exponent", line, col);
    }

    if (last_was_underscore) {
        return error_token(t, "Invalid underscore in numeric literal", line, col);
    }

    return make_token(t, is_float ? TOK_FLOAT_LIT : TOK_INT_LIT,
                      start, line, col);
}

/* ------------------------------------------------------------------ */
/*  Escape sequences (shared by char, string, template)               */
/* ------------------------------------------------------------------ */

static int scan_escape(Tokenizer *t) {
    if (is_at_end(t)) return 0;
    char c = advance_char(t);
    switch (c) {
    case 'n': case 't': case 'r': case '\\': case '\'':
    case '"': case '`': case '$': case '0':
        return 1;
    case 'u':
        for (int i = 0; i < 4; i++) {
            if (is_at_end(t) || !isxdigit((unsigned char)peek(t)))
                return 0;
            advance_char(t);
        }
        return 1;
    default:
        return 0;
    }
}

/* ------------------------------------------------------------------ */
/*  Char literal                                                      */
/* ------------------------------------------------------------------ */

Token tokenizer_scan_char(Tokenizer *t, const char *start,
                          int line, int col) {
    if (is_at_end(t) || peek(t) == '\n')
        return error_token(t, "Unterminated char literal", line, col);

    if (peek(t) == '\\') {
        advance_char(t); /* consume \ */
        if (!scan_escape(t))
            return error_token(t, "Invalid escape in char literal", line, col);
    } else if (peek(t) == '\'') {
        return error_token(t, "Empty char literal", line, col);
    } else {
        advance_char(t); /* the character */
    }

    if (is_at_end(t) || peek(t) != '\'')
        return error_token(t, "Unterminated char literal", line, col);
    advance_char(t); /* closing ' */
    return make_token(t, TOK_CHAR_LIT, start, line, col);
}

/* ------------------------------------------------------------------ */
/*  String literal                                                    */
/* ------------------------------------------------------------------ */

Token tokenizer_scan_string(Tokenizer *t, const char *start,
                            int line, int col) {
    while (!is_at_end(t) && peek(t) != '"') {
        if (peek(t) == '\n')
            return error_token(t, "Unterminated string literal", line, col);
        if (peek(t) == '\\') {
            advance_char(t); /* consume \ */
            if (!scan_escape(t))
                return error_token(t, "Invalid escape in string", line, col);
        } else {
            advance_char(t);
        }
    }
    if (is_at_end(t))
        return error_token(t, "Unterminated string literal", line, col);
    advance_char(t); /* closing " */
    return make_token(t, TOK_STRING_LIT, start, line, col);
}

/* ------------------------------------------------------------------ */
/*  Template literal                                                  */
/* ------------------------------------------------------------------ */

#include "tokenizer_literals_p2.inc"
