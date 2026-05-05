Token tokenizer_scan_template_body(Tokenizer *t, const char *start,
                                   int line, int col, TokenType on_interp,
                                   TokenType on_end) {
    while (!is_at_end(t)) {
        if (peek(t) == '\\') {
            advance_char(t);
            if (!scan_escape(t))
                return error_token(t, "Invalid escape in template literal",
                                   line, col);
            continue;
        }
        if (peek(t) == '$' && peek_next(t) == '{') {
            Token tok = make_token(t, on_interp, start, line, col);
            if (t->template_depth >= TOKENIZER_MAX_TEMPLATE_DEPTH)
                return error_token(t, "Template interpolation nesting too deep",
                                   line, col);
            advance_char(t); /* $ */
            advance_char(t); /* { */
            t->interpolation_brace_depth[t->template_depth] = 0;
            t->template_depth++;
            return tok;
        }
        if (peek(t) == '`') {
            advance_char(t); /* closing ` */
            return make_token(t, on_end, start, line, col);
        }
        advance_char(t);
    }
    return error_token(t, "Unterminated template literal", line, col);
}
