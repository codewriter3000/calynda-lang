#ifndef CALYNDA_MACHINE_H
#define CALYNDA_MACHINE_H

#include "codegen.h"
#include "hir.h"
#include "runtime_abi.h"
#include "target.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct MachineInstruction MachineInstruction;
typedef struct MachineBlock MachineBlock;
typedef struct MachineUnit MachineUnit;
typedef struct MachineFrameSlot MachineFrameSlot;
typedef struct MachineStaticArrayElement MachineStaticArrayElement;
typedef struct MachineStaticArrayObject MachineStaticArrayObject;
typedef struct MachineStaticArrayBinding MachineStaticArrayBinding;

typedef enum {
    MACHINE_STATIC_ARRAY_ELEMENT_WORD = 0,
    MACHINE_STATIC_ARRAY_ELEMENT_OBJECT
} MachineStaticArrayElementKind;

struct MachineStaticArrayElement {
    MachineStaticArrayElementKind kind;
    CalyndaRtWord                 word;
    size_t                        object_index;
};

struct MachineStaticArrayObject {
    MachineStaticArrayElement *elements;
    size_t                    element_count;
};

struct MachineStaticArrayBinding {
    char   *global_name;
    size_t  object_index;
};

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
    bool          is_exported;
    bool          is_static;
    size_t        parameter_count;
    MachineFrameSlot *frame_slots;
    size_t        frame_slot_count;
    size_t        spill_slot_count;
    size_t        helper_slot_count;
    size_t        outgoing_stack_slot_count;
    MachineBlock *blocks;
    size_t        block_count;
    /* ASM unit fields */
    char         *asm_body;
    size_t        asm_body_length;
    /* Boot flag for START units */
    bool          is_boot;
};

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} MachineBuildError;

typedef struct {
    CodegenTargetKind        target;
    const TargetDescriptor  *target_desc;
    MachineUnit             *units;
    size_t                   unit_count;
    MachineStaticArrayObject *static_array_objects;
    size_t                    static_array_object_count;
    size_t                    static_array_object_capacity;
    MachineStaticArrayBinding *static_array_bindings;
    size_t                    static_array_binding_count;
    size_t                    static_array_binding_capacity;
    MachineBuildError        error;
    bool                     has_error;
} MachineProgram;

void machine_program_init(MachineProgram *program);
void machine_program_free(MachineProgram *program);

bool machine_build_program(MachineProgram *program,
                          const LirProgram *lir_program,
                          const CodegenProgram *codegen_program,
                          const HirProgram *hir_program);

const MachineBuildError *machine_get_error(const MachineProgram *program);
bool machine_format_error(const MachineBuildError *error,
                          char *buffer,
                          size_t buffer_size);

bool machine_dump_program(FILE *out, const MachineProgram *program);
char *machine_dump_program_to_string(const MachineProgram *program);

bool machine_program_has_boot(const MachineProgram *program);

#endif /* CALYNDA_MACHINE_H */