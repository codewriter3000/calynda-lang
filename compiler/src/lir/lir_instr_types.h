#ifndef CALYNDA_LIR_INSTR_TYPES_H
#define CALYNDA_LIR_INSTR_TYPES_H

#include "mir.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct LirInstruction LirInstruction;
typedef struct LirBasicBlock LirBasicBlock;
typedef struct LirUnit LirUnit;

typedef enum {
    LIR_SLOT_PARAMETER = 0,
    LIR_SLOT_LOCAL,
    LIR_SLOT_CAPTURE,
    LIR_SLOT_SYNTHETIC
} LirSlotKind;

typedef struct {
    LirSlotKind   kind;
    char         *name;
    const Symbol *symbol;
    CheckedType   type;
    bool          is_final;
    size_t        index;
} LirSlot;

typedef enum {
    LIR_OPERAND_INVALID = 0,
    LIR_OPERAND_VREG,
    LIR_OPERAND_SLOT,
    LIR_OPERAND_GLOBAL,
    LIR_OPERAND_LITERAL
} LirOperandKind;

typedef struct {
    LirOperandKind kind;
    CheckedType    type;
    union {
        size_t vreg_index;
        size_t slot_index;
        char  *global_name;
        struct {
            AstLiteralKind kind;
            char          *text;
            bool           bool_value;
        } literal;
    } as;
} LirOperand;

typedef enum {
    LIR_TEMPLATE_PART_TEXT = 0,
    LIR_TEMPLATE_PART_VALUE
} LirTemplatePartKind;

typedef struct {
    LirTemplatePartKind kind;
    union {
        char      *text;
        LirOperand value;
    } as;
} LirTemplatePart;

typedef enum {
    LIR_INSTR_INCOMING_ARG = 0,
    LIR_INSTR_INCOMING_CAPTURE,
    LIR_INSTR_OUTGOING_ARG,
    LIR_INSTR_BINARY,
    LIR_INSTR_UNARY,
    LIR_INSTR_CLOSURE,
    LIR_INSTR_CALL,
    LIR_INSTR_CAST,
    LIR_INSTR_MEMBER,
    LIR_INSTR_INDEX_LOAD,
    LIR_INSTR_ARRAY_LITERAL,
    LIR_INSTR_TEMPLATE,
    LIR_INSTR_STORE_SLOT,
    LIR_INSTR_STORE_GLOBAL,
    LIR_INSTR_STORE_INDEX,
    LIR_INSTR_STORE_MEMBER,
    LIR_INSTR_UNION_NEW,
    LIR_INSTR_UNION_GET_TAG,
    LIR_INSTR_UNION_GET_PAYLOAD,
    LIR_INSTR_HETERO_ARRAY_NEW
} LirInstructionKind;

struct LirInstruction {
    LirInstructionKind kind;
    union {
        struct {
            size_t slot_index;
            size_t argument_index;
        } incoming_arg;
        struct {
            size_t slot_index;
            size_t capture_index;
        } incoming_capture;
        struct {
            size_t     argument_index;
            LirOperand value;
        } outgoing_arg;
        struct {
            size_t            dest_vreg;
            AstBinaryOperator operator;
            LirOperand        left;
            LirOperand        right;
        } binary;
        struct {
            size_t           dest_vreg;
            AstUnaryOperator operator;
            LirOperand       operand;
        } unary;
        struct {
            size_t      dest_vreg;
            char       *unit_name;
            LirOperand *captures;
            size_t      capture_count;
        } closure;
        struct {
            bool       has_result;
            size_t     dest_vreg;
            LirOperand callee;
            size_t     argument_count;
        } call;
        struct {
            size_t      dest_vreg;
            CheckedType target_type;
            LirOperand  operand;
        } cast;
        struct {
            size_t     dest_vreg;
            LirOperand target;
            char      *member;
        } member;
        struct {
            size_t     dest_vreg;
            LirOperand target;
            LirOperand index;
        } index_load;
        struct {
            size_t     dest_vreg;
            LirOperand *elements;
            size_t     element_count;
        } array_literal;
        struct {
            size_t           dest_vreg;
            LirTemplatePart *parts;
            size_t           part_count;
        } template_literal;
        struct {
            size_t     slot_index;
            LirOperand value;
        } store_slot;
        struct {
            char      *global_name;
            LirOperand value;
        } store_global;
        struct {
            LirOperand target;
            LirOperand index;
            LirOperand value;
        } store_index;
        struct {
            LirOperand target;
            char      *member;
            LirOperand value;
        } store_member;
        struct {
            size_t                 dest_vreg;
            CalyndaRtTypeDescriptor type_desc;
            LirOperand            *elements;
            size_t                 element_count;
        } hetero_array_new;
        struct {
            size_t                 dest_vreg;
            CalyndaRtTypeDescriptor type_desc;
            size_t                 variant_index;
            bool                   has_payload;
            LirOperand             payload;
        } union_new;
        struct {
            size_t     dest_vreg;
            LirOperand target;
        } union_get_tag;
        struct {
            size_t     dest_vreg;
            LirOperand target;
        } union_get_payload;
    } as;
};

#endif /* CALYNDA_LIR_INSTR_TYPES_H */
