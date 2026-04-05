#ifndef CALYNDA_LIR_H
#define CALYNDA_LIR_H

#include "lir_instr_types.h"

typedef enum {
    LIR_TERM_NONE = 0,
    LIR_TERM_RETURN,
    LIR_TERM_JUMP,
    LIR_TERM_BRANCH,
    LIR_TERM_THROW
} LirTerminatorKind;

typedef struct {
    LirTerminatorKind kind;
    union {
        struct {
            bool       has_value;
            LirOperand value;
        } return_term;
        struct {
            size_t target_block;
        } jump_term;
        struct {
            LirOperand condition;
            size_t     true_block;
            size_t     false_block;
        } branch_term;
        struct {
            LirOperand value;
        } throw_term;
    } as;
} LirTerminator;

struct LirBasicBlock {
    char          *label;
    LirInstruction *instructions;
    size_t         instruction_count;
    size_t         instruction_capacity;
    LirTerminator  terminator;
};

typedef enum {
    LIR_UNIT_START = 0,
    LIR_UNIT_BINDING,
    LIR_UNIT_INIT,
    LIR_UNIT_LAMBDA,
    LIR_UNIT_ASM
} LirUnitKind;

struct LirUnit {
    LirUnitKind    kind;
    char          *name;
    const Symbol  *symbol;
    CheckedType    return_type;
    LirSlot       *slots;
    size_t         slot_count;
    size_t         slot_capacity;
    size_t         capture_count;
    size_t         parameter_count;
    size_t         virtual_register_count;
    LirBasicBlock *blocks;
    size_t         block_count;
    size_t         block_capacity;
    /* ASM unit fields */
    char          *asm_body;
    size_t         asm_body_length;
    /* Boot flag for START units */
    bool           is_boot;
};

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} LirBuildError;

typedef struct {
    LirUnit      *units;
    size_t        unit_count;
    size_t        unit_capacity;
    LirBuildError error;
    bool          has_error;
} LirProgram;

void lir_program_init(LirProgram *program);
void lir_program_free(LirProgram *program);

bool lir_build_program(LirProgram *program, const MirProgram *mir_program);

const LirBuildError *lir_get_error(const LirProgram *program);
bool lir_format_error(const LirBuildError *error,
                      char *buffer,
                      size_t buffer_size);

bool lir_dump_program(FILE *out, const LirProgram *program);
char *lir_dump_program_to_string(const LirProgram *program);

#endif /* CALYNDA_LIR_H */