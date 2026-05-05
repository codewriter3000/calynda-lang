# High-Level Intermediate Representation (HIR)

## Overview

The HIR is the first intermediate representation in the Calynda compiler. It lowers the AST into a simpler, more explicit form while maintaining high-level constructs like lambdas and complex expressions.

## Purpose

- Simplify the AST into a more uniform representation
- Make control flow explicit
- Desugar complex language constructs
- Preserve high-level semantics for optimization
- Prepare for further lowering to MIR

## Key Components

### Files

- **hir.c/h**: HIR program structure and API
- **hir_types.h**: HIR node type definitions
- **hir_expr_types.h**: Expression node types
- **hir_lower.c**: Main lowering driver (AST → HIR)
- **hir_lower_decls.c**: Declaration lowering
- **hir_lower_expr.c**: Expression lowering
- **hir_lower_expr_ext.c**: Extended expression lowering
- **hir_lower_stmt.c**: Statement lowering
- **hir_helpers.c**: Helper functions
- **hir_memory.c**: Memory management
- **hir_internal.h**: Internal data structures
- **hir_dump.c/h**: HIR visualization
- **hir_dump_expr.c**: Expression dumping
- **hir_dump_expr_ext.c**: Extended expression dumping
- **hir_dump_helpers.c**: Dump utilities
- **hir_dump_internal.h**: Internal dump helpers

## Transformations

### From AST to HIR

1. **Explicit Typing**: All expressions are annotated with their checked types
2. **Name Resolution**: All identifiers are resolved to symbols
3. **Desugaring**: Complex constructs are broken down:
   - String templates → concatenation operations
   - Compound assignments (`+=`) → read + write
   - Increment/decrement → explicit operations

4. **Control Flow**: Still uses structured statements (if, while, return)
5. **Lambda Capture**: Lambda expressions record their captured variables

## Data Structures

### HIR Program
```c
typedef struct {
    HirUnit *units;
    size_t   unit_count;
} HirProgram;
```

### HIR Unit
Each top-level function/lambda becomes a unit:
```c
typedef struct {
    char        *name;
    Symbol      *symbol;
    CheckedType  return_type;
    HirBlock    *body;
} HirUnit;
```

### HIR Expressions
HIR expressions are similar to AST expressions but:
- All names are resolved to symbols
- All have explicit type information
- Template literals are desugared

### HIR Statements
- Variable declarations with initialization
- Expression statements
- Return statements
- Assignment statements

## What HIR is NOT

- HIR is **not** in SSA form (Static Single Assignment)
- HIR does **not** have explicit basic blocks
- HIR still has **structured control flow** (if/while/return)
- HIR does **not** perform register allocation

## Usage

```c
HirProgram hir;
hir_program_init(&hir);

if (!hir_build_program(&hir, &ast_program, &symbols, &checker)) {
    const HirBuildError *error = hir_get_error(&hir);
    // Handle error
}

hir_program_free(&hir);
```

## Design Notes

- HIR maintains a close correspondence to the original source
- HIR is easier to analyze than AST due to explicit types and resolved names
- HIR is the last representation that preserves lambda closures explicitly
- Future optimizations can be performed at the HIR level
## Changes in 1.0.0-alpha.6

- HIR lowering now distinguishes value-captured and reference-captured locals. Lambdas observe writes to enclosing scope through the new reference-capture path (`hir_lower_expr*`).
- `var` parameters lower to opaque-typed locals; reads/writes go through runtime type queries instead of static dispatch.
- `|var` parameters lower to non-local-return writes that pair with the runtime NLR slot stack (`runtime_nlr.c`).
- New surface forms (`num`, `arr<?>`, `string` argument to `car`/`cdr`) are normalised here before MIR.
