#include "mir_internal.h"

static bool mr_lower_block_inline(MirUnitBuildContext *context, const HirBlock *block);

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
        {
            bool is_cell = mr_symbol_in_escape_set(
                context->escape_set,
                context->escape_set_count,
                statement->as.local_binding.symbol);

            memset(&local, 0, sizeof(local));
            local.kind = MIR_LOCAL_LOCAL;
            local.name = ast_copy_text(statement->as.local_binding.name);
            local.symbol = statement->as.local_binding.symbol;
            local.is_final = statement->as.local_binding.is_final;
            local.is_cell = is_cell;
            local.index = context->unit->local_count;
            if (is_cell) {
                /* Cell locals hold an opaque heap-cell reference */
                local.type = mr_nlr_external_type();
            } else {
                local.type = statement->as.local_binding.type;
            }
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
            if (is_cell) {
                return mr_emit_cell_alloc(context, value, statement->source_span, local.index);
            }
            return mr_append_store_local_instruction(context,
                                                  local.index,
                                                  value,
                                                  statement->source_span);
        }

    case HIR_STMT_RETURN:
        if (context->is_nlr_block) {
            /* NLR block: emit nlr_invoke(captures[0], value) + void return */
            MirValue slot_value;
            memset(&slot_value, 0, sizeof(slot_value));
            slot_value.kind = MIR_VALUE_LOCAL;
            slot_value.type = mr_nlr_external_type();
            slot_value.as.local_index = 0; /* NLR slot is always capture[0] */

            if (statement->as.return_expression) {
                if (!mr_lower_expression(context, statement->as.return_expression, &value)) {
                    return false;
                }
            } else {
                value = mr_invalid_value();
            }
            if (!mr_emit_active_cleanups(context, statement->source_span)) {
                mr_value_free(&value);
                return false;
            }
            if (!mr_emit_nlr_invoke_call(context, slot_value, value, statement->source_span)) {
                return false;
            }
            block = mr_current_block(context);
            if (!block) {
                mr_set_error(context->build,
                              statement->source_span,
                              NULL,
                              "Internal error: missing current MIR return block after NLR invoke.");
                return false;
            }
            memset(&block->terminator, 0, sizeof(block->terminator));
            block->terminator.kind = MIR_TERM_RETURN;
            block->terminator.as.return_term.has_value = false;
            return true;
        }
        if (statement->as.return_expression) {
            if (!mr_lower_expression(context, statement->as.return_expression, &value)) {
                return false;
            }
        } else {
            value = mr_invalid_value();
        }
        if (!mr_emit_active_cleanups(context, statement->source_span)) {
            mr_value_free(&value);
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
        if (statement->as.return_expression) {
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
        if (!mr_emit_active_cleanups(context, statement->source_span)) {
            mr_value_free(&value);
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

    case HIR_STMT_MANUAL:
        if (statement->as.manual.body) {
            bool prev_checked = context->in_checked_manual;
            bool result;
            if (!mr_enter_manual_scope(context, statement->source_span)) {
                return false;
            }
            context->in_checked_manual = statement->as.manual.is_checked
                                          || context->build->global_bounds_check;
            result = mr_lower_block_inline(context, statement->as.manual.body);
            if (result) {
                result = mr_leave_manual_scope(context, statement->source_span);
            }
            context->in_checked_manual = prev_checked;
            return result;
        }
        return true;
    }

    return false;
}

static bool mr_lower_block_inline(MirUnitBuildContext *context, const HirBlock *block) {
    MirBasicBlock *current_block;
    size_t i;
    /* Saved escape set from outer block scope */
    const Symbol **saved_escape_set = context->escape_set;
    size_t          saved_escape_count = context->escape_set_count;
    const Symbol **new_escape_set = NULL;
    size_t new_escape_count = 0;

    if (!context || !block) {
        return false;
    }

    if (!mr_compute_block_escape_set(context->build, block,
                                     &new_escape_set, &new_escape_count)) {
        return false;
    }
    context->escape_set = new_escape_set;
    context->escape_set_count = new_escape_count;

    for (i = 0; i < block->statement_count; i++) {
        current_block = mr_current_block(context);
        if (!current_block) {
            mr_set_error(context->build,
                          (AstSourceSpan){0},
                          NULL,
                          "Internal error: missing current MIR block while lowering block.");
            free(new_escape_set);
            context->escape_set = saved_escape_set;
            context->escape_set_count = saved_escape_count;
            return false;
        }
        if (current_block->terminator.kind != MIR_TERM_NONE) {
            break;
        }
        if (!mr_lower_statement(context, block->statements[i])) {
            free(new_escape_set);
            context->escape_set = saved_escape_set;
            context->escape_set_count = saved_escape_count;
            return false;
        }
    }

    free(new_escape_set);
    context->escape_set = saved_escape_set;
    context->escape_set_count = saved_escape_count;
    return true;
}

bool mr_lower_block(MirUnitBuildContext *context, const HirBlock *block) {
    MirBasicBlock *current_block;

    if (!mr_lower_block_inline(context, block)) {
        return false;
    }

#include "mir_lower_p2.inc"
