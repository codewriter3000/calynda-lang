#ifndef CALYNDA_MIR_H
#define CALYNDA_MIR_H

#include "mir_instr_types.h"

typedef enum {
    MIR_TERM_NONE = 0,
    MIR_TERM_RETURN,
    MIR_TERM_GOTO,
    MIR_TERM_BRANCH,
    MIR_TERM_THROW
} MirTerminatorKind;

typedef struct {
    MirTerminatorKind kind;
    union {
        struct {
            bool     has_value;
            MirValue value;
        } return_term;
        struct {
            size_t target_block;
        } goto_term;
        struct {
            MirValue condition;
            size_t   true_block;
            size_t   false_block;
        } branch_term;
        struct {
            MirValue value;
        } throw_term;
    } as;
} MirTerminator;

struct MirBasicBlock {
    char          *label;
    MirInstruction *instructions;
    size_t         instruction_count;
    size_t         instruction_capacity;
    MirTerminator  terminator;
};

typedef enum {
    MIR_UNIT_START = 0,
    MIR_UNIT_BINDING,
    MIR_UNIT_INIT,
    MIR_UNIT_LAMBDA,
    MIR_UNIT_ASM
} MirUnitKind;

struct MirUnit {
    MirUnitKind    kind;
    char          *name;
    const Symbol  *symbol;
    CheckedType    return_type;
    MirLocal      *locals;
    size_t         local_count;
    size_t         local_capacity;
    size_t         parameter_count;
    size_t         next_temp_index;
    MirBasicBlock *blocks;
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
} MirBuildError;

typedef struct {
    MirUnit      *units;
    size_t        unit_count;
    size_t        unit_capacity;
    MirBuildError error;
    bool          has_error;
} MirProgram;

void mir_program_init(MirProgram *program);
void mir_program_free(MirProgram *program);

bool mir_build_program(MirProgram *program, const HirProgram *hir_program);

const MirBuildError *mir_get_error(const MirProgram *program);
bool mir_format_error(const MirBuildError *error,
                      char *buffer,
                      size_t buffer_size);

bool mir_dump_program(FILE *out, const MirProgram *program);
char *mir_dump_program_to_string(const MirProgram *program);

#endif /* CALYNDA_MIR_H */