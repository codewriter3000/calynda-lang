#ifndef CALYNDA_CODEGEN_H
#define CALYNDA_CODEGEN_H

#include "lir.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct CodegenBlock CodegenBlock;
typedef struct CodegenUnit CodegenUnit;

typedef enum {
    CODEGEN_TARGET_X86_64_SYSV_ELF = 0
} CodegenTargetKind;

typedef enum {
    CODEGEN_REG_RAX = 0,
    CODEGEN_REG_RDI,
    CODEGEN_REG_RSI,
    CODEGEN_REG_RDX,
    CODEGEN_REG_RCX,
    CODEGEN_REG_R8,
    CODEGEN_REG_R9,
    CODEGEN_REG_R10,
    CODEGEN_REG_R11,
    CODEGEN_REG_R12,
    CODEGEN_REG_R13,
    CODEGEN_REG_R14,
    CODEGEN_REG_R15
} CodegenRegister;

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
    CODEGEN_RUNTIME_CAST_VALUE
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
};

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} CodegenBuildError;

typedef struct {
    CodegenTargetKind target;
    CodegenUnit      *units;
    size_t            unit_count;
    CodegenBuildError error;
    bool              has_error;
} CodegenProgram;

void codegen_program_init(CodegenProgram *program);
void codegen_program_free(CodegenProgram *program);

bool codegen_build_program(CodegenProgram *program, const LirProgram *lir_program);

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