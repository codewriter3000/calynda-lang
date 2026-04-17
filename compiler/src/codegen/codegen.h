#ifndef CALYNDA_CODEGEN_H
#define CALYNDA_CODEGEN_H

#include "lir.h"
#include "target.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct CodegenBlock CodegenBlock;
typedef struct CodegenUnit CodegenUnit;

typedef TargetKind CodegenTargetKind;

#define CODEGEN_TARGET_X86_64_SYSV_ELF TARGET_KIND_X86_64_SYSV_ELF

typedef int CodegenRegister;

/* Backward-compatible register constants (x86_64 register IDs) */
#define CODEGEN_REG_RAX  0
#define CODEGEN_REG_RDI  1
#define CODEGEN_REG_RSI  2
#define CODEGEN_REG_RDX  3
#define CODEGEN_REG_RCX  4
#define CODEGEN_REG_R8   5
#define CODEGEN_REG_R9   6
#define CODEGEN_REG_R10  7
#define CODEGEN_REG_R11  8
#define CODEGEN_REG_R12  9
#define CODEGEN_REG_R13  10
#define CODEGEN_REG_R14  11
#define CODEGEN_REG_R15  12

typedef enum {
    CODEGEN_VREG_REGISTER = 0,
    CODEGEN_VREG_SPILL
} CodegenVRegLocationKind;

typedef struct {
    CodegenVRegLocationKind kind;
    union {
        CodegenRegister reg;
        size_t          spill_slot_index;
    } as;
} CodegenVRegLocation;

typedef struct {
    size_t      lir_slot_index;
    size_t      frame_slot_index;
    LirSlotKind kind;
    char       *name;
    CheckedType type;
    bool        is_final;
} CodegenFrameSlot;

typedef struct {
    size_t             vreg_index;
    CheckedType        type;
    CodegenVRegLocation location;
} CodegenVRegAllocation;

typedef enum {
    CODEGEN_SELECTION_DIRECT = 0,
    CODEGEN_SELECTION_RUNTIME
} CodegenSelectionKind;

typedef enum {
    CODEGEN_DIRECT_ABI_ARG_MOVE = 0,
    CODEGEN_DIRECT_ABI_CAPTURE_LOAD,
    CODEGEN_DIRECT_ABI_OUTGOING_ARG,
    CODEGEN_DIRECT_SCALAR_BINARY,
    CODEGEN_DIRECT_SCALAR_UNARY,
    CODEGEN_DIRECT_SCALAR_CAST,
    CODEGEN_DIRECT_CALL_GLOBAL,
    CODEGEN_DIRECT_STORE_SLOT,
    CODEGEN_DIRECT_STORE_GLOBAL,
    CODEGEN_DIRECT_RETURN,
    CODEGEN_DIRECT_JUMP,
    CODEGEN_DIRECT_BRANCH
} CodegenDirectPattern;

typedef enum {
    CODEGEN_RUNTIME_CLOSURE_NEW = 0,
    CODEGEN_RUNTIME_CALL_CALLABLE,
    CODEGEN_RUNTIME_MEMBER_LOAD,
    CODEGEN_RUNTIME_INDEX_LOAD,
    CODEGEN_RUNTIME_ARRAY_LITERAL,
    CODEGEN_RUNTIME_TEMPLATE_BUILD,
    CODEGEN_RUNTIME_STORE_INDEX,
    CODEGEN_RUNTIME_STORE_MEMBER,
    CODEGEN_RUNTIME_THROW,
    CODEGEN_RUNTIME_CAST_VALUE,
    CODEGEN_RUNTIME_UNION_NEW,
    CODEGEN_RUNTIME_UNION_GET_TAG,
    CODEGEN_RUNTIME_UNION_GET_PAYLOAD,
    CODEGEN_RUNTIME_HETERO_ARRAY_NEW,
    CODEGEN_RUNTIME_HETERO_ARRAY_GET_TAG,
    CODEGEN_RUNTIME_THREAD_SPAWN,
    CODEGEN_RUNTIME_THREAD_JOIN,
    CODEGEN_RUNTIME_THREAD_CANCEL,
    CODEGEN_RUNTIME_MUTEX_NEW,
    CODEGEN_RUNTIME_MUTEX_LOCK,
    CODEGEN_RUNTIME_MUTEX_UNLOCK,
    /* Future helpers (alpha.2) */
    CODEGEN_RUNTIME_FUTURE_SPAWN,
    CODEGEN_RUNTIME_FUTURE_GET,
    CODEGEN_RUNTIME_FUTURE_CANCEL,
    /* Atomic helpers (alpha.2) */
    CODEGEN_RUNTIME_ATOMIC_NEW,
    CODEGEN_RUNTIME_ATOMIC_LOAD,
    CODEGEN_RUNTIME_ATOMIC_STORE,
    CODEGEN_RUNTIME_ATOMIC_EXCHANGE,
    CODEGEN_RUNTIME_TYPEOF,
    CODEGEN_RUNTIME_ISINT,
    CODEGEN_RUNTIME_ISFLOAT,
    CODEGEN_RUNTIME_ISBOOL,
    CODEGEN_RUNTIME_ISSTRING,
    CODEGEN_RUNTIME_ISARRAY,
    CODEGEN_RUNTIME_ISSAMETYPE
} CodegenRuntimeHelper;

typedef struct {
    CodegenSelectionKind kind;
    union {
        CodegenDirectPattern direct_pattern;
        CodegenRuntimeHelper runtime_helper;
    } as;
} CodegenSelection;

typedef struct {
    LirInstructionKind kind;
    CodegenSelection   selection;
} CodegenSelectedInstruction;

typedef struct {
    LirTerminatorKind kind;
    CodegenSelection  selection;
} CodegenSelectedTerminator;

struct CodegenBlock {
    char                       *label;
    CodegenSelectedInstruction *instructions;
    size_t                      instruction_count;
    CodegenSelectedTerminator   terminator;
};

struct CodegenUnit {
    LirUnitKind            kind;
    char                  *name;
    const Symbol          *symbol;
    CheckedType            return_type;
    CodegenFrameSlot      *frame_slots;
    size_t                 frame_slot_count;
    CodegenVRegAllocation *vreg_allocations;
    size_t                 vreg_count;
    size_t                 spill_slot_count;
    CodegenBlock          *blocks;
    size_t                 block_count;
    /* ASM unit fields */
    char                  *asm_body;
    size_t                 asm_body_length;
    /* Boot flag for START units */
    bool                   is_boot;
};

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} CodegenBuildError;

typedef struct {
    CodegenTargetKind        target;
    const TargetDescriptor  *target_desc;
    CodegenUnit             *units;
    size_t                   unit_count;
    CodegenBuildError        error;
    bool                     has_error;
} CodegenProgram;

void codegen_program_init(CodegenProgram *program);
void codegen_program_free(CodegenProgram *program);

bool codegen_build_program(CodegenProgram *program,
                           const LirProgram *lir_program,
                           const TargetDescriptor *target);

const CodegenBuildError *codegen_get_error(const CodegenProgram *program);
bool codegen_format_error(const CodegenBuildError *error,
                          char *buffer,
                          size_t buffer_size);

const char *codegen_target_name(CodegenTargetKind target);
const char *codegen_register_name(CodegenRegister reg);
const char *codegen_direct_pattern_name(CodegenDirectPattern pattern);
const char *codegen_runtime_helper_name(CodegenRuntimeHelper helper);

bool codegen_dump_program(FILE *out, const CodegenProgram *program);
char *codegen_dump_program_to_string(const CodegenProgram *program);

#endif /* CALYNDA_CODEGEN_H */
