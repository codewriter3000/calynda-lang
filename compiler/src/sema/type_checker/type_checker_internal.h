#ifndef CALYNDA_TYPE_CHECKER_INTERNAL_H
#define CALYNDA_TYPE_CHECKER_INTERNAL_H

#include "type_checker.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    BLOCK_CONTEXT_LAMBDA = 0,
    BLOCK_CONTEXT_START
} BlockContextKind;

typedef struct {
    BlockContextKind kind;
    bool             has_expected_return_type;
    CheckedType      expected_return_type;
    AstSourceSpan    owner_span;
    AstSourceSpan    related_span;
    bool             has_related_span;
} BlockContext;

/* type_checker_names.c */
bool tc_reserve_items(void **items, size_t *capacity,
                      size_t needed, size_t item_size);
const char *tc_primitive_type_name(AstPrimitiveType primitive);
const char *tc_binary_operator_name(AstBinaryOperator operator);
const char *tc_assignment_operator_name(AstAssignmentOperator operator);
const char *tc_block_context_name(BlockContextKind kind);

/* type_checker_types.c */
CheckedType tc_checked_type_invalid(void);
CheckedType tc_checked_type_void(void);
CheckedType tc_checked_type_null(void);
CheckedType tc_checked_type_external(void);
CheckedType tc_checked_type_value(AstPrimitiveType primitive, size_t array_depth);
CheckedType tc_checked_type_named(const char *name, size_t generic_arg_count,
                                  size_t array_depth);
CheckedType tc_checked_type_type_param(const char *name);
bool tc_checked_type_is_hetero_array(CheckedType type);
bool tc_checked_type_equals(CheckedType left, CheckedType right);
bool tc_checked_type_is_scalar_value(CheckedType type);
bool tc_checked_type_is_bool(CheckedType type);
bool tc_checked_type_is_string(CheckedType type);
bool tc_checked_type_is_numeric(CheckedType type);
bool tc_checked_type_is_integral(CheckedType type);
bool tc_checked_type_is_reference_like(CheckedType type);
bool tc_primitive_is_float(AstPrimitiveType primitive);
bool tc_primitive_is_integral(AstPrimitiveType primitive);
int tc_primitive_width(AstPrimitiveType primitive);
bool tc_primitive_is_signed(AstPrimitiveType primitive);
AstPrimitiveType tc_signed_primitive_for_width(int width);
AstPrimitiveType tc_unsigned_primitive_for_width(int width);

/* type_checker_convert.c */
CheckedType tc_promote_numeric_types(CheckedType left, CheckedType right);
CheckedType tc_checked_type_from_resolved_type(ResolvedType type);
CheckedType tc_checked_type_from_ast_type(TypeChecker *checker, const AstType *type);
CheckedType tc_checked_type_from_cast_target(TypeChecker *checker,
                                             const AstExpression *expression);
bool tc_checked_type_assignable(CheckedType target, CheckedType source);
bool tc_merge_types_for_inference(CheckedType left, CheckedType right,
                                  CheckedType *merged);
bool tc_merge_return_types(CheckedType left, CheckedType right,
                           CheckedType *merged);
TypeCheckInfo tc_type_check_info_make(CheckedType type);
TypeCheckInfo tc_type_check_info_make_callable(CheckedType return_type,
                                               const AstParameterList *parameters);
TypeCheckInfo tc_type_check_info_make_external_value(void);
TypeCheckInfo tc_type_check_info_make_external_callable(void);
CheckedType tc_type_check_source_type(const TypeCheckInfo *info);
const AstSourceSpan *tc_block_context_related_span(const BlockContext *context,
                                                   AstSourceSpan primary_span);
bool tc_source_span_is_valid(AstSourceSpan span);

/* type_checker_util.c */
void tc_set_error(TypeChecker *checker, const char *format, ...);
void tc_set_error_at(TypeChecker *checker,
                     AstSourceSpan primary_span,
                     const AstSourceSpan *related_span,
                     const char *format, ...);
TypeCheckExpressionEntry *tc_ensure_expression_entry(TypeChecker *checker,
                                                     const AstExpression *expression);
TypeCheckSymbolEntry *tc_ensure_symbol_entry(TypeChecker *checker,
                                             const Symbol *symbol);
const TypeCheckInfo *tc_store_expression_info(TypeChecker *checker,
                                              const AstExpression *expression,
                                              TypeCheckInfo info);
bool tc_validate_binding_modifiers(TypeChecker *checker,
                                   const AstBindingDecl *binding);
bool tc_validate_internal_access(TypeChecker *checker,
                                 const AstExpression *identifier,
                                 const Symbol *symbol);
const TypeCheckInfo *tc_check_hetero_array_literal(TypeChecker *checker,
                                                   const AstExpression *expression);

/* type_checker_resolve.c */
bool tc_validate_program_start_decls(TypeChecker *checker,
                                     const AstProgram *program);
const TypeCheckInfo *tc_resolve_symbol_info(TypeChecker *checker,
                                            const Symbol *symbol);
bool tc_resolve_binding_symbol(TypeChecker *checker,
                               const Symbol *symbol,
                               const AstType *declared_type,
                               bool is_inferred_type,
                               const AstExpression *initializer,
                               TypeCheckInfo *info);

/* type_checker_lambda.c */
bool tc_resolve_parameters_in_scope(TypeChecker *checker,
                                    const AstParameterList *parameters,
                                    const Scope *scope);
const TypeCheckInfo *tc_check_lambda_expression(TypeChecker *checker,
                                                const AstExpression *expression,
                                                const CheckedType *expected_return_type,
                                                const AstSourceSpan *related_span);
bool tc_check_start_decl(TypeChecker *checker, const AstStartDecl *start_decl);

/* type_checker_block.c */
bool tc_check_block(TypeChecker *checker, const AstBlock *block,
                    const BlockContext *context,
                    CheckedType *return_type, AstSourceSpan *return_span);

/* type_checker_ops.c */
bool tc_check_binary_operator(TypeChecker *checker,
                              const AstExpression *expression,
                              AstBinaryOperator operator,
                              CheckedType left_type,
                              CheckedType right_type,
                              CheckedType *result_type);
bool tc_map_compound_assignment(AstAssignmentOperator operator,
                                AstBinaryOperator *binary_operator);
bool tc_expression_is_assignment_target(TypeChecker *checker,
                                        const AstExpression *expression,
                                        const Symbol **root_symbol);

/* type_checker_expr.c */
const TypeCheckInfo *tc_check_expression(TypeChecker *checker,
                                         const AstExpression *expression);

/* type_checker_expr_ext.c */
const TypeCheckInfo *tc_check_expression_ext(TypeChecker *checker,
                                             const AstExpression *expression);

/* type_checker_expr_more.c */
const TypeCheckInfo *tc_check_expression_more(TypeChecker *checker,
                                              const AstExpression *expression);

#endif /* CALYNDA_TYPE_CHECKER_INTERNAL_H */
