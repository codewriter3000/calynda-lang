#include "mir_internal.h"

bool mr_lower_statement(MirUnitBuildContext *context,
                        const HirStatement *statement) {
    MirBasicBlock *block;
    MirValue value;
    MirLocal local;

    if (!context || !statement) {
        return false;
    }

    block = mr_current_block(context);
    if (!block) {
        mr_set_error(context->build,
                      statement->source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering statement.");
        return false;
    }

    switch (statement->kind) {
    case HIR_STMT_LOCAL_BINDING:
        memset(&local, 0, sizeof(local));
        local.kind = MIR_LOCAL_LOCAL;
        local.name = ast_copy_text(statement->as.local_binding.name);
        local.symbol = statement->as.local_binding.symbol;
        local.type = statement->as.local_binding.type;
        local.is_final = statement->as.local_binding.is_final;
        local.index = context->unit->local_count;
        if (!local.name || !mr_append_local(context->unit, local)) {
            free(local.name);
            mr_set_error(context->build,
                          statement->source_span,
                          NULL,
                          "Out of memory while lowering MIR locals.");
            return false;
        }
        if (!mr_lower_expression(context, statement->as.local_binding.initializer, &value)) {
            return false;
        }
        return mr_append_store_local_instruction(context,
                                              local.index,
                                              value,
                                              statement->source_span);

    case HIR_STMT_RETURN:
        memset(&block->terminator, 0, sizeof(block->terminator));
        block->terminator.kind = MIR_TERM_RETURN;
        if (statement->as.return_expression) {
            if (!mr_lower_expression(context, statement->as.return_expression, &value)) {
                return false;
            }
            block = mr_current_block(context);
            if (!block) {
                mr_value_free(&value);
                mr_set_error(context->build,
                              statement->source_span,
                              NULL,
                              "Internal error: missing current MIR return block.");
                return false;
            }
            memset(&block->terminator, 0, sizeof(block->terminator));
            block->terminator.kind = MIR_TERM_RETURN;
            block->terminator.as.return_term.has_value = true;
            block->terminator.as.return_term.value = value;
        }
        return true;

    case HIR_STMT_EXPRESSION:
        if (!statement->as.expression) {
            return true;
        }
        if (!mr_lower_expression(context, statement->as.expression, &value)) {
            return false;
        }
        mr_value_free(&value);
        return true;

    case HIR_STMT_THROW:
        if (!mr_lower_expression(context, statement->as.throw_expression, &value)) {
            return false;
        }
        block = mr_current_block(context);
        if (!block) {
            mr_value_free(&value);
            mr_set_error(context->build,
                          statement->source_span,
                          NULL,
                          "Internal error: missing current MIR throw block.");
            return false;
        }
        memset(&block->terminator, 0, sizeof(block->terminator));
        block->terminator.kind = MIR_TERM_THROW;
        block->terminator.as.throw_term.value = value;
        return true;

    case HIR_STMT_EXIT:
        mr_set_error(context->build,
                      statement->source_span,
                      NULL,
                      "Internal error: exit should already be normalized before MIR lowering.");
        return false;
    }

    return false;
}

bool mr_lower_block(MirUnitBuildContext *context, const HirBlock *block) {
    MirBasicBlock *current_block;
    size_t i;

    if (!context || !block) {
        return false;
    }

    for (i = 0; i < block->statement_count; i++) {
        current_block = mr_current_block(context);
        if (!current_block) {
            mr_set_error(context->build,
                          (AstSourceSpan){0},
                          NULL,
                          "Internal error: missing current MIR block while lowering block.");
            return false;
        }
        if (current_block->terminator.kind != MIR_TERM_NONE) {
            break;
        }
        if (!mr_lower_statement(context, block->statements[i])) {
            return false;
        }
    }

    current_block = mr_current_block(context);
    if (!current_block) {
        mr_set_error(context->build,
                      (AstSourceSpan){0},
                      NULL,
                      "Internal error: missing current MIR block at block exit.");
        return false;
    }

    if (current_block->terminator.kind == MIR_TERM_NONE) {
        current_block->terminator.kind = MIR_TERM_RETURN;
        current_block->terminator.as.return_term.has_value = false;
    }
    return true;
}

bool mr_lower_parameters(MirUnitBuildContext *context,
                         const HirParameterList *parameters) {
    size_t i;

    if (!context || !parameters) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        MirLocal local;

        memset(&local, 0, sizeof(local));
        local.kind = MIR_LOCAL_PARAMETER;
        local.name = ast_copy_text(parameters->items[i].name);
        local.symbol = parameters->items[i].symbol;
        local.type = parameters->items[i].type;
        local.index = context->unit->local_count;
        if (!local.name || !mr_append_local(context->unit, local)) {
            free(local.name);
            mr_set_error(context->build,
                          parameters->items[i].source_span,
                          NULL,
                          "Out of memory while lowering MIR parameters.");
            return false;
        }
        context->unit->parameter_count++;
    }

    return true;
}

bool mr_lower_start_unit(MirBuildContext *context,
                         const HirStartDecl *start_decl,
                         bool call_module_init) {
    MirUnit unit;
    MirUnitBuildContext unit_context;

    if (!context || !start_decl) {
        return false;
    }

    memset(&unit, 0, sizeof(unit));
    memset(&unit_context, 0, sizeof(unit_context));
    unit.kind = MIR_UNIT_START;
    unit.name = ast_copy_text(start_decl->is_boot ? "boot" : "start");
    unit.return_type = (CheckedType){CHECKED_TYPE_VALUE, AST_PRIMITIVE_INT32, 0, NULL, 0};
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
    if (!mr_create_block(&unit_context, &unit_context.current_block_index) ||
        !mr_lower_parameters(&unit_context, &start_decl->parameters)) {
        mr_unit_free(&unit);
        return false;
    }

    if (call_module_init &&
        !mr_append_call_global_instruction(&unit_context,
                                        MIR_MODULE_INIT_NAME,
                                        mr_checked_type_void_value(),
                                        start_decl->source_span)) {
        mr_unit_free(&unit);
        return false;
    }

    if (!mr_lower_block(&unit_context, start_decl->body)) {
        mr_unit_free(&unit);
        return false;
    }

    if (!mr_append_unit(context->program, unit)) {
        mr_unit_free(&unit);
        mr_set_error(context,
                      start_decl->source_span,
                      NULL,
                      "Out of memory while assembling MIR start unit.");
        return false;
    }

    return true;
}
