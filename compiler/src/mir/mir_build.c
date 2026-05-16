#include "mir_internal.h"

static AstPrimitiveType mr_static_array_canonical_primitive(AstPrimitiveType primitive) {
    switch (primitive) {
    case AST_PRIMITIVE_BYTE:   return AST_PRIMITIVE_UINT8;
    case AST_PRIMITIVE_SBYTE:  return AST_PRIMITIVE_INT8;
    case AST_PRIMITIVE_SHORT:  return AST_PRIMITIVE_INT16;
    case AST_PRIMITIVE_INT:    return AST_PRIMITIVE_INT32;
    case AST_PRIMITIVE_UINT:   return AST_PRIMITIVE_UINT32;
    case AST_PRIMITIVE_LONG:   return AST_PRIMITIVE_INT64;
    case AST_PRIMITIVE_ULONG:  return AST_PRIMITIVE_UINT64;
    default:                   return primitive;
    }
}

static bool mr_static_array_primitive_is_integral(AstPrimitiveType primitive) {
    switch (mr_static_array_canonical_primitive(primitive)) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_INT64:
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_UINT16:
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_UINT64:
        return true;
    default:
        return false;
    }
}

static bool mr_static_array_primitive_is_signed(AstPrimitiveType primitive) {
    switch (mr_static_array_canonical_primitive(primitive)) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_INT64:
        return true;
    default:
        return false;
    }
}

static bool mr_is_static_scalar_expression(const HirExpression *expression) {
    AstPrimitiveType primitive;

    if (!expression) {
        return false;
    }

    if (expression->kind == HIR_EXPR_UNARY) {
        if (expression->as.unary.operator == AST_UNARY_OP_PLUS) {
            return mr_is_static_scalar_expression(expression->as.unary.operand);
        }
        if (expression->as.unary.operator == AST_UNARY_OP_NEGATE) {
            return expression->type.kind == CHECKED_TYPE_VALUE &&
                   expression->type.array_depth == 0 &&
                   expression->as.unary.operand != NULL &&
                   expression->as.unary.operand->kind == HIR_EXPR_LITERAL &&
                   expression->as.unary.operand->as.literal.kind == AST_LITERAL_INTEGER &&
                   mr_static_array_primitive_is_integral(expression->type.primitive) &&
                   mr_static_array_primitive_is_signed(expression->type.primitive);
        }
        return false;
    }

    if (expression->kind != HIR_EXPR_LITERAL ||
        expression->type.kind != CHECKED_TYPE_VALUE ||
        expression->type.array_depth != 0) {
        return false;
    }

    primitive = mr_static_array_canonical_primitive(expression->type.primitive);
    switch (expression->as.literal.kind) {
    case AST_LITERAL_BOOL:
        return primitive == AST_PRIMITIVE_BOOL;
    case AST_LITERAL_INTEGER:
        return mr_static_array_primitive_is_integral(primitive);
    default:
        return false;
    }
}

static bool mr_is_static_array_literal_expression(const HirExpression *expression) {
    size_t i;

    if (!expression ||
        expression->kind != HIR_EXPR_ARRAY_LITERAL ||
        expression->type.kind != CHECKED_TYPE_VALUE ||
        expression->type.array_depth == 0) {
        return false;
    }

    for (i = 0; i < expression->as.array_literal.element_count; i++) {
        const HirExpression *element = expression->as.array_literal.elements[i];

        if (!element) {
            return false;
        }

        if (element->type.kind == CHECKED_TYPE_VALUE &&
            element->type.array_depth > 0) {
            if (!mr_is_static_array_literal_expression(element)) {
                return false;
            }
        } else if (!mr_is_static_scalar_expression(element)) {
            return false;
        }
    }

    return true;
}

static bool mr_top_level_binding_uses_static_array_data(const HirTopLevelDecl *decl) {
    return decl != NULL &&
           decl->kind == HIR_TOP_LEVEL_BINDING &&
           decl->as.binding.is_final &&
           decl->as.binding.initializer != NULL &&
           mr_is_static_array_literal_expression(decl->as.binding.initializer);
}

bool mr_lower_module_init_unit(MirBuildContext *context,
                               bool *created_module_init_unit) {
    MirUnit unit;
    MirUnitBuildContext unit_context;
    MirBasicBlock *block;
    size_t i;
    bool has_initializers = false;
    bool ok = false;

    if (!context) {
        return false;
    }

    if (created_module_init_unit) {
        *created_module_init_unit = false;
    }

    for (i = 0; i < context->hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = context->hir_program->top_level_decls[i];

        if (decl->kind == HIR_TOP_LEVEL_BINDING &&
            !mr_top_level_binding_uses_lambda_unit(decl) &&
            !mr_top_level_binding_uses_static_array_data(decl)) {
            has_initializers = true;
            break;
        }
    }

    if (!has_initializers) {
        return true;
    }

    memset(&unit, 0, sizeof(unit));
    memset(&unit_context, 0, sizeof(unit_context));
    unit.kind = MIR_UNIT_INIT;
    unit.name = ast_copy_text(MIR_MODULE_INIT_NAME);
    unit.return_type = mr_checked_type_void_value();
    if (!unit.name) {
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while lowering MIR module init unit.");
        return false;
    }

    unit_context.build = context;
    unit_context.unit = &unit;
    unit_context.in_checked_manual = context->global_bounds_check;
    if (!mr_create_block(&unit_context, &unit_context.current_block_index)) {
        goto cleanup;
    }

    for (i = 0; i < context->hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = context->hir_program->top_level_decls[i];
        MirValue value;

        if (decl->kind != HIR_TOP_LEVEL_BINDING ||
            mr_top_level_binding_uses_lambda_unit(decl) ||
            mr_top_level_binding_uses_static_array_data(decl)) {
            continue;
        }

        if (!mr_lower_expression(&unit_context, decl->as.binding.initializer, &value) ||
            !mr_append_store_global_instruction(&unit_context,
                                             mr_named_symbol_global_name(
                                                 decl->as.binding.name,
                                                 decl->as.binding.symbol),
                                             value,
                                             decl->as.binding.source_span)) {
            goto cleanup;
        }
    }

    block = mr_current_block(&unit_context);
    if (!block) {
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Internal error: missing MIR block at module init exit.");
        goto cleanup;
    }
    if (block->terminator.kind == MIR_TERM_NONE) {
        block->terminator.kind = MIR_TERM_RETURN;
        block->terminator.as.return_term.has_value = false;
    }

    if (!mr_append_unit(context->program, unit)) {
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while assembling MIR module init unit.");
        goto cleanup;
    }

    ok = true;

    if (created_module_init_unit) {
        *created_module_init_unit = true;
    }

cleanup:
    mr_free_manual_cleanup_state(&unit_context);
    if (!ok) {
        mr_unit_free(&unit);
    }
    return ok;
}

bool mir_build_program(MirProgram *program, const HirProgram *hir_program,
                       bool global_bounds_check) {
    MirBuildContext context;
    const HirStartDecl *start_decl = NULL;
    size_t i;
    bool created_module_init_unit;

    if (!program || !hir_program) {
        return false;
    }

    mir_program_free(program);
    mir_program_init(program);

    memset(&context, 0, sizeof(context));
    context.program = program;
    context.hir_program = hir_program;
    context.global_bounds_check = global_bounds_check;

    if (hir_get_error(hir_program) != NULL) {
        const HirBuildError *hir_error = hir_get_error(hir_program);

        mr_set_error(&context,
                     hir_error->primary_span,
                     hir_error->has_related_span
                         ? &hir_error->related_span
                         : NULL,
                     "%s",
                     hir_error->message);
        return false;
    }

    for (i = 0; i < hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = hir_program->top_level_decls[i];

        if (decl->kind == HIR_TOP_LEVEL_START) {
            start_decl = &decl->as.start;
            continue;
        }

        /* Union declarations are type-only metadata; no MIR code emitted. */
        if (decl->kind == HIR_TOP_LEVEL_UNION) {
            continue;
        }

        if (decl->kind == HIR_TOP_LEVEL_ASM) {
            MirUnit asm_unit;
            memset(&asm_unit, 0, sizeof(asm_unit));
            asm_unit.kind = MIR_UNIT_ASM;
            asm_unit.name = ast_copy_text(mr_named_symbol_global_name(
                decl->as.asm_decl.name,
                decl->as.asm_decl.symbol));
            asm_unit.symbol = decl->as.asm_decl.symbol;
            asm_unit.return_type = decl->as.asm_decl.return_type;
            asm_unit.parameter_count = decl->as.asm_decl.parameter_count;
            asm_unit.asm_body = ast_copy_text_n(decl->as.asm_decl.body,
                                                decl->as.asm_decl.body_length);
            asm_unit.asm_body_length = decl->as.asm_decl.body_length;
            if (!asm_unit.name || !asm_unit.asm_body) {
                mr_unit_free(&asm_unit);
                mr_set_error(&context,
                              decl->as.asm_decl.source_span,
                              NULL,
                              "Out of memory while lowering asm unit '%s'.",
                              decl->as.asm_decl.name);
                return false;
            }
            if (!mr_append_unit(context.program, asm_unit)) {
                mr_unit_free(&asm_unit);
                mr_set_error(&context,
                              decl->as.asm_decl.source_span,
                              NULL,
                              "Out of memory while appending asm unit '%s'.",
                              decl->as.asm_decl.name);
                return false;
            }
            continue;
        }

        if (mr_top_level_binding_uses_lambda_unit(decl)) {
            if (!mr_lower_lambda_unit(&context,
                                   mr_named_symbol_global_name(
                                       decl->as.binding.name,
                                       decl->as.binding.symbol),
                                   decl->as.binding.symbol,
                                   &decl->as.binding.initializer->as.lambda,
                                   decl->as.binding.callable_signature.return_type,
                                   MIR_UNIT_BINDING)) {
                return false;
            }
            continue;
        }
    }

    if (!mr_lower_module_init_unit(&context, &created_module_init_unit)) {
        return false;
    }

    if (start_decl &&
        !mr_lower_start_unit(&context, start_decl, created_module_init_unit)) {
        return false;
    }

    for (i = 0; i < program->unit_count; i++) {
        mir_apply_self_tco(&program->units[i]);
    }

    return true;
}
