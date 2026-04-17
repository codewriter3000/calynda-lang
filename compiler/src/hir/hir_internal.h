#ifndef HIR_INTERNAL_H
#define HIR_INTERNAL_H

#include "hir.h"

typedef struct {
    HirProgram         *program;
    const AstProgram   *ast_program;
    const SymbolTable  *symbols;
    const TypeChecker  *checker;
    const AstParameterList *inline_parameters;
    const Symbol      **inline_parameter_symbols;
    const AstExpression *const *inline_argument_sources;
    size_t              inline_resolved_count;
    size_t              synthetic_local_count;
} HirBuildContext;

/* hir_memory.c */
void hr_free_named_symbol(HirNamedSymbol *symbol);
void hr_free_callable_signature(HirCallableSignature *signature);
void hr_free_parameter_list(HirParameterList *list);
void hr_free_template_part_list(HirTemplatePartList *list);
void hr_free_call_expression(HirCallExpression *call);
void hr_free_array_literal_expression(HirArrayLiteralExpression *array_literal);

/* hir_helpers.c */
bool hr_source_span_is_valid(AstSourceSpan span);
size_t hr_primitive_byte_size(AstPrimitiveType primitive);
size_t hr_layout_byte_size(const SymbolTable *symbols, const char *name);
void hr_set_error(HirBuildContext *context,
                  AstSourceSpan primary_span,
                  const AstSourceSpan *related_span,
                  const char *format,
                  ...);
bool hr_reserve_items(void **items, size_t *capacity,
                      size_t needed, size_t item_size);
char *hr_qualified_name_to_string(const AstQualifiedName *name);
bool hr_append_named_symbol(HirNamedSymbol **items, size_t *count,
                            size_t *capacity, HirNamedSymbol symbol);
bool hr_append_top_level_decl(HirProgram *program, HirTopLevelDecl *decl);
bool hr_append_parameter(HirParameterList *list, HirParameter parameter);
bool hr_append_template_part(HirTemplatePartList *list, HirTemplatePart part);
bool hr_append_argument(HirCallExpression *call, HirExpression *argument);
bool hr_append_array_element(HirArrayLiteralExpression *array_literal,
                             HirExpression *element);
bool hr_append_statement(HirBlock *block, HirStatement *statement);
CheckedType hr_checked_type_from_resolved_type(ResolvedType type);
CheckedType hr_checked_type_void_value(void);
HirBlock *hr_block_new(void);
HirStatement *hr_statement_new(HirStatementKind kind);
HirExpression *hr_expression_new(HirExpressionKind kind);
HirTopLevelDecl *hr_top_level_decl_new(HirTopLevelDeclKind kind);

/* hir_lower.c */
bool hr_lower_callable_signature(HirBuildContext *context,
                                 const TypeCheckInfo *info,
                                 HirCallableSignature *signature);
bool hr_lower_parameters(HirBuildContext *context,
                         HirParameterList *out,
                         const AstParameterList *parameters,
                         const Scope *scope);
HirBlock *hr_lower_body_to_block(HirBuildContext *context,
                                 const AstLambdaBody *body);
HirBlock *hr_lower_start_body_to_block(HirBuildContext *context,
                                       const AstLambdaBody *body);
HirBlock *hr_lower_block(HirBuildContext *context, const AstBlock *block);

/* hir_lower_stmt.c */
HirStatement *hr_lower_statement(HirBuildContext *context,
                                 const AstStatement *statement,
                                 const Scope *scope);
bool hr_lower_swap_statement(HirBuildContext *context,
                             HirBlock *block,
                             const AstStatement *statement);

/* hir_lower_expr.c */
HirExpression *hr_lower_expression(HirBuildContext *context,
                                   const AstExpression *expression);
HirExpression *hr_lower_memory_expression(HirBuildContext *context,
                                          const AstExpression *expression,
                                          const TypeCheckInfo *info);

/* hir_lower_expr_ext.c */
HirExpression *hr_lower_expr_complex(HirBuildContext *context,
                                     const AstExpression *expression,
                                     HirExpression *hir_expression,
                                     const TypeCheckInfo *info);

/* hir_lower_decls.c */
bool hr_lower_top_level_decls(HirBuildContext *context);

#endif
