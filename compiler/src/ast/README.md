# Abstract Syntax Tree (AST)

## Overview

The AST module defines the data structures representing the syntactic structure of Calynda programs. It provides the foundation for all subsequent compiler phases.

## Purpose

- Define all AST node types for the Calynda language
- Provide constructors and destructors for AST nodes
- Support memory management for tree structures
- Enable tree traversal and manipulation
- Preserve source location information for error reporting

## Key Components

### Files

- **ast.c/h**: Core AST API and memory management
- **ast_types.h**: Type annotation structures
- **ast_decl_types.h**: Declaration node types (functions, variables, unions)
- **ast_expressions.c**: Expression node constructors
- **ast_statements.c**: Statement node constructors
- **ast_declarations.c**: Declaration node constructors
- **ast_dump.c/h**: AST visualization and debugging
- **ast_dump_expr.c**: Expression dumping
- **ast_dump_stmt.c**: Statement dumping
- **ast_dump_decl.c**: Declaration dumping
- **ast_dump_types.c**: Type annotation dumping
- **ast_dump_names.c**: Name formatting utilities
- **ast_dump_internal.h**: Internal dump helpers
- **ast_internal.h**: Internal AST utilities

## Node Categories

### Top-Level Declarations
- **Package Declaration**: Module identifier
- **Import Declarations**: Dependency imports
- **Function/Binding Declarations**: Top-level functions and constants
- **Union Declarations**: Algebraic data types
- **Assembly Declarations**: Inline assembly blocks

### Statements
- **Expression Statements**
- **Variable Declarations**
- **Return/Exit/Throw Statements**
- **Assignment Statements**

### Expressions
- **Literals**: integers, floats, strings, characters, booleans, null
- **Template Literals**: String interpolation
- **Identifiers and Qualified Names**
- **Binary/Unary Operations**
- **Function Calls**
- **Member Access and Indexing**
- **Lambda Expressions**
- **Array Literals**
- **Union Constructors**

### Types
- **Primitive Types**: `int32`, `float64`, `bool`, `char`, `string`, etc.
- **Array Types**: `[T]`, `[T; N]`
- **Named Types**: User-defined types with optional generic arguments
- **Void Type**

## Data Structure Highlights

### Source Locations
All AST nodes contain source span information:
```c
typedef struct {
    int start_line;
    int start_column;
    int end_line;
    int end_column;
} AstSourceSpan;
```

### Expressions
```c
typedef struct AstExpression {
    AstExpressionKind kind;
    AstSourceSpan     span;
    union {
        AstLiteral            literal;
        AstIdentifier         identifier;
        AstBinaryExpression   binary;
        AstCallExpression     call;
        // ... more variants
    } as;
} AstExpression;
```

## Memory Management

All AST nodes are allocated on the heap and must be explicitly freed using the provided `*_free()` functions. The AST owns all child nodes and will recursively free them.

## Debugging Support

The `ast_dump_*` functions provide human-readable visualization of the AST for debugging:
```c
ast_dump_program(stdout, &program);
```

## Design Notes

- The AST is immutable after construction (by convention)
- All strings are dynamically allocated and owned by the AST
- The AST preserves all information from the source, including comments (future work)
- Generic type arguments are represented as lists for flexibility