#ifndef CALYNDA_MIR_INSTR_TYPES_H
#define CALYNDA_MIR_INSTR_TYPES_H

#include "hir.h"
#include "runtime.h"

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
    bool          is_cell;  /* local holds a heap cell ref; reads/writes go through cell helpers */
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
    MIR_INSTR_STORE_MEMBER,
    MIR_INSTR_UNION_NEW,
    MIR_INSTR_UNION_GET_TAG,
    MIR_INSTR_UNION_GET_PAYLOAD,
    MIR_INSTR_HETERO_ARRAY_NEW
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
        struct {
            size_t                 dest_temp;
            CalyndaRtTypeDescriptor type_desc;
            MirValue              *elements;
            size_t                 element_count;
        } hetero_array_new;
        struct {
            size_t                 dest_temp;
            CalyndaRtTypeDescriptor type_desc;
            size_t                 variant_index;
            bool                   has_payload;
            MirValue               payload;
        } union_new;
        struct {
            size_t   dest_temp;
            MirValue target;
        } union_get_tag;
        struct {
            size_t   dest_temp;
            MirValue target;
        } union_get_payload;
    } as;
};

#endif /* CALYNDA_MIR_INSTR_TYPES_H */
