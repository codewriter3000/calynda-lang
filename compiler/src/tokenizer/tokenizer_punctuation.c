#include "tokenizer_internal.h"

/* ------------------------------------------------------------------ */
/*  Punctuation and operator scanning                                 */
/* ------------------------------------------------------------------ */

Token tokenizer_scan_punctuation(Tokenizer *t, char c,
                                 const char *start,
                                 int line, int col) {
    switch (c) {
    /* Single-character punctuation */
    case '(': return make_token(t, TOK_LPAREN,    start, line, col);
    case ')': return make_token(t, TOK_RPAREN,    start, line, col);
    case '{':
        if (t->template_depth > 0)
            t->interpolation_brace_depth[t->template_depth - 1]++;
        return make_token(t, TOK_LBRACE,    start, line, col);
    case '}':
        if (t->template_depth > 0 &&
            t->interpolation_brace_depth[t->template_depth - 1] > 0) {
            t->interpolation_brace_depth[t->template_depth - 1]--;
        }
        return make_token(t, TOK_RBRACE,    start, line, col);
    case '[': return make_token(t, TOK_LBRACKET,  start, line, col);
    case ']': return make_token(t, TOK_RBRACKET,  start, line, col);
    case ';': return make_token(t, TOK_SEMICOLON, start, line, col);
    case ',': return make_token(t, TOK_COMMA,     start, line, col);
    case '.':
        if (peek(t) == '.' && peek_next(t) == '.') {
            advance_char(t); /* second . */
            advance_char(t); /* third .  */
            return make_token(t, TOK_ELLIPSIS, start, line, col);
        }
        return make_token(t, TOK_DOT,       start, line, col);
    case '?': return make_token(t, TOK_QUESTION,  start, line, col);
    case ':': return make_token(t, TOK_COLON,     start, line, col);

    /* - -> -- -= */
    case '-':
        if (match_char(t, '>')) return make_token(t, TOK_ARROW,        start, line, col);
        if (match_char(t, '-')) return make_token(t, TOK_MINUS_MINUS,  start, line, col);
        if (match_char(t, '=')) return make_token(t, TOK_MINUS_ASSIGN, start, line, col);
        return make_token(t, TOK_MINUS, start, line, col);

    /* + ++ += */
    case '+':
        if (match_char(t, '+')) return make_token(t, TOK_PLUS_PLUS,   start, line, col);
        if (match_char(t, '=')) return make_token(t, TOK_PLUS_ASSIGN, start, line, col);
        return make_token(t, TOK_PLUS, start, line, col);

    /* * */
    case '*':
        if (match_char(t, '=')) return make_token(t, TOK_STAR_ASSIGN, start, line, col);
        return make_token(t, TOK_STAR, start, line, col);

    /* / */
    case '/':
        if (match_char(t, '=')) return make_token(t, TOK_SLASH_ASSIGN, start, line, col);
        return make_token(t, TOK_SLASH, start, line, col);

    /* % */
    case '%':
        if (match_char(t, '=')) return make_token(t, TOK_PERCENT_ASSIGN, start, line, col);
        return make_token(t, TOK_PERCENT, start, line, col);

    /* = and == */
    case '=':
        if (match_char(t, '=')) return make_token(t, TOK_EQ,     start, line, col);
        return make_token(t, TOK_ASSIGN, start, line, col);

    /* ! and != */
    case '!':
        if (match_char(t, '=')) return make_token(t, TOK_NEQ,  start, line, col);
        return make_token(t, TOK_BANG, start, line, col);

    /* < <= << <<= */
    case '<':
        if (match_char(t, '<')) {
            if (match_char(t, '=')) return make_token(t, TOK_LSHIFT_ASSIGN, start, line, col);
            return make_token(t, TOK_LSHIFT, start, line, col);
        }
        if (match_char(t, '=')) return make_token(t, TOK_LTE, start, line, col);
        return make_token(t, TOK_LT, start, line, col);

    /* > >= ><
       >> >>= */
    case '>':
        if (match_char(t, '<')) return make_token(t, TOK_SWAP, start, line, col);
        if (match_char(t, '>')) {
            if (match_char(t, '=')) return make_token(t, TOK_RSHIFT_ASSIGN, start, line, col);
            return make_token(t, TOK_RSHIFT, start, line, col);
        }
        if (match_char(t, '=')) return make_token(t, TOK_GTE, start, line, col);
        return make_token(t, TOK_GT, start, line, col);

    /* & && &= */
    case '&':
        if (match_char(t, '&')) return make_token(t, TOK_LOGIC_AND,  start, line, col);
        if (match_char(t, '=')) return make_token(t, TOK_AMP_ASSIGN, start, line, col);
        return make_token(t, TOK_AMP, start, line, col);

    /* | || |= */
    case '|':
        if (match_char(t, '|')) return make_token(t, TOK_LOGIC_OR,    start, line, col);
        if (match_char(t, '=')) return make_token(t, TOK_PIPE_ASSIGN, start, line, col);
        return make_token(t, TOK_PIPE, start, line, col);

    /* ^ ^= */
    case '^':
        if (match_char(t, '=')) return make_token(t, TOK_CARET_ASSIGN, start, line, col);
        return make_token(t, TOK_CARET, start, line, col);

    /* ~ ~& ~^ */
    case '~':
        if (match_char(t, '&')) return make_token(t, TOK_TILDE_AMP,   start, line, col);
        if (match_char(t, '^')) return make_token(t, TOK_TILDE_CARET, start, line, col);
        return make_token(t, TOK_TILDE, start, line, col);

    /* $ (standalone — ${  is handled inside templates) */
    case '$':
        if (match_char(t, '{')) return make_token(t, TOK_DOLLAR_LBRACE, start, line, col);
        return error_token(t, "Unexpected character '$'", line, col);

    /* Char literal */
    case '\'':
        return tokenizer_scan_char(t, start, line, col);

    /* String literal */
    case '"':
        return tokenizer_scan_string(t, start, line, col);

    /* Template literal */
    case '`':
        return tokenizer_scan_template_body(t, start, line, col,
                                            TOK_TEMPLATE_START, TOK_TEMPLATE_FULL);

    default:
        return error_token(t, "Unexpected character", line, col);
    }
}
