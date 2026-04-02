#ifndef CALYNDA_TOKENIZER_H
#define CALYNDA_TOKENIZER_H

#include <stddef.h>
#include <stdint.h>

#define TOKENIZER_MAX_TEMPLATE_DEPTH 32

/* ------------------------------------------------------------------ */
/*  Token types                                                       */
/* ------------------------------------------------------------------ */

typedef enum {
    /* Special */
    TOK_EOF = 0,
    TOK_ERROR,

    /* Literals */
    TOK_INT_LIT,          /* 42, 0xFF, 0b1010, 0o77         */
    TOK_FLOAT_LIT,        /* 3.14, 1e10, 2.5E-3             */
    TOK_CHAR_LIT,         /* 'a', '\n'                       */
    TOK_STRING_LIT,       /* "hello"                         */
    TOK_TEMPLATE_START,   /* ` ... up to first ${ or `       */
    TOK_TEMPLATE_MIDDLE,  /* } ... between interpolations    */
    TOK_TEMPLATE_END,     /* } ... to closing `              */
    TOK_TEMPLATE_FULL,    /* ` ... ` with no interpolations  */

    /* Identifier */
    TOK_IDENTIFIER,

    /* Keywords */
    TOK_PACKAGE,
    TOK_IMPORT,
    TOK_START,
    TOK_VAR,
    TOK_PUBLIC,
    TOK_PRIVATE,
    TOK_FINAL,
    TOK_VOID,
    TOK_RETURN,
    TOK_EXIT,
    TOK_THROW,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NULL,

    /* Primitive type keywords */
    TOK_INT8,
    TOK_INT16,
    TOK_INT32,
    TOK_INT64,
    TOK_UINT8,
    TOK_UINT16,
    TOK_UINT32,
    TOK_UINT64,
    TOK_FLOAT32,
    TOK_FLOAT64,
    TOK_BOOL,
    TOK_CHAR,
    TOK_STRING,

    /* Punctuation */
    TOK_LPAREN,           /* (   */
    TOK_RPAREN,           /* )   */
    TOK_LBRACE,           /* {   */
    TOK_RBRACE,           /* }   */
    TOK_LBRACKET,         /* [   */
    TOK_RBRACKET,         /* ]   */
    TOK_SEMICOLON,        /* ;   */
    TOK_COMMA,            /* ,   */
    TOK_DOT,              /* .   */
    TOK_ARROW,            /* ->  */
    TOK_QUESTION,         /* ?   */
    TOK_COLON,            /* :   */
    TOK_DOLLAR_LBRACE,    /* ${  */

    /* Assignment operators */
    TOK_ASSIGN,           /* =   */
    TOK_PLUS_ASSIGN,      /* +=  */
    TOK_MINUS_ASSIGN,     /* -=  */
    TOK_STAR_ASSIGN,      /* *=  */
    TOK_SLASH_ASSIGN,     /* /=  */
    TOK_PERCENT_ASSIGN,   /* %=  */
    TOK_AMP_ASSIGN,       /* &=  */
    TOK_PIPE_ASSIGN,      /* |=  */
    TOK_CARET_ASSIGN,     /* ^=  */
    TOK_LSHIFT_ASSIGN,    /* <<= */
    TOK_RSHIFT_ASSIGN,    /* >>= */

    /* Comparison / equality */
    TOK_EQ,               /* ==  */
    TOK_NEQ,              /* !=  */
    TOK_LT,               /* <   */
    TOK_GT,               /* >   */
    TOK_LTE,              /* <=  */
    TOK_GTE,              /* >=  */

    /* Logical */
    TOK_LOGIC_OR,         /* ||  */
    TOK_LOGIC_AND,        /* &&  */

    /* Bitwise */
    TOK_PIPE,             /* |   */
    TOK_AMP,              /* &   */
    TOK_CARET,            /* ^   */
    TOK_TILDE,            /* ~   */
    TOK_TILDE_AMP,        /* ~&  */
    TOK_TILDE_CARET,      /* ~^  */

    /* Shift */
    TOK_LSHIFT,           /* <<  */
    TOK_RSHIFT,           /* >>  */

    /* Arithmetic */
    TOK_PLUS,             /* +   */
    TOK_MINUS,            /* -   */
    TOK_STAR,             /* *   */
    TOK_SLASH,            /* /   */
    TOK_PERCENT,          /* %   */

    /* Unary */
    TOK_BANG,             /* !   */

    TOK_COUNT             /* sentinel — number of token types */
} TokenType;

/* ------------------------------------------------------------------ */
/*  Token                                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    TokenType   type;
    const char *start;   /* pointer into source */
    size_t      length;
    int         line;
    int         column;
} Token;

/* ------------------------------------------------------------------ */
/*  Tokenizer state                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *source;
    const char *current;
    int         line;
    int         column;
    int         template_depth;   /* nesting depth of template literals */
    int         interpolation_brace_depth[TOKENIZER_MAX_TEMPLATE_DEPTH];
} Tokenizer;

/* ------------------------------------------------------------------ */
/*  API                                                               */
/* ------------------------------------------------------------------ */

void  tokenizer_init(Tokenizer *t, const char *source);
Token tokenizer_next(Tokenizer *t);

/* Returns a human-readable name for a token type. */
const char *token_type_name(TokenType type);

#endif /* CALYNDA_TOKENIZER_H */
