# Semantic Analysis

## Overview

The semantic analysis phase performs symbol resolution and type checking on the AST. This directory contains the symbol table builder and type checker.

## Purpose

- Build symbol tables with scope hierarchy
- Resolve all identifiers to their declarations
- Perform type checking and type inference
- Validate language semantics (definite assignment, accessibility, etc.)
- Provide detailed error messages for semantic errors

## Key Components

### Symbol Table

#### Files
- **symbol_table.c/h**: Symbol table API and data structures
- **symbol_table_core.c**: Core symbol table operations
- **symbol_table_predecl.c**: First pass: collect all declarations
- **symbol_table_analyze.c**: Second pass: analyze function bodies
- **symbol_table_analyze_expr.c**: Expression analysis for symbol resolution
- **symbol_table_imports.c**: Import resolution and management
- **symbol_table_query.c**: Symbol lookup operations
- **symbol_table_registry.c**: Tracking symbol resolutions
- **symbol_table_internal.h**: Internal helpers

#### Responsibilities
- Create lexical scopes for packages, functions, blocks, and unions
- Register symbols for all declarations
- Resolve identifier expressions to their symbol definitions
- Track unresolved identifiers for error reporting
- Handle import statements and qualified names

### Type Checker

#### Files
- **type_checker.c/h**: Type checker API and infrastructure
- **type_checker_expr.c**: Expression type checking
- **type_checker_expr_ext.c**: Extended expression type checking
- **type_checker_expr_more.c**: Additional expression cases
- **type_checker_block.c**: Block and statement type checking
- **type_checker_lambda.c**: Lambda expression type checking
- **type_checker_check.c**: Type compatibility checking
- **type_checker_convert.c**: Type conversion rules
- **type_checker_ops.c**: Operator type checking
- **type_checker_types.c**: Type manipulation utilities
- **type_checker_resolve.c**: Symbol type resolution
- **type_checker_util.c**: Utility functions
- **type_checker_names.c**: Type name formatting
- **type_checker_internal.h**: Internal helpers

#### Responsibilities
- Infer types for expressions
- Check type compatibility for assignments and operations
- Validate function call arguments
- Check return statement types
- Handle generic type instantiation
- Ensure type safety for all operations

### Type Resolution

#### Files
- **type_resolution.c/h**: Type annotation resolution
- **type_resolution_expr.c**: Resolve types in expressions
- **type_resolution_resolve.c**: Resolve named types to definitions
- **type_resolution_helpers.c**: Helper functions
- **type_resolution_internal.h**: Internal data structures

#### Responsibilities
- Resolve `AstType` annotations to concrete types
- Handle array dimensions and generic arguments
- Validate type names against symbol table
- Support forward references in type declarations

### Semantic Dumping

#### Files
- **semantic_dump.c/h**: Semantic information visualization
- **semantic_dump_scope.c**: Scope hierarchy dumping
- **semantic_dump_symbols.c**: Symbol table dumping
- **semantic_dump_types.c**: Type information dumping
- **semantic_dump_builder.c**: Formatted output building
- **semantic_dump_internal.h**: Internal dump helpers

## Data Structures

### Symbol
```c
typedef struct Symbol {
    SymbolKind      kind;
    char           *name;
    char           *qualified_name;
    const AstType  *declared_type;
    bool            is_final;
    bool            is_exported;
    size_t          generic_param_count;
    AstSourceSpan   declaration_span;
    Scope          *scope;
} Symbol;
```

### Scope
```c
typedef struct Scope {
    ScopeKind  kind;
    Scope      *parent;
    Scope     **children;
    Symbol    **symbols;
} Scope;
```

### CheckedType
```c
typedef struct {
    CheckedTypeKind  kind;
    AstPrimitiveType primitive;
    size_t           array_depth;
    const char      *name;
    size_t           generic_arg_count;
} CheckedType;
```

## Multi-Pass Analysis

### Pass 1: Symbol Table Building
1. Create scopes for all declarations
2. Register all top-level symbols
3. Process imports

### Pass 2: Type Resolution
1. Resolve all type annotations
2. Build type descriptors

### Pass 3: Type Checking
1. Type check all expressions
2. Infer types where needed
3. Validate type compatibility

## Usage

```c
// Build symbol table
SymbolTable symbols;
symbol_table_init(&symbols);
if (!symbol_table_build(&symbols, &program)) {
    // Handle error
}

// Type check
TypeChecker checker;
type_checker_init(&checker);
if (!type_checker_check_program(&checker, &program, &symbols)) {
    // Handle error
}
```

## Design Notes

- The symbol table is built in two passes to handle forward references
- Type checking happens after full symbol resolution
- Generic types are validated but not instantiated at this stage
- The type system is designed to prevent common programming errors