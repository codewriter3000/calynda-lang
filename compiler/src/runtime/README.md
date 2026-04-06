# Runtime Library Interface

## Overview

The runtime module defines the interface between compiled Calynda code and the runtime library. It specifies data layouts, ABI conventions, and helper function signatures.

## Purpose

- Define runtime data structures and their binary layout
- Specify runtime function signatures (ABI)
- Document memory management conventions
- Enable compiler to generate correct runtime calls
- Support runtime introspection and debugging

## Key Components

### Files

- **runtime.h**: Public runtime API and data structures
- **runtime.c**: Runtime utility implementations
- **runtime_internal.h**: Internal runtime details
- **runtime_ops.c**: Runtime operation helpers
- **runtime_format.c**: Value formatting and display
- **runtime_union.c**: Union type support
- **runtime_names.c**: Type and operation names

## Runtime Data Types

### Value Representation
Calynda uses a 64-bit word for all values:
```c
typedef uint64_t CalyndaRtWord;
```

- **Primitives**: Encoded directly (int32, bool, float64)
- **Pointers**: Object references
- **Tagged Values**: Union variants

### Object Layout
All heap objects start with a header:
```c
typedef struct {
    uint32_t magic;  // 0x434C5944 ("CLYD")
    uint32_t kind;   // Object type discriminator
} CalyndaRtObjectHeader;
```

### String Objects
```c
typedef struct {
    CalyndaRtObjectHeader header;
    size_t                length;
    char                 *bytes;  // UTF-8
} CalyndaRtString;
```

### Array Objects
```c
typedef struct {
    CalyndaRtObjectHeader header;
    size_t                count;
    CalyndaRtWord        *elements;
} CalyndaRtArray;
```

### Closure Objects
```c
typedef struct {
    CalyndaRtObjectHeader header;
    CalyndaRtClosureEntry entry;          // Function pointer
    size_t                capture_count;
    CalyndaRtWord        *captures;
} CalyndaRtClosure;
```

### Union Objects
```c
typedef struct {
    CalyndaRtObjectHeader        header;
    const CalyndaRtTypeDescriptor *type_desc;
    uint32_t                      tag;      // Variant index
    CalyndaRtWord                 payload;
} CalyndaRtUnion;
```

### Heterogeneous Arrays
```c
typedef struct {
    CalyndaRtObjectHeader  header;
    size_t                 count;
    CalyndaRtWord         *elements;
    CalyndaRtTypeTag      *element_tags;  // Type for each element
} CalyndaRtHeteroArray;
```

## Runtime Functions

### Memory Management
```c
// Allocate object (GC-managed in future)
void* __calynda_rt_alloc(size_t size);

// Free object (explicit for now, GC in future)
void __calynda_rt_free(void* ptr);
```

### String Operations
```c
// Create string from C string
CalyndaRtWord calynda_rt_make_string_copy(const char *bytes);

// Get C string from Calynda string
const char *calynda_rt_string_bytes(CalyndaRtWord word);
```

### Closure Operations
```c
// Create closure with captures
CalyndaRtWord __calynda_rt_closure_new(
    CalyndaRtClosureEntry code_ptr,
    size_t capture_count,
    const CalyndaRtWord *captures
);

// Call closure or function
CalyndaRtWord __calynda_rt_call_callable(
    CalyndaRtWord callable,
    size_t argument_count,
    const CalyndaRtWord *arguments
);
```

### Array Operations
```c
// Create array from elements
CalyndaRtWord __calynda_rt_array_literal(
    size_t element_count,
    const CalyndaRtWord *elements
);

// Get array length
size_t calynda_rt_array_length(CalyndaRtWord word);

// Index load/store
CalyndaRtWord __calynda_rt_index_load(CalyndaRtWord target, CalyndaRtWord index);
void __calynda_rt_store_index(CalyndaRtWord target, CalyndaRtWord index, CalyndaRtWord value);
```

### Union Operations
```c
// Create union variant
CalyndaRtWord __calynda_rt_union_new(
    const CalyndaRtTypeDescriptor *type_desc,
    uint32_t variant_tag,
    CalyndaRtWord payload
);

// Get variant tag
uint32_t __calynda_rt_union_get_tag(CalyndaRtWord value);

// Extract payload
CalyndaRtWord __calynda_rt_union_get_payload(CalyndaRtWord value);
```

### Type Conversions
```c
// Cast between types (runtime checking)
CalyndaRtWord __calynda_rt_cast_value(
    CalyndaRtWord source,
    CalyndaRtTypeTag target_type_tag
);
```

### Template Literals
```c
// Build template string from parts
CalyndaRtWord __calynda_rt_template_build(
    size_t part_count,
    const CalyndaRtTemplatePart *parts
);
```

### Exception Handling
```c
// Throw exception (unwind stack)
void __calynda_rt_throw(CalyndaRtWord value) __attribute__((noreturn));
```

## Program Entry

### Start Function
```c
typedef CalyndaRtWord (*CalyndaRtProgramStartEntry)(CalyndaRtWord arguments);

int calynda_rt_start_process(
    CalyndaRtProgramStartEntry entry,
    int argc,
    char **argv
);
```

The runtime initializes and invokes the program's start function.

## Calling Convention

### Arguments
- Passed as array of `CalyndaRtWord`
- No distinction between primitives and objects at ABI level

### Return Values
- Returned as `CalyndaRtWord`
- Void functions return 0

### Captures (Closures)
- Passed separately from arguments
- Closure entry receives captures first, then arguments

## Memory Model

### Current: Manual Memory Management
- Objects explicitly allocated and freed
- Compiler inserts free calls (future: escape analysis)

### Future: Garbage Collection
- Mark-and-sweep or generational GC
- Automatic memory reclamation
- Cycle detection

## Standard Library

The runtime includes builtin objects:
```c
extern CalyndaRtPackage __calynda_pkg_stdlib;
```

Built-in callables:
- `print()`: Output to stdout
- `println()`: Output with newline
- String operations
- Math functions

## Debugging Support

### Runtime Introspection
```c
// Check if word is object pointer
bool calynda_rt_is_object(CalyndaRtWord word);

// Get object header
const CalyndaRtObjectHeader *calynda_rt_as_object(CalyndaRtWord word);

// Format value as string
bool calynda_rt_format_word(CalyndaRtWord word, char *buffer, size_t buffer_size);
```

### Layout Dumping
```c
// Dump runtime data structure layouts
bool calynda_rt_dump_layout(FILE *out);
```

## Platform-Specific Details

### x86-64 System V ABI
- `CalyndaRtWord` passed in general-purpose registers
- Return in RAX
- Closures: captures in RDI, args in RSI

### AArch64
- `CalyndaRtWord` passed in X0-X7
- Return in X0
- Closures: captures in X0, args in X1

## Design Notes

- Runtime is written in C for portability
- ABI is stable across compiler versions
- Runtime is linked as shared library (future: static option)
- Runtime functions are prefixed with `__calynda_rt_` or `calynda_rt_`
- Public API uses `calynda_rt_`, private uses `__calynda_rt_`