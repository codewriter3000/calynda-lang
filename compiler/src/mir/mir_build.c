#include "mir_internal.h"

bool mr_lower_module_init_unit(MirBuildContext *context,
                               bool *created_module_init_unit) {
    MirUnit unit;
    MirUnitBuildContext unit_context;
    MirBasicBlock *block;
    size_t i;
    bool has_initializers = false;

    if (!context) {
        return false;
    }

    if (created_module_init_unit) {
        *created_module_init_unit = false;
    }

    for (i = 0; i < context->hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = context->hir_program->top_level_decls[i];

        if (decl->kind == HIR_TOP_LEVEL_BINDING &&
            !mr_top_level_binding_uses_lambda_unit(decl)) {
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
    if (!mr_create_block(&unit_context, &unit_context.current_block_index)) {
        mr_unit_free(&unit);
        return false;
    }

    for (i = 0; i < context->hir_program->top_level_count; i++) {
        const HirTopLevelDecl *decl = context->hir_program->top_level_decls[i];
        MirValue value;

        if (decl->kind != HIR_TOP_LEVEL_BINDING ||
            mr_top_level_binding_uses_lambda_unit(decl)) {
            continue;
        }

        if (!mr_lower_expression(&unit_context, decl->as.binding.initializer, &value) ||
            !mr_append_store_global_instruction(&unit_context,
                                             decl->as.binding.name,
                                             value,
                                             decl->as.binding.source_span)) {
            mr_unit_free(&unit);
            return false;
        }
    }

    block = mr_current_block(&unit_context);
    if (!block) {
        mr_unit_free(&unit);
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Internal error: missing MIR block at module init exit.");
        return false;
    }
    if (block->terminator.kind == MIR_TERM_NONE) {
        block->terminator.kind = MIR_TERM_RETURN;
        block->terminator.as.return_term.has_value = false;
    }

    if (!mr_append_unit(context->program, unit)) {
        mr_unit_free(&unit);
        mr_set_error(context,
                      (AstSourceSpan){0},
                      NULL,
                      "Out of memory while assembling MIR module init unit.");
        return false;
    }

    if (created_module_init_unit) {
        *created_module_init_unit = true;
    }
    return true;
}

bool mir_build_program(MirProgram *program, const HirProgram *hir_program) {
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

    if (hir_get_error(hir_program) != NULL) {
        mr_set_error(&context,
                      (AstSourceSpan){0},
                      NULL,
                      "Cannot lower program to MIR while the HIR reports errors.");
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
            asm_unit.name = ast_copy_text(decl->as.asm_decl.name);
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
                                   decl->as.binding.name,
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

    return true;
}
