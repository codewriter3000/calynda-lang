# Parser

## Overview

The parser is the second phase of the Calynda compiler. It takes the stream of tokens from the tokenizer and constructs an Abstract Syntax Tree (AST) that represents the syntactic structure of the program.

## Purpose

- Convert flat token stream into hierarchical AST
- Enforce Calynda's grammatical rules
- Handle operator precedence and associativity
- Support lookahead for disambiguating constructs
- Provide detailed error messages with source locations

## Key Components

### Files

- **parser.c/h**: Main parser driver and public API
- **parser_decl.c**: Parsing top-level declarations (functions, types, imports)
- **parser_expr.c**: Expression parsing with precedence climbing
- **parser_binary.c**: Binary operator handling
- **parser_postfix.c**: Postfix expressions (calls, indexing, member access)
- **parser_stmt.c**: Statement parsing
- **parser_types.c**: Type annotation parsing
- **parser_union.c**: Union type declaration parsing
- **parser_literals.c**: Literal expression construction
- **parser_lookahead.c**: Multi-token lookahead for disambiguation
- **parser_utils.c**: Helper functions and error reporting
- **parser_internal.h**: Internal data structures and helpers

## Parsing Strategy

### Recursive Descent
The parser uses recursive descent parsing, where each grammatical construct has a corresponding parsing function.

### Operator Precedence
Binary operators are parsed using precedence climbing to correctly handle expressions like `a + b * c`.

### Lookahead
The parser uses multi-token lookahead to disambiguate:
- Generic instantiation vs. comparison operators (`Foo<Bar>` vs `a < b > c`)
- Function types vs. parenthesized expressions

## Data Structures

### Parser State
```c
typedef struct {
    Tokenizer   tokenizer;
    Token      *tokens;
    size_t      token_count;
    size_t      current;
    bool        pending_gt;    // For generic disambiguation
    ParserError error;
    bool        has_error;
} Parser;
```

### AST Output
The parser produces an `AstProgram` which contains:
- Package declaration
- Import statements
- Top-level declarations (functions, types, unions, asm blocks)

## Usage

```c
Parser parser;
parser_init(&parser, source_code);

AstProgram program;
ast_program_init(&program);

if (!parser_parse_program(&parser, &program)) {
    const ParserError *error = parser_get_error(&parser);
    // Handle error
}

parser_free(&parser);
```

## Design Notes

- The parser performs all tokenization upfront and stores tokens in an array
- This allows arbitrary lookahead for disambiguation
- Error recovery is limited; the parser stops at the first error
- The AST preserves source location information for all nodes
- The parser is designed to be simple and maintainable rather than highly optimized