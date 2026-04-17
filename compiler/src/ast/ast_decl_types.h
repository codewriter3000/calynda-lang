#ifndef CALYNDA_AST_DECL_TYPES_H
#define CALYNDA_AST_DECL_TYPES_H

#include "ast_types.h"

typedef struct {
    AstExpression     *callee;
    AstExpressionList arguments;
} AstCallExpression;

typedef struct {
    AstExpression *target;
    AstExpression *index;
} AstIndexExpression;

typedef struct {
    AstExpression *target;
    char          *member;
} AstMemberExpression;

typedef struct {
    AstPrimitiveType target_type;
    AstExpression   *expression;
} AstCastExpression;

typedef struct {
    AstExpressionList elements;
} AstArrayLiteralExpression;

typedef struct {
    AstExpression *inner;
} AstGroupingExpression;

typedef struct {
    AstExpression *operand;
} AstPostIncrementExpression;

typedef struct {
    AstExpression *operand;
} AstPostDecrementExpression;

typedef struct {
    AstExpression *callable;
} AstSpawnExpression;

typedef enum {
    AST_MEMORY_MALLOC = 0,
    AST_MEMORY_CALLOC,
    AST_MEMORY_REALLOC,
    AST_MEMORY_FREE,
    AST_MEMORY_DEREF,
    AST_MEMORY_ADDR,
    AST_MEMORY_OFFSET,
    AST_MEMORY_STORE,
    AST_MEMORY_CLEANUP,
    AST_MEMORY_STACKALLOC
} AstMemoryOpKind;

typedef struct {
    AstMemoryOpKind    kind;
    AstExpressionList  arguments;
} AstMemoryOpExpression;

typedef enum {
    AST_EXPR_LITERAL = 0,
    AST_EXPR_IDENTIFIER,
    AST_EXPR_LAMBDA,
    AST_EXPR_ASSIGNMENT,
    AST_EXPR_TERNARY,
    AST_EXPR_BINARY,
    AST_EXPR_UNARY,
    AST_EXPR_CALL,
    AST_EXPR_INDEX,
    AST_EXPR_MEMBER,
    AST_EXPR_CAST,
    AST_EXPR_ARRAY_LITERAL,
    AST_EXPR_GROUPING,
    AST_EXPR_DISCARD,
    AST_EXPR_POST_INCREMENT,
    AST_EXPR_POST_DECREMENT,
    AST_EXPR_MEMORY_OP,
    AST_EXPR_SPAWN
} AstExpressionKind;

struct AstExpression {
    AstExpressionKind kind;
    AstSourceSpan     source_span;
    union {
        AstLiteral                literal;
        char                     *identifier;
        AstLambdaExpression       lambda;
        AstAssignmentExpression   assignment;
        AstTernaryExpression      ternary;
        AstBinaryExpression       binary;
        AstUnaryExpression        unary;
        AstCallExpression         call;
        AstIndexExpression        index;
        AstMemberExpression       member;
        AstCastExpression         cast;
        AstArrayLiteralExpression array_literal;
        AstGroupingExpression     grouping;
        AstPostIncrementExpression post_increment;
        AstPostDecrementExpression post_decrement;
        AstMemoryOpExpression     memory_op;
        AstSpawnExpression        spawn;
    } as;
};

typedef struct {
    bool          is_final;
    bool          is_internal;
    bool          is_inferred_type;
    AstType       declared_type;
    char          *name;
    AstSourceSpan name_span;
    AstExpression *initializer;
} AstLocalBindingStatement;

typedef struct {
    AstParameterList parameters;
    AstBlock        *body;
    bool             is_checked;
} AstManualStatement;

typedef struct {
    AstExpression *left;
    AstExpression *right;
} AstSwapStatement;

typedef enum {
    AST_STMT_LOCAL_BINDING = 0,
    AST_STMT_RETURN,
    AST_STMT_EXIT,
    AST_STMT_THROW,
    AST_STMT_EXPRESSION,
    AST_STMT_MANUAL,
    AST_STMT_SWAP
} AstStatementKind;

struct AstStatement {
    AstStatementKind kind;
    AstSourceSpan    source_span;
    union {
        AstLocalBindingStatement local_binding;
        AstExpression            *return_expression;
        AstExpression            *throw_expression;
        AstExpression            *expression;
        AstManualStatement       manual;
        AstSwapStatement         swap;
    } as;
};

struct AstBlock {
    AstStatement **statements;
    size_t       statement_count;
    size_t       statement_capacity;
};

typedef struct {
    AstModifier  *modifiers;
    size_t       modifier_count;
    size_t       modifier_capacity;
    bool         is_inferred_type;
    AstType      declared_type;
    char         *name;
    AstSourceSpan name_span;
    AstExpression *initializer;
} AstBindingDecl;

typedef struct {
    AstParameterList parameters;
    AstLambdaBody    body;
    AstSourceSpan    start_span;
    bool             is_boot;
} AstStartDecl;

typedef struct {
    char    *name;
    AstType *payload_type;   /* NULL if no payload */
} AstUnionVariant;

typedef struct {
    AstModifier  *modifiers;
    size_t       modifier_count;
    size_t       modifier_capacity;
    char         *name;
    AstSourceSpan name_span;
    char         **generic_params;
    size_t       generic_param_count;
    size_t       generic_param_capacity;
    AstUnionVariant *variants;
    size_t       variant_count;
    size_t       variant_capacity;
} AstUnionDecl;

typedef struct {
    AstModifier  *modifiers;
    size_t       modifier_count;
    size_t       modifier_capacity;
    AstType      target_type;
    char         *name;
    AstSourceSpan name_span;
} AstTypeAliasDecl;

typedef struct {
    AstModifier  *modifiers;
    size_t       modifier_count;
    size_t       modifier_capacity;
    AstType      return_type;
    char         *name;
    AstSourceSpan name_span;
    AstParameterList parameters;
    char         *body;         /* raw asm body text (between { and }) */
    size_t       body_length;
} AstAsmDecl;

typedef struct {
    AstType   field_type;
    char     *name;
} AstLayoutField;

typedef struct {
    char           *name;
    AstSourceSpan   name_span;
    AstLayoutField *fields;
    size_t          field_count;
    size_t          field_capacity;
} AstLayoutDecl;

typedef enum {
    AST_TOP_LEVEL_START = 0,
    AST_TOP_LEVEL_BINDING,
    AST_TOP_LEVEL_UNION,
    AST_TOP_LEVEL_ASM,
    AST_TOP_LEVEL_LAYOUT,
    AST_TOP_LEVEL_TYPE_ALIAS
} AstTopLevelDeclKind;

struct AstTopLevelDecl {
    AstTopLevelDeclKind kind;
    union {
        AstStartDecl   start_decl;
        AstBindingDecl binding_decl;
        AstUnionDecl   union_decl;
        AstAsmDecl     asm_decl;
        AstLayoutDecl  layout_decl;
        AstTypeAliasDecl type_alias_decl;
    } as;
};

typedef enum {
    AST_IMPORT_PLAIN = 0,
    AST_IMPORT_ALIAS,
    AST_IMPORT_WILDCARD,
    AST_IMPORT_SELECTIVE
} AstImportKind;

typedef struct {
    AstImportKind    kind;
    AstQualifiedName module_name;
    char            *alias;           /* for AST_IMPORT_ALIAS */
    char           **selected_names;  /* for AST_IMPORT_SELECTIVE */
    size_t           selected_count;
    size_t           selected_capacity;
} AstImportDecl;

typedef struct {
    bool             has_package;
    AstQualifiedName package_name;
    AstImportDecl   *imports;
    size_t           import_count;
    size_t           import_capacity;
    AstTopLevelDecl **top_level_decls;
    size_t           top_level_count;
    size_t           top_level_capacity;
} AstProgram;

#endif /* CALYNDA_AST_DECL_TYPES_H */
