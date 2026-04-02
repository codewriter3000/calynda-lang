#ifndef CALYNDA_MIR_H
#define CALYNDA_MIR_H

#include "hir.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct MirInstruction MirInstruction;
typedef struct MirBasicBlock MirBasicBlock;
typedef struct MirUnit MirUnit;

typedef enum {
    MIR_LOCAL_PARAMETER = 0,
    MIR_LOCAL_LOCAL,
    MIR_LOCAL_CAPTURE,
    MIR_LOCAL_SYNTHETIC
} MirLocalKind;

typedef struct {
    MirLocalKind  kind;
    char         *name;
    const Symbol *symbol;
    CheckedType   type;
    bool          is_final;
    size_t        index;
} MirLocal;

typedef enum {
    MIR_VALUE_INVALID = 0,
    MIR_VALUE_TEMP,
    MIR_VALUE_LOCAL,
    MIR_VALUE_GLOBAL,
    MIR_VALUE_LITERAL
} MirValueKind;

typedef struct {
    MirValueKind kind;
    CheckedType  type;
    union {
        size_t temp_index;
        size_t local_index;
        char  *global_name;
        struct {
            AstLiteralKind kind;
            char          *text;
            bool           bool_value;
        } literal;
    } as;
} MirValue;

typedef enum {
    MIR_TEMPLATE_PART_TEXT = 0,
    MIR_TEMPLATE_PART_VALUE
} MirTemplatePartKind;

typedef struct {
    MirTemplatePartKind kind;
    union {
        char    *text;
        MirValue value;
    } as;
} MirTemplatePart;

typedef enum {
    MIR_INSTR_BINARY = 0,
    MIR_INSTR_UNARY,
    MIR_INSTR_CLOSURE,
    MIR_INSTR_CALL,
    MIR_INSTR_CAST,
    MIR_INSTR_MEMBER,
    MIR_INSTR_INDEX_LOAD,
    MIR_INSTR_ARRAY_LITERAL,
    MIR_INSTR_TEMPLATE,
    MIR_INSTR_STORE_LOCAL,
    MIR_INSTR_STORE_GLOBAL,
    MIR_INSTR_STORE_INDEX,
    MIR_INSTR_STORE_MEMBER
} MirInstructionKind;

struct MirInstruction {
    MirInstructionKind kind;
    union {
        struct {
            size_t            dest_temp;
            AstBinaryOperator operator;
            MirValue          left;
            MirValue          right;
        } binary;
        struct {
            size_t           dest_temp;
            AstUnaryOperator operator;
            MirValue         operand;
        } unary;
        struct {
            size_t    dest_temp;
            char     *unit_name;
            MirValue *captures;
            size_t    capture_count;
        } closure;
        struct {
            bool      has_result;
            size_t    dest_temp;
            MirValue  callee;
            MirValue *arguments;
            size_t    argument_count;
        } call;
        struct {
            size_t      dest_temp;
            CheckedType target_type;
            MirValue    operand;
        } cast;
        struct {
            size_t   dest_temp;
            MirValue target;
            char    *member;
        } member;
        struct {
            size_t   dest_temp;
            MirValue target;
            MirValue index;
        } index_load;
        struct {
            size_t    dest_temp;
            MirValue *elements;
            size_t    element_count;
        } array_literal;
        struct {
            size_t           dest_temp;
            MirTemplatePart *parts;
            size_t           part_count;
        } template_literal;
        struct {
            size_t   local_index;
            MirValue value;
        } store_local;
        struct {
            char    *global_name;
            MirValue value;
        } store_global;
        struct {
            MirValue target;
            MirValue index;
            MirValue value;
        } store_index;
        struct {
            MirValue target;
            char    *member;
            MirValue value;
        } store_member;
    } as;
};

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
    MIR_UNIT_LAMBDA
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