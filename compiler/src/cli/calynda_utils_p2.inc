    /* Error line */
    line_ptr = caly_diag_find_line(source, line);
    if (!line_ptr) return;
    line_len = 0;
    while (line_ptr[line_len] && line_ptr[line_len] != '\n') line_len++;
    fprintf(stderr, " %*d | %.*s\n", gutter, line, line_len, line_ptr);

    /* Caret underline */
    if (col_start > 0) {
        int cs = col_start;
        int ce = (col_end >= col_start && col_end > 0) ? col_end : col_start;
        int carets = ce - cs + 1;
        if (carets < 1) carets = 1;
        fprintf(stderr, " %*s | ", gutter, "");
        for (i = 1; i < cs; i++) fputc(' ', stderr);
        for (i = 0; i < carets; i++) fputc('^', stderr);
        fputc('\n', stderr);
    }
}
