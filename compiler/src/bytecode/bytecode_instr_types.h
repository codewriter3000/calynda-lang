#ifndef CALYNDA_BYTECODE_INSTR_TYPES_H
#define CALYNDA_BYTECODE_INSTR_TYPES_H

#include "mir.h"
#include "runtime.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
    BYTECODE_CONSTANT_TEXT,
    BYTECODE_CONSTANT_TYPE_DESCRIPTOR
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
        struct {
            char            *name;
            size_t           generic_param_count;
            CalyndaRtTypeTag *generic_param_tags;
            size_t           variant_count;
            char           **variant_names;
            CalyndaRtTypeTag *variant_payload_tags;
        } type_descriptor;
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
    BYTECODE_INSTR_STORE_MEMBER,
    BYTECODE_INSTR_UNION_NEW,
    BYTECODE_INSTR_UNION_GET_TAG,
    BYTECODE_INSTR_UNION_GET_PAYLOAD,
    BYTECODE_INSTR_HETERO_ARRAY_NEW,
    BYTECODE_INSTR_HETERO_ARRAY_GET_TAG,
    BYTECODE_INSTR_TYPEOF,
    BYTECODE_INSTR_ISINT,
    BYTECODE_INSTR_ISFLOAT,
    BYTECODE_INSTR_ISBOOL,
    BYTECODE_INSTR_ISSTRING,
    BYTECODE_INSTR_ISARRAY,
    BYTECODE_INSTR_ISSAMETYPE
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
        struct {
            size_t         dest_temp;
            size_t         type_desc_index;
            uint32_t       variant_tag;
            BytecodeValue  payload;
        } union_new;
        struct {
            size_t        dest_temp;
            BytecodeValue target;
        } union_get_tag;
        struct {
            size_t        dest_temp;
            BytecodeValue target;
        } union_get_payload;
        struct {
            size_t             dest_temp;
            size_t             type_desc_index;
            BytecodeValue     *elements;
            size_t             element_count;
        } hetero_array_new;
        struct {
            size_t        dest_temp;
            BytecodeValue target;
            BytecodeValue index;
        } hetero_array_get_tag;
        struct {
            size_t        dest_temp;
            BytecodeValue operand;
            BytecodeValue type_text;
        } builtin_type_query;
        struct {
            size_t        dest_temp;
            BytecodeValue left;
            BytecodeValue left_type_text;
            BytecodeValue right;
            BytecodeValue right_type_text;
        } same_type_query;
    } as;
};

#endif /* CALYNDA_BYTECODE_INSTR_TYPES_H */
