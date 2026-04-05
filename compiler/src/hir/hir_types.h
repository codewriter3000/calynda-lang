#ifndef CALYNDA_HIR_TYPES_H
#define CALYNDA_HIR_TYPES_H

#include "hir_expr_types.h"

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
    bool          is_exported;
    bool          is_static;
    bool          is_internal;
    bool          is_callable;
    HirCallableSignature callable_signature;
    AstSourceSpan source_span;
    HirExpression *initializer;
} HirBindingDecl;

typedef struct {
    HirParameterList parameters;
    HirBlock        *body;
    AstSourceSpan    source_span;
    bool             is_boot;
} HirStartDecl;

typedef struct {
    char         *name;
    CheckedType   payload_type;
    bool          has_payload;
} HirUnionVariant;

typedef struct {
    char             *name;
    const Symbol     *symbol;
    AstSourceSpan     source_span;
    char            **generic_params;
    size_t            generic_param_count;
    HirUnionVariant  *variants;
    size_t            variant_count;
    bool              is_exported;
    bool              is_static;
} HirUnionDecl;

typedef struct {
    char         *name;
    const Symbol *symbol;
    AstSourceSpan source_span;
    bool          is_exported;
    bool          is_static;
    bool          is_internal;
    CheckedType   return_type;
    size_t        parameter_count;
    CheckedType  *parameter_types;
    char        **parameter_names;
    char         *body;
    size_t        body_length;
} HirAsmDecl;

typedef enum {
    HIR_TOP_LEVEL_BINDING = 0,
    HIR_TOP_LEVEL_START,
    HIR_TOP_LEVEL_UNION,
    HIR_TOP_LEVEL_ASM
} HirTopLevelDeclKind;

struct HirTopLevelDecl {
    HirTopLevelDeclKind kind;
    union {
        HirBindingDecl binding;
        HirStartDecl   start;
        HirUnionDecl   union_decl;
        HirAsmDecl     asm_decl;
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

#endif /* CALYNDA_HIR_TYPES_H */
