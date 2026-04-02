#ifndef CALYNDA_BYTECODE_H
#define CALYNDA_BYTECODE_H

#include "mir.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct BytecodeInstruction BytecodeInstruction;
typedef struct BytecodeBasicBlock BytecodeBasicBlock;
typedef struct BytecodeUnit BytecodeUnit;

typedef enum {
    BYTECODE_TARGET_PORTABLE_V1 = 0
} BytecodeTargetKind;

typedef enum {
    BYTECODE_LOCAL_PARAMETER = 0,
    BYTECODE_LOCAL_LOCAL,
    BYTECODE_LOCAL_CAPTURE,
    BYTECODE_LOCAL_SYNTHETIC
} BytecodeLocalKind;

typedef struct {
    BytecodeLocalKind kind;
    char             *name;
    const Symbol     *symbol;
    CheckedType       type;
    bool              is_final;
    size_t            index;
} BytecodeLocal;

typedef enum {
    BYTECODE_CONSTANT_LITERAL = 0,
    BYTECODE_CONSTANT_SYMBOL,
    BYTECODE_CONSTANT_TEXT
} BytecodeConstantKind;

typedef struct {
    BytecodeConstantKind kind;
    union {
        struct {
            AstLiteralKind kind;
            char          *text;
            bool           bool_value;
        } literal;
        char *text;
    } as;
} BytecodeConstant;

typedef enum {
    BYTECODE_VALUE_INVALID = 0,
    BYTECODE_VALUE_TEMP,
    BYTECODE_VALUE_LOCAL,
    BYTECODE_VALUE_GLOBAL,
    BYTECODE_VALUE_CONSTANT
} BytecodeValueKind;

typedef struct {
    BytecodeValueKind kind;
    CheckedType       type;
    union {
        size_t temp_index;
        size_t local_index;
        size_t global_index;
        size_t constant_index;
    } as;
} BytecodeValue;

typedef enum {
    BYTECODE_TEMPLATE_PART_TEXT = 0,
    BYTECODE_TEMPLATE_PART_VALUE
} BytecodeTemplatePartKind;

typedef struct {
    BytecodeTemplatePartKind kind;
    union {
        size_t        text_index;
        BytecodeValue value;
    } as;
} BytecodeTemplatePart;

typedef enum {
    BYTECODE_INSTR_BINARY = 0,
    BYTECODE_INSTR_UNARY,
    BYTECODE_INSTR_CLOSURE,
    BYTECODE_INSTR_CALL,
    BYTECODE_INSTR_CAST,
    BYTECODE_INSTR_MEMBER,
    BYTECODE_INSTR_INDEX_LOAD,
    BYTECODE_INSTR_ARRAY_LITERAL,
    BYTECODE_INSTR_TEMPLATE,
    BYTECODE_INSTR_STORE_LOCAL,
    BYTECODE_INSTR_STORE_GLOBAL,
    BYTECODE_INSTR_STORE_INDEX,
    BYTECODE_INSTR_STORE_MEMBER
} BytecodeInstructionKind;

struct BytecodeInstruction {
    BytecodeInstructionKind kind;
    union {
        struct {
            size_t        dest_temp;
            AstBinaryOperator operator;
            BytecodeValue left;
            BytecodeValue right;
        } binary;
        struct {
            size_t          dest_temp;
            AstUnaryOperator operator;
            BytecodeValue   operand;
        } unary;
        struct {
            size_t         dest_temp;
            size_t         unit_index;
            BytecodeValue *captures;
            size_t         capture_count;
        } closure;
        struct {
            bool           has_result;
            size_t         dest_temp;
            BytecodeValue  callee;
            BytecodeValue *arguments;
            size_t         argument_count;
        } call;
        struct {
            size_t       dest_temp;
            CheckedType  target_type;
            BytecodeValue operand;
        } cast;
        struct {
            size_t       dest_temp;
            BytecodeValue target;
            size_t       member_index;
        } member;
        struct {
            size_t       dest_temp;
            BytecodeValue target;
            BytecodeValue index;
        } index_load;
        struct {
            size_t         dest_temp;
            BytecodeValue *elements;
            size_t         element_count;
        } array_literal;
        struct {
            size_t               dest_temp;
            BytecodeTemplatePart *parts;
            size_t               part_count;
        } template_literal;
        struct {
            size_t       local_index;
            BytecodeValue value;
        } store_local;
        struct {
            size_t       global_index;
            BytecodeValue value;
        } store_global;
        struct {
            BytecodeValue target;
            BytecodeValue index;
            BytecodeValue value;
        } store_index;
        struct {
            BytecodeValue target;
            size_t       member_index;
            BytecodeValue value;
        } store_member;
    } as;
};

typedef enum {
    BYTECODE_TERM_NONE = 0,
    BYTECODE_TERM_RETURN,
    BYTECODE_TERM_JUMP,
    BYTECODE_TERM_BRANCH,
    BYTECODE_TERM_THROW
} BytecodeTerminatorKind;

typedef struct {
    BytecodeTerminatorKind kind;
    union {
        struct {
            bool          has_value;
            BytecodeValue value;
        } return_term;
        struct {
            size_t target_block;
        } jump_term;
        struct {
            BytecodeValue condition;
            size_t        true_block;
            size_t        false_block;
        } branch_term;
        struct {
            BytecodeValue value;
        } throw_term;
    } as;
} BytecodeTerminator;

struct BytecodeBasicBlock {
    char                *label;
    BytecodeInstruction *instructions;
    size_t               instruction_count;
    BytecodeTerminator   terminator;
};

typedef enum {
    BYTECODE_UNIT_START = 0,
    BYTECODE_UNIT_BINDING,
    BYTECODE_UNIT_INIT,
    BYTECODE_UNIT_LAMBDA
} BytecodeUnitKind;

struct BytecodeUnit {
    BytecodeUnitKind   kind;
    char              *name;
    const Symbol      *symbol;
    CheckedType        return_type;
    BytecodeLocal     *locals;
    size_t             local_count;
    size_t             parameter_count;
    size_t             temp_count;
    BytecodeBasicBlock *blocks;
    size_t             block_count;
};

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} BytecodeBuildError;

typedef struct {
    BytecodeTargetKind target;
    BytecodeConstant  *constants;
    size_t             constant_count;
    size_t             constant_capacity;
    BytecodeUnit      *units;
    size_t             unit_count;
    BytecodeBuildError error;
    bool               has_error;
} BytecodeProgram;

void bytecode_program_init(BytecodeProgram *program);
void bytecode_program_free(BytecodeProgram *program);

bool bytecode_build_program(BytecodeProgram *program, const MirProgram *mir_program);

const BytecodeBuildError *bytecode_get_error(const BytecodeProgram *program);
bool bytecode_format_error(const BytecodeBuildError *error,
                           char *buffer,
                           size_t buffer_size);
const char *bytecode_target_name(BytecodeTargetKind target);

bool bytecode_dump_program(FILE *out, const BytecodeProgram *program);
char *bytecode_dump_program_to_string(const BytecodeProgram *program);

#endif /* CALYNDA_BYTECODE_H */