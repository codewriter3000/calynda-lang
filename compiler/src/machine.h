#ifndef CALYNDA_MACHINE_H
#define CALYNDA_MACHINE_H

#include "codegen.h"
#include "runtime_abi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct MachineInstruction MachineInstruction;
typedef struct MachineBlock MachineBlock;
typedef struct MachineUnit MachineUnit;
typedef struct MachineFrameSlot MachineFrameSlot;

struct MachineFrameSlot {
    size_t       index;
    LirSlotKind  kind;
    char        *name;
    CheckedType  type;
    bool         is_final;
};

struct MachineInstruction {
    char *text;
};

struct MachineBlock {
    char               *label;
    MachineInstruction *instructions;
    size_t              instruction_count;
    size_t              instruction_capacity;
};

struct MachineUnit {
    LirUnitKind   kind;
    char         *name;
    CheckedType   return_type;
    size_t        parameter_count;
    MachineFrameSlot *frame_slots;
    size_t        frame_slot_count;
    size_t        spill_slot_count;
    size_t        helper_slot_count;
    size_t        outgoing_stack_slot_count;
    MachineBlock *blocks;
    size_t        block_count;
};

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} MachineBuildError;

typedef struct {
    CodegenTargetKind target;
    MachineUnit      *units;
    size_t            unit_count;
    MachineBuildError error;
    bool              has_error;
} MachineProgram;

void machine_program_init(MachineProgram *program);
void machine_program_free(MachineProgram *program);

bool machine_build_program(MachineProgram *program,
                          const LirProgram *lir_program,
                          const CodegenProgram *codegen_program);

const MachineBuildError *machine_get_error(const MachineProgram *program);
bool machine_format_error(const MachineBuildError *error,
                          char *buffer,
                          size_t buffer_size);

bool machine_dump_program(FILE *out, const MachineProgram *program);
char *machine_dump_program_to_string(const MachineProgram *program);

#endif /* CALYNDA_MACHINE_H */