#ifndef CALYNDA_AST_H
#define CALYNDA_AST_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    AST_MODIFIER_PUBLIC = 0,
    AST_MODIFIER_PRIVATE,
    AST_MODIFIER_FINAL
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
    AST_PRIMITIVE_STRING
} AstPrimitiveType;

typedef enum {
    AST_TYPE_VOID = 0,
    AST_TYPE_PRIMITIVE
} AstTypeKind;

typedef struct {
    bool  has_size;
    char *size_literal;
} AstArrayDimension;

typedef struct {
    AstTypeKind       kind;
    AstPrimitiveType  primitive;
    AstArrayDimension *dimensions;
    size_t            dimension_count;
    size_t            dimension_capacity;
} AstType;

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
    AST_UNARY_OP_PLUS
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
    AST_EXPR_GROUPING
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
    } as;
};

typedef struct {
    bool          is_final;
    bool          is_inferred_type;
    AstType       declared_type;
    char          *name;
    AstSourceSpan name_span;
    AstExpression *initializer;
} AstLocalBindingStatement;

typedef enum {
    AST_STMT_LOCAL_BINDING = 0,
    AST_STMT_RETURN,
    AST_STMT_EXIT,
    AST_STMT_THROW,
    AST_STMT_EXPRESSION
} AstStatementKind;

struct AstStatement {
    AstStatementKind kind;
    AstSourceSpan    source_span;
    union {
        AstLocalBindingStatement local_binding;
        AstExpression            *return_expression;
        AstExpression            *throw_expression;
        AstExpression            *expression;
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
} AstStartDecl;

typedef enum {
    AST_TOP_LEVEL_START = 0,
    AST_TOP_LEVEL_BINDING
} AstTopLevelDeclKind;

struct AstTopLevelDecl {
    AstTopLevelDeclKind kind;
    union {
        AstStartDecl   start_decl;
        AstBindingDecl binding_decl;
    } as;
};

typedef struct {
    bool             has_package;
    AstQualifiedName package_name;
    AstQualifiedName *imports;
    size_t           import_count;
    size_t           import_capacity;
    AstTopLevelDecl **top_level_decls;
    size_t           top_level_count;
    size_t           top_level_capacity;
} AstProgram;

char *ast_copy_text(const char *text);
char *ast_copy_text_n(const char *text, size_t length);

void ast_qualified_name_init(AstQualifiedName *name);
void ast_qualified_name_free(AstQualifiedName *name);
bool ast_qualified_name_append(AstQualifiedName *name, const char *segment);

void ast_type_init_void(AstType *type);
void ast_type_init_primitive(AstType *type, AstPrimitiveType primitive);
void ast_type_free(AstType *type);
bool ast_type_add_dimension(AstType *type, bool has_size, const char *size_literal);

void ast_parameter_list_init(AstParameterList *list);
void ast_parameter_list_free(AstParameterList *list);
bool ast_parameter_list_append(AstParameterList *list, AstParameter *parameter);

AstBlock *ast_block_new(void);
void ast_block_free(AstBlock *block);
bool ast_block_append_statement(AstBlock *block, AstStatement *statement);

AstExpression *ast_expression_new(AstExpressionKind kind);
void ast_expression_free(AstExpression *expression);
bool ast_expression_list_append(AstExpressionList *list, AstExpression *expression);
bool ast_template_literal_append_text(AstLiteral *literal, const char *text);
bool ast_template_literal_append_expression(AstLiteral *literal,
                                            AstExpression *expression);

AstStatement *ast_statement_new(AstStatementKind kind);
void ast_statement_free(AstStatement *statement);

AstTopLevelDecl *ast_top_level_decl_new(AstTopLevelDeclKind kind);
void ast_top_level_decl_free(AstTopLevelDecl *decl);
bool ast_binding_decl_add_modifier(AstBindingDecl *decl, AstModifier modifier);

void ast_program_init(AstProgram *program);
void ast_program_free(AstProgram *program);
bool ast_program_set_package(AstProgram *program, AstQualifiedName *package_name);
bool ast_program_add_import(AstProgram *program, AstQualifiedName *import_name);
bool ast_program_add_top_level_decl(AstProgram *program, AstTopLevelDecl *decl);

#endif /* CALYNDA_AST_H */