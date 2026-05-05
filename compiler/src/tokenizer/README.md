# Tokenizer

## Overview

The tokenizer (lexical analyzer) is the first phase of the Calynda compiler. It converts raw source code text into a stream of tokens that can be consumed by the parser.

## Purpose

- Break down source code into meaningful lexical units (tokens)
- Handle all Calynda language syntax including keywords, operators, literals, and punctuation
- Support template literals with interpolation
- Track source location (line and column) for error reporting
- Handle special constructs like inline assembly blocks

## Key Components

### Files

- **tokenizer.c/h**: Main tokenizer implementation and public API
- **tokenizer_keywords.c**: Keyword recognition and classification
- **tokenizer_literals.c**: Parsing of numeric, string, character, and template literals
- **tokenizer_punctuation.c**: Recognition of operators and punctuation symbols
- **tokenizer_internal.h**: Internal helper functions and state management

### Token Types

The tokenizer recognizes several categories of tokens:

- **Literals**: `TOK_INT_LIT`, `TOK_FLOAT_LIT`, `TOK_CHAR_LIT`, `TOK_STRING_LIT`
- **Template Literals**: `TOK_TEMPLATE_START`, `TOK_TEMPLATE_MIDDLE`, `TOK_TEMPLATE_END`, `TOK_TEMPLATE_FULL`
- **Keywords**: `TOK_PACKAGE`, `TOK_IMPORT`, `TOK_VAR`, `TOK_RETURN`, etc.
- **Primitive Types**: `TOK_INT8`, `TOK_INT32`, `TOK_FLOAT64`, `TOK_BOOL`, etc.
- **Operators**: `TOK_PLUS`, `TOK_ASSIGN`, `TOK_EQ`, `TOK_LOGIC_AND`, etc.
- **Punctuation**: `TOK_LPAREN`, `TOK_SEMICOLON`, `TOK_DOT`, etc.

### Special Features

#### Template Literal Support
The tokenizer maintains a stack to track nested template literal interpolations (e.g., `` `Hello ${name}` ``), supporting up to 32 levels of nesting.

#### Inline Assembly
Special handling for `asm { ... }` blocks where the content is captured as raw text rather than tokenized.

## Data Structures

### Token
```c
typedef struct {
    TokenType   type;
    const char *start;   // Pointer into source
    size_t      length;
    int         line;
    int         column;
} Token;
```

### Tokenizer State
```c
typedef struct {
    const char *source;
    const char *current;
    int         line;
    int         column;
    int         template_depth;
    int         interpolation_brace_depth[TOKENIZER_MAX_TEMPLATE_DEPTH];
    int         asm_body_pending;
} Tokenizer;
```

## Usage

```c
Tokenizer t;
tokenizer_init(&t, source_code);

Token tok;
while ((tok = tokenizer_next(&t)).type != TOK_EOF) {
    // Process token
}
```

## Design Notes

- The tokenizer is a single-pass, forward-only scanner
- It does not backtrack; uses single-character lookahead
- Source positions are tracked for error reporting
- The tokenizer is designed to be fast and memory-efficient
- It operates directly on the source string without copying (zero-copy design)
## Changes in 1.0.0-alpha.6

- No tokenizer changes for this release. New keywords were not introduced — `var` is reused as a parameter modifier and `num` / `arr<?>` are recognised by the existing identifier/type-name infrastructure.
