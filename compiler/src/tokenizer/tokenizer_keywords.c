#include "tokenizer_internal.h"

/* ------------------------------------------------------------------ */
/*  Keyword table                                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *text;
    TokenType   type;
} Keyword;

static const Keyword keywords[] = {
    {"package",  TOK_PACKAGE},
    {"import",   TOK_IMPORT},
    {"start",    TOK_START},
    {"boot",     TOK_BOOT},
    {"var",      TOK_VAR},
    {"public",   TOK_PUBLIC},
    {"private",  TOK_PRIVATE},
    {"final",    TOK_FINAL},
    {"void",     TOK_VOID},
    {"return",   TOK_RETURN},
    {"exit",     TOK_EXIT},
    {"throw",    TOK_THROW},
    {"true",     TOK_TRUE},
    {"false",    TOK_FALSE},
    {"null",     TOK_NULL},
    {"export",   TOK_EXPORT},
    {"as",       TOK_AS},
    {"internal", TOK_INTERNAL},
    {"static",   TOK_STATIC},
    {"thread_local", TOK_THREAD_LOCAL},
    {"union",    TOK_UNION},
    {"type",     TOK_TYPE},
    {"manual",   TOK_MANUAL},
    {"arr",      TOK_ARR},
    {"ptr",      TOK_PTR},
    {"malloc",   TOK_MALLOC},
    {"calloc",   TOK_CALLOC},
    {"realloc",  TOK_REALLOC},
    {"free",     TOK_FREE},
    {"deref",    TOK_DEREF},
    {"addr",     TOK_ADDR},
    {"offset",   TOK_OFFSET},
    {"store",    TOK_STORE},
    {"cleanup",  TOK_CLEANUP},
    {"stackalloc", TOK_STACKALLOC},
    {"layout",   TOK_LAYOUT},
    {"checked",  TOK_CHECKED},
    {"asm",      TOK_ASM},
    {"spawn",    TOK_SPAWN},
    {"int8",     TOK_INT8},
    {"int16",    TOK_INT16},
    {"int32",    TOK_INT32},
    {"int64",    TOK_INT64},
    {"uint8",    TOK_UINT8},
    {"uint16",   TOK_UINT16},
    {"uint32",   TOK_UINT32},
    {"uint64",   TOK_UINT64},
    {"float32",  TOK_FLOAT32},
    {"float64",  TOK_FLOAT64},
    {"bool",     TOK_BOOL},
    {"char",     TOK_CHAR},
    {"string",   TOK_STRING},
    {"byte",     TOK_BYTE},
    {"sbyte",    TOK_SBYTE},
    {"short",    TOK_SHORT},
    {"int",      TOK_INT},
    {"long",     TOK_LONG},
    {"ulong",    TOK_ULONG},
    {"uint",     TOK_UINT},
    {"float",    TOK_FLOAT},
    {"double",   TOK_DOUBLE},
    {NULL, TOK_EOF}
};

static TokenType check_keyword(const char *start, size_t len) {
    for (int i = 0; keywords[i].text != NULL; i++) {
        if (strlen(keywords[i].text) == len &&
            memcmp(keywords[i].text, start, len) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENTIFIER;
}

/* ------------------------------------------------------------------ */
/*  Identifier / keyword                                              */
/* ------------------------------------------------------------------ */

Token tokenizer_scan_identifier(Tokenizer *t, const char *start,
                                int line, int col) {
    while (!is_at_end(t) && (isalnum((unsigned char)peek(t)) || peek(t) == '_'))
        advance_char(t);
    size_t len = (size_t)(t->current - start);
    /* A standalone '_' is the discard token, not an identifier. */
    if (len == 1 && start[0] == '_')
        return make_token(t, TOK_UNDERSCORE, start, line, col);
    TokenType type = check_keyword(start, len);
    return make_token(t, type, start, line, col);
}
