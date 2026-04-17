#include "mir_internal.h"

static bool mr_normalize_hosted_start_returns(MirBuildContext *context,
                                              MirUnit *unit,
                                              AstSourceSpan source_span) {
    size_t i;

    if (!context || !unit) {
        return false;
    }

    for (i = 0; i < unit->block_count; i++) {
        MirBasicBlock *block = &unit->blocks[i];

        if (block->terminator.kind != MIR_TERM_RETURN ||
            block->terminator.as.return_term.has_value) {
            continue;
        }

        block->terminator.as.return_term.value = mr_invalid_value();
        block->terminator.as.return_term.value.kind = MIR_VALUE_LITERAL;
        block->terminator.as.return_term.value.type =
            (CheckedType){CHECKED_TYPE_VALUE, AST_PRIMITIVE_INT32, 0, NULL, 0, false};
        block->terminator.as.return_term.value.as.literal.kind = AST_LITERAL_INTEGER;
        block->terminator.as.return_term.value.as.literal.text = ast_copy_text("0");
        if (!block->terminator.as.return_term.value.as.literal.text) {
            mr_set_error(context,
                         source_span,
                         NULL,
                         "Out of memory while normalizing void start returns.");
            return false;
        }
        block->terminator.as.return_term.has_value = true;
    }

    return true;
}

bool mr_lower_start_unit(MirBuildContext *context,
                         const HirStartDecl *start_decl,
                         bool call_module_init) {
    MirUnit unit;
    MirUnitBuildContext unit_context;
    bool ok = false;

    if (!context || !start_decl) {
        return false;
    }

    memset(&unit, 0, sizeof(unit));
    memset(&unit_context, 0, sizeof(unit_context));
    unit.kind = MIR_UNIT_START;
    unit.name = ast_copy_text(start_decl->is_boot ? "boot" : "start");
    unit.return_type =
        (CheckedType){CHECKED_TYPE_VALUE, AST_PRIMITIVE_INT32, 0, NULL, 0, false};
    unit.is_boot = start_decl->is_boot;
    if (!unit.name) {
        mr_set_error(context,
                      start_decl->source_span,
                      NULL,
                      "Out of memory while lowering MIR start unit.");
        return false;
    }

    unit_context.build = context;
    unit_context.unit = &unit;
    unit_context.in_checked_manual = context->global_bounds_check;
    if (!mr_create_block(&unit_context, &unit_context.current_block_index) ||
        !mr_lower_parameters(&unit_context, &start_decl->parameters)) {
        goto cleanup;
    }

    if (call_module_init &&
        !mr_append_call_global_instruction(&unit_context,
                                        MIR_MODULE_INIT_NAME,
                                        mr_checked_type_void_value(),
                                        start_decl->source_span)) {
        goto cleanup;
    }

    if (!mr_lower_block(&unit_context, start_decl->body)) {
        goto cleanup;
    }

    if (!start_decl->is_boot &&
        !mr_normalize_hosted_start_returns(context, &unit, start_decl->source_span)) {
        goto cleanup;
    }

    if (!mr_append_unit(context->program, unit)) {
        mr_set_error(context,
                      start_decl->source_span,
                      NULL,
                      "Out of memory while assembling MIR start unit.");
        goto cleanup;
    }

    ok = true;

cleanup:
    mr_free_manual_cleanup_state(&unit_context);
    if (!ok) {
        mr_unit_free(&unit);
    }

    return ok;
}
