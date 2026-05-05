/* mir_lvalue.c */
bool mr_map_compound_assignment(AstAssignmentOperator operator,
                                AstBinaryOperator *binary_operator);
bool mr_lower_assignment_target(MirUnitBuildContext *context,
                                const HirExpression *expression,
                                MirLValue *lvalue);
bool mr_load_lvalue_value(MirUnitBuildContext *context,
                          const MirLValue *lvalue,
                          AstSourceSpan source_span,
                          MirValue *value);
bool mr_store_lvalue_value(MirUnitBuildContext *context,
                           const MirLValue *lvalue,
                           MirValue value,
                           AstSourceSpan source_span);

/* mir_control.c */
bool mr_lower_ternary_expression(MirUnitBuildContext *context,
                                 const HirExpression *expression,
                                 MirValue *value);
bool mr_lower_short_circuit_binary(MirUnitBuildContext *context,
                                   const HirExpression *expression,
                                   MirValue *value);

/* mir_expr_memory.c */
bool mr_lower_memory_op_expression(MirUnitBuildContext *context,
                                   const HirExpression *expression,
                                   MirValue *value);
bool mr_register_manual_cleanup(MirUnitBuildContext *context,
                                const MirValue *argument,
                                const MirValue *callee,
                                AstSourceSpan source_span,
                                MirValue *captured_argument);
bool mr_enter_manual_scope(MirUnitBuildContext *context,
                           AstSourceSpan source_span);
bool mr_leave_manual_scope(MirUnitBuildContext *context,
                           AstSourceSpan source_span);
bool mr_emit_active_cleanups(MirUnitBuildContext *context,
                             AstSourceSpan source_span);
void mr_free_manual_cleanup_state(MirUnitBuildContext *context);

/* mir_assign.c */
bool mr_lower_assignment_expression(MirUnitBuildContext *context,
                                    const HirExpression *expression,
                                    MirValue *value);
bool mr_lower_postfix_increment(MirUnitBuildContext *context,
                                const HirExpression *expression,
                                MirValue *value, bool is_increment);

/* mir_expr_helpers.c */
bool mr_lower_template_part_value(MirUnitBuildContext *context,
                                  const HirExpression *expression,
                                  MirValue *value);
bool mr_lower_template_expression(MirUnitBuildContext *context,
                                  const HirExpression *expression,
                                  MirValue *value);
bool mr_lower_member_expression(MirUnitBuildContext *context,
                                const HirExpression *expression,
                                MirValue *value);
bool mr_lower_index_expression(MirUnitBuildContext *context,
                               const HirExpression *expression,
                               MirValue *value);
bool mr_lower_array_literal(MirUnitBuildContext *context,
                            const HirExpression *expression,
                            MirValue *value);

/* mir_expr.c */
bool mr_lower_expression(MirUnitBuildContext *context,
                         const HirExpression *expression,
                         MirValue *value);

/* mir_lower.c */
bool mr_lower_statement(MirUnitBuildContext *context,
                        const HirStatement *statement);
bool mr_lower_block(MirUnitBuildContext *context, const HirBlock *block);
bool mr_lower_parameters(MirUnitBuildContext *context,
                         const HirParameterList *parameters);
bool mr_top_level_binding_uses_lambda_unit(const HirTopLevelDecl *decl);
bool mr_lower_module_init_unit(MirBuildContext *context,
                               bool *created_module_init_unit);

/* Inline helper for CHECKED_TYPE_EXTERNAL (used for NLR slot pointers) */
static inline CheckedType mr_nlr_external_type(void) {
    CheckedType t;
    memset(&t, 0, sizeof(t));
    t.kind = CHECKED_TYPE_EXTERNAL;
    return t;
}

/* mir_lambda.c */
bool mr_build_lambda_unit(MirBuildContext *context,
                          const char *name, const Symbol *symbol,
                          const HirLambdaExpression *lambda,
                          CheckedType return_type, MirUnitKind kind,
                          const MirCaptureList *captures,
                          bool is_nlr_block);
bool mr_lower_lambda_unit(MirBuildContext *context,
                          const char *name, const Symbol *symbol,
                          const HirLambdaExpression *lambda,
                          CheckedType return_type, MirUnitKind kind);
bool mr_lower_lambda_expression(MirUnitBuildContext *context,
                                const HirExpression *expression,
                                MirValue *value);
bool mr_lower_nlr_block_lambda_expression(MirUnitBuildContext *context,
                                          const HirExpression *expression,
                                          size_t nlr_slot_local_index,
                                          MirValue *value);
bool mr_build_nlr_return_unit(MirBuildContext *context,
                              const char *name,
                              bool has_value,
                              CheckedType value_type,
                              AstSourceSpan source_span);

/* mir_store.c (NLR helpers) */
bool mr_emit_nlr_invoke_call(MirUnitBuildContext *context,
                             MirValue slot_value,
                             MirValue return_value,
                             AstSourceSpan source_span);
bool mr_lower_start_unit(MirBuildContext *context,
                         const HirStartDecl *start_decl,
                         bool call_module_init);

