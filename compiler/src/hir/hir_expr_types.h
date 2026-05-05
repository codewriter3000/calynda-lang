#ifndef CALYNDA_HIR_EXPR_TYPES_H
#define CALYNDA_HIR_EXPR_TYPES_H

#include "ast.h"
#include "symbol_table.h"
#include "type_checker.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct HirExpression HirExpression;
typedef struct HirStatement HirStatement;
typedef struct HirTopLevelDecl HirTopLevelDecl;
typedef struct HirBlock HirBlock;

typedef struct {
    const Symbol *symbol;
    char         *name;
    char         *qualified_name;
    AstSourceSpan source_span;
} HirNamedSymbol;

typedef struct {
    const Symbol *symbol;
    char         *name;
    SymbolKind    kind;
    CheckedType   type;
    AstSourceSpan source_span;
} HirSymbolRef;

typedef struct {
    char         *name;
    const Symbol *symbol;
    CheckedType   type;
    bool          is_varargs;
    bool          is_block;       /* true for |var non-local return block parameters */
    AstSourceSpan source_span;
} HirParameter;

typedef struct {
    HirParameter *items;
    size_t        count;
    size_t        capacity;
} HirParameterList;

typedef struct {
    CheckedType *parameter_types;
    size_t       parameter_count;
    bool         has_parameter_types;
    CheckedType  return_type;
} HirCallableSignature;

typedef struct {
    AstLiteralKind kind;
    union {
        char *text;
        bool  bool_value;
    } as;
} HirLiteral;

typedef struct {
    AstTemplatePartKind kind;
    union {
        char          *text;
        HirExpression *expression;
    } as;
} HirTemplatePart;

typedef struct {
    HirTemplatePart *items;
    size_t           count;
    size_t           capacity;
} HirTemplatePartList;

typedef struct {
    AstAssignmentOperator operator;
    HirExpression        *target;
    HirExpression        *value;
} HirAssignmentExpression;

typedef struct {
    AstBinaryOperator operator;
    HirExpression    *left;
    HirExpression    *right;
} HirBinaryExpression;

typedef struct {
    AstUnaryOperator operator;
    HirExpression   *operand;
} HirUnaryExpression;

typedef struct {
    HirExpression *condition;
    HirExpression *then_branch;
    HirExpression *else_branch;
} HirTernaryExpression;

typedef struct {
    HirExpression  *callee;
    HirExpression **arguments;
    size_t          argument_count;
    size_t          argument_capacity;
} HirCallExpression;

typedef struct {
    HirExpression *target;
    HirExpression *index;
} HirIndexExpression;

typedef struct {
    HirExpression *target;
    char          *member;
} HirMemberExpression;

typedef struct {
    CheckedType    target_type;
    HirExpression *expression;
} HirCastExpression;

typedef struct {
    HirExpression **elements;
    size_t         element_count;
    size_t         element_capacity;
} HirArrayLiteralExpression;

typedef struct {
    HirParameterList parameters;
    HirBlock        *body;
    bool             is_nlr_block; /* lambda is passed to a |var parameter */
} HirLambdaExpression;

typedef struct {
    HirExpression *operand;
} HirPostIncrementExpression;

typedef struct {
    HirExpression *operand;
} HirPostDecrementExpression;

typedef enum {
    HIR_MEMORY_MALLOC = 0,
    HIR_MEMORY_CALLOC,
    HIR_MEMORY_REALLOC,
    HIR_MEMORY_FREE,
    HIR_MEMORY_DEREF,
    HIR_MEMORY_ADDR,
    HIR_MEMORY_OFFSET,
    HIR_MEMORY_STORE,
    HIR_MEMORY_CLEANUP,
    HIR_MEMORY_STACKALLOC
} HirMemoryOpKind;

typedef struct {
    HirMemoryOpKind  kind;
    HirExpression  **arguments;
    size_t           argument_count;
    size_t           element_size; /* sizeof(T) for ptr<T>; 0 = word-size (untyped) */
    bool             is_checked_ptr; /* true when first arg is ptr<T, checked> */
} HirMemoryOpExpression;

typedef enum {
    HIR_EXPR_LITERAL = 0,
    HIR_EXPR_TEMPLATE,
    HIR_EXPR_SYMBOL,
    HIR_EXPR_LAMBDA,
    HIR_EXPR_ASSIGNMENT,
    HIR_EXPR_TERNARY,
    HIR_EXPR_BINARY,
    HIR_EXPR_UNARY,
    HIR_EXPR_CALL,
    HIR_EXPR_INDEX,
    HIR_EXPR_MEMBER,
    HIR_EXPR_CAST,
    HIR_EXPR_ARRAY_LITERAL,
    HIR_EXPR_DISCARD,
    HIR_EXPR_POST_INCREMENT,
    HIR_EXPR_POST_DECREMENT,
    HIR_EXPR_MEMORY_OP,
    HIR_EXPR_NONLOCAL_RETURN  /* |var non-local return expression */
} HirExpressionKind;

struct HirExpression {
    HirExpressionKind kind;
    CheckedType       type;
    bool              is_callable;
    HirCallableSignature callable_signature;
    AstSourceSpan     source_span;
    union {
        HirLiteral                literal;
        HirTemplatePartList       template_parts;
        HirSymbolRef              symbol;
        HirLambdaExpression       lambda;
        HirAssignmentExpression   assignment;
        HirTernaryExpression      ternary;
        HirBinaryExpression       binary;
        HirUnaryExpression        unary;
        HirCallExpression         call;
        HirIndexExpression        index;
        HirMemberExpression       member;
        HirCastExpression         cast;
        HirArrayLiteralExpression array_literal;
        HirPostIncrementExpression  post_increment;
        HirPostDecrementExpression  post_decrement;
        HirMemoryOpExpression       memory_op;
        HirExpression              *nonlocal_return_value; /* NULL for bare return */
    } as;
};

#endif /* CALYNDA_HIR_EXPR_TYPES_H */
