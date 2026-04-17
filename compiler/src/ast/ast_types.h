#ifndef CALYNDA_AST_TYPES_H
#define CALYNDA_AST_TYPES_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    AST_MODIFIER_PUBLIC = 0,
    AST_MODIFIER_PRIVATE,
    AST_MODIFIER_FINAL,
    AST_MODIFIER_EXPORT,
    AST_MODIFIER_STATIC,
    AST_MODIFIER_INTERNAL,
    AST_MODIFIER_THREAD_LOCAL
} AstModifier;

typedef enum {
    AST_PRIMITIVE_INT8 = 0,
    AST_PRIMITIVE_INT16,
    AST_PRIMITIVE_INT32,
    AST_PRIMITIVE_INT64,
    AST_PRIMITIVE_UINT8,
    AST_PRIMITIVE_UINT16,
    AST_PRIMITIVE_UINT32,
    AST_PRIMITIVE_UINT64,
    AST_PRIMITIVE_FLOAT32,
    AST_PRIMITIVE_FLOAT64,
    AST_PRIMITIVE_BOOL,
    AST_PRIMITIVE_CHAR,
    AST_PRIMITIVE_STRING,
    AST_PRIMITIVE_BYTE,
    AST_PRIMITIVE_SBYTE,
    AST_PRIMITIVE_SHORT,
    AST_PRIMITIVE_INT,
    AST_PRIMITIVE_LONG,
    AST_PRIMITIVE_ULONG,
    AST_PRIMITIVE_UINT,
    AST_PRIMITIVE_FLOAT,
    AST_PRIMITIVE_DOUBLE
} AstPrimitiveType;

typedef enum {
    AST_TYPE_VOID = 0,
    AST_TYPE_PRIMITIVE,
    AST_TYPE_ARR,
    AST_TYPE_PTR,
    AST_TYPE_NAMED,
    AST_TYPE_THREAD,
    AST_TYPE_MUTEX,
    AST_TYPE_FUTURE,
    AST_TYPE_ATOMIC
} AstTypeKind;

typedef struct {
    bool  has_size;
    char *size_literal;
} AstArrayDimension;

typedef struct AstType AstType;

typedef enum {
    AST_GENERIC_ARG_WILDCARD = 0,
    AST_GENERIC_ARG_TYPE
} AstGenericArgKind;

typedef struct AstGenericArg AstGenericArg;

struct AstGenericArg {
    AstGenericArgKind kind;
    AstType          *type;   /* NULL for wildcard */
};

typedef struct {
    AstGenericArg *items;
    size_t         count;
    size_t         capacity;
} AstGenericArgList;

struct AstType {
    AstTypeKind       kind;
    AstPrimitiveType  primitive;
    char             *name;              /* for AST_TYPE_NAMED */
    AstArrayDimension *dimensions;
    size_t            dimension_count;
    size_t            dimension_capacity;
    AstGenericArgList generic_args;
};

typedef struct {
    int start_line;
    int start_column;
    int end_line;
    int end_column;
} AstSourceSpan;

typedef struct {
    char         **segments;
    size_t         count;
    size_t         capacity;
    AstSourceSpan  source_span;
    AstSourceSpan  tail_span;
} AstQualifiedName;

typedef struct AstExpression AstExpression;
typedef struct AstStatement AstStatement;
typedef struct AstTopLevelDecl AstTopLevelDecl;
typedef struct AstBlock AstBlock;

typedef struct {
    AstType       type;
    char         *name;
    AstSourceSpan name_span;
    AstExpression *default_expr;
    bool          is_varargs;
} AstParameter;

typedef struct {
    AstParameter *items;
    size_t       count;
    size_t       capacity;
} AstParameterList;

typedef struct {
    AstExpression **items;
    size_t        count;
    size_t        capacity;
} AstExpressionList;

typedef enum {
    AST_TEMPLATE_PART_TEXT = 0,
    AST_TEMPLATE_PART_EXPRESSION
} AstTemplatePartKind;

typedef struct {
    AstTemplatePartKind kind;
    union {
        char          *text;
        AstExpression *expression;
    } as;
} AstTemplatePart;

typedef struct {
    AstTemplatePart *items;
    size_t          count;
    size_t          capacity;
} AstTemplatePartList;

typedef enum {
    AST_LITERAL_INTEGER = 0,
    AST_LITERAL_FLOAT,
    AST_LITERAL_BOOL,
    AST_LITERAL_CHAR,
    AST_LITERAL_STRING,
    AST_LITERAL_TEMPLATE,
    AST_LITERAL_NULL
} AstLiteralKind;

typedef struct {
    AstLiteralKind kind;
    union {
        char                *text;
        bool                bool_value;
        AstTemplatePartList template_parts;
    } as;
} AstLiteral;

typedef enum {
    AST_ASSIGN_OP_ASSIGN = 0,
    AST_ASSIGN_OP_ADD,
    AST_ASSIGN_OP_SUBTRACT,
    AST_ASSIGN_OP_MULTIPLY,
    AST_ASSIGN_OP_DIVIDE,
    AST_ASSIGN_OP_MODULO,
    AST_ASSIGN_OP_BIT_AND,
    AST_ASSIGN_OP_BIT_OR,
    AST_ASSIGN_OP_BIT_XOR,
    AST_ASSIGN_OP_SHIFT_LEFT,
    AST_ASSIGN_OP_SHIFT_RIGHT
} AstAssignmentOperator;

typedef enum {
    AST_BINARY_OP_LOGICAL_OR = 0,
    AST_BINARY_OP_LOGICAL_AND,
    AST_BINARY_OP_BIT_OR,
    AST_BINARY_OP_BIT_NAND,
    AST_BINARY_OP_BIT_XOR,
    AST_BINARY_OP_BIT_XNOR,
    AST_BINARY_OP_BIT_AND,
    AST_BINARY_OP_EQUAL,
    AST_BINARY_OP_NOT_EQUAL,
    AST_BINARY_OP_LESS,
    AST_BINARY_OP_GREATER,
    AST_BINARY_OP_LESS_EQUAL,
    AST_BINARY_OP_GREATER_EQUAL,
    AST_BINARY_OP_SHIFT_LEFT,
    AST_BINARY_OP_SHIFT_RIGHT,
    AST_BINARY_OP_ADD,
    AST_BINARY_OP_SUBTRACT,
    AST_BINARY_OP_MULTIPLY,
    AST_BINARY_OP_DIVIDE,
    AST_BINARY_OP_MODULO
} AstBinaryOperator;

typedef enum {
    AST_UNARY_OP_LOGICAL_NOT = 0,
    AST_UNARY_OP_BITWISE_NOT,
    AST_UNARY_OP_NEGATE,
    AST_UNARY_OP_PLUS,
    AST_UNARY_OP_PRE_INCREMENT,
    AST_UNARY_OP_PRE_DECREMENT,
    AST_UNARY_OP_DEREF,
    AST_UNARY_OP_ADDRESS_OF
} AstUnaryOperator;

typedef enum {
    AST_LAMBDA_BODY_BLOCK = 0,
    AST_LAMBDA_BODY_EXPRESSION
} AstLambdaBodyKind;

typedef struct {
    AstLambdaBodyKind kind;
    union {
        AstBlock      *block;
        AstExpression *expression;
    } as;
} AstLambdaBody;

typedef struct {
    AstParameterList parameters;
    AstLambdaBody    body;
} AstLambdaExpression;

typedef struct {
    AstAssignmentOperator operator;
    AstExpression        *target;
    AstExpression        *value;
} AstAssignmentExpression;

typedef struct {
    AstExpression *condition;
    AstExpression *then_branch;
    AstExpression *else_branch;
} AstTernaryExpression;

typedef struct {
    AstBinaryOperator operator;
    AstExpression    *left;
    AstExpression    *right;
} AstBinaryExpression;

typedef struct {
    AstUnaryOperator operator;
    AstExpression   *operand;
} AstUnaryExpression;

#endif /* CALYNDA_AST_TYPES_H */
