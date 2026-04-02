#ifndef CALYNDA_HIR_H
#define CALYNDA_HIR_H

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
} HirLambdaExpression;

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
    HIR_EXPR_ARRAY_LITERAL
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
    } as;
};

typedef struct {
    char         *name;
    const Symbol *symbol;
    CheckedType   type;
    bool          is_callable;
    HirCallableSignature callable_signature;
    bool          is_final;
    AstSourceSpan source_span;
    HirExpression *initializer;
} HirLocalBinding;

typedef enum {
    HIR_STMT_LOCAL_BINDING = 0,
    HIR_STMT_RETURN,
    HIR_STMT_EXIT,
    HIR_STMT_THROW,
    HIR_STMT_EXPRESSION
} HirStatementKind;

struct HirStatement {
    HirStatementKind kind;
    AstSourceSpan    source_span;
    union {
        HirLocalBinding local_binding;
        HirExpression  *return_expression;
        HirExpression  *throw_expression;
        HirExpression  *expression;
    } as;
};

struct HirBlock {
    HirStatement **statements;
    size_t        statement_count;
    size_t        statement_capacity;
};

typedef struct {
    char         *name;
    const Symbol *symbol;
    CheckedType   type;
    bool          is_final;
    bool          is_callable;
    HirCallableSignature callable_signature;
    AstSourceSpan source_span;
    HirExpression *initializer;
} HirBindingDecl;

typedef struct {
    HirParameterList parameters;
    HirBlock        *body;
    AstSourceSpan    source_span;
} HirStartDecl;

typedef enum {
    HIR_TOP_LEVEL_BINDING = 0,
    HIR_TOP_LEVEL_START
} HirTopLevelDeclKind;

struct HirTopLevelDecl {
    HirTopLevelDeclKind kind;
    union {
        HirBindingDecl binding;
        HirStartDecl   start;
    } as;
};

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} HirBuildError;

typedef struct {
    bool             has_package;
    HirNamedSymbol   package;
    HirNamedSymbol  *imports;
    size_t           import_count;
    size_t           import_capacity;
    HirTopLevelDecl **top_level_decls;
    size_t           top_level_count;
    size_t           top_level_capacity;
    HirBuildError    error;
    bool             has_error;
} HirProgram;

void hir_program_init(HirProgram *program);
void hir_program_free(HirProgram *program);
void hir_block_free(HirBlock *block);
void hir_statement_free(HirStatement *statement);
void hir_expression_free(HirExpression *expression);

bool hir_build_program(HirProgram *program,
                       const AstProgram *ast_program,
                       const SymbolTable *symbols,
                       const TypeChecker *checker);

const HirBuildError *hir_get_error(const HirProgram *program);
bool hir_format_error(const HirBuildError *error,
                      char *buffer,
                      size_t buffer_size);

#endif /* CALYNDA_HIR_H */