    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR closures.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.closure.dest_temp;
    return true;
}

bool mr_lower_nlr_block_lambda_expression(MirUnitBuildContext *context,
                                          const HirExpression *expression,
                                          size_t nlr_slot_local_index,
                                          MirValue *value) {
    MirCaptureList captures;
    MirInstruction instruction;
    char unit_name[128];
    const char *parent_name;
    int written;
    size_t i;

    memset(&captures, 0, sizeof(captures));
    if (!context || !expression || !value) {
        return false;
    }

    if (!mr_analyze_lambda_captures(context->build,
                                 &expression->as.lambda,
                                 expression->source_span,
                                 &captures)) {
        return false;
    }

    parent_name = (context->unit && context->unit->name) ? context->unit->name : "lambda";
    written = snprintf(unit_name,
                       sizeof(unit_name),
                       "%s$nlr%zu",
                       parent_name,
                       context->build->next_lambda_unit_index++);
    if (written < 0 || (size_t)written >= sizeof(unit_name)) {
        mr_capture_list_free(&captures);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Failed to name MIR NLR lambda unit.");
        return false;
    }

    /* Propagate is_cell from enclosing scope BEFORE building the lambda unit */
    for (i = 0; i < captures.count; i++) {
        size_t local_index = mr_find_local_index(context->unit, captures.items[i].symbol);
        captures.items[i].is_cell = (local_index != (size_t)-1) &&
                                    context->unit->locals[local_index].is_cell;
    }

    if (!mr_build_lambda_unit(context->build,
                           unit_name,
                           NULL,
                           &expression->as.lambda,
                           expression->callable_signature.return_type,
                           MIR_UNIT_LAMBDA,
                           &captures,
                           true)) {
        mr_capture_list_free(&captures);
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CLOSURE;
    instruction.as.closure.dest_temp = context->unit->next_temp_index++;
    instruction.as.closure.unit_name = ast_copy_text(unit_name);
    if (!instruction.as.closure.unit_name) {
        mr_capture_list_free(&captures);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR NLR closures.");
        return false;
    }

    /* captures[0] = NLR slot; captures[1..n] = regular captures */
    instruction.as.closure.capture_count = 1 + captures.count;
    if (instruction.as.closure.capture_count > 0) {
        instruction.as.closure.captures = calloc(instruction.as.closure.capture_count,
                                                 sizeof(*instruction.as.closure.captures));
        if (!instruction.as.closure.captures) {
            mr_capture_list_free(&captures);
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          expression->source_span,
                          NULL,
                          "Out of memory while lowering MIR NLR closures.");
            return false;
        }
    }

    /* capture[0] = NLR slot local */
    instruction.as.closure.captures[0].kind = MIR_VALUE_LOCAL;
    instruction.as.closure.captures[0].type = mr_nlr_external_type();
    instruction.as.closure.captures[0].as.local_index = nlr_slot_local_index;

    for (i = 0; i < captures.count; i++) {
        size_t local_index = mr_find_local_index(context->unit, captures.items[i].symbol);

        if (local_index == (size_t)-1) {
            mr_capture_list_free(&captures);
            mr_instruction_free(&instruction);
            mr_set_error(context->build,
                          expression->source_span,
                          &captures.items[i].symbol->declaration_span,
                          "Internal error: missing MIR capture for symbol '%s'.",
                          captures.items[i].symbol->name ? captures.items[i].symbol->name : "<anonymous>");
            return false;
        }

        instruction.as.closure.captures[1 + i].kind = MIR_VALUE_LOCAL;
        instruction.as.closure.captures[1 + i].type = captures.items[i].is_cell
            ? mr_nlr_external_type()
            : captures.items[i].type;
        instruction.as.closure.captures[1 + i].as.local_index = local_index;
    }

    mr_capture_list_free(&captures);

    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      expression->source_span,
                      NULL,
                      "Out of memory while lowering MIR NLR closures.");
        return false;
    }

    value->kind = MIR_VALUE_TEMP;
    value->type = expression->type;
    value->as.temp_index = instruction.as.closure.dest_temp;
    return true;
}

bool mr_build_nlr_return_unit(MirBuildContext *context,
                              const char *name,
                              bool has_value,
                              CheckedType value_type,
                              AstSourceSpan source_span) {
    MirUnit unit;
    MirUnitBuildContext unit_ctx;
    MirLocal nlr_slot_local;
    MirLocal retval_local;
    MirValue slot_value;
    MirValue retval_value;
    MirBasicBlock *block;
    bool ok = false;

    memset(&unit, 0, sizeof(unit));
    memset(&unit_ctx, 0, sizeof(unit_ctx));
    unit.kind = MIR_UNIT_LAMBDA;
    unit.name = ast_copy_text(name);
    unit.return_type = mr_checked_type_void_value();
    if (!unit.name) {
        mr_set_error(context, source_span, NULL,
                     "Out of memory while lowering NLR return unit.");
        return false;
    }

    /* capture[0] = NLR slot */
    memset(&nlr_slot_local, 0, sizeof(nlr_slot_local));
    nlr_slot_local.kind = MIR_LOCAL_CAPTURE;
    nlr_slot_local.name = ast_copy_text("__nlr_slot");
    nlr_slot_local.type = mr_nlr_external_type();
    nlr_slot_local.index = unit.local_count;
    if (!nlr_slot_local.name || !mr_append_local(&unit, nlr_slot_local)) {
        free(nlr_slot_local.name);
        mr_unit_free(&unit);
        mr_set_error(context, source_span, NULL,
                     "Out of memory while lowering NLR return unit slot capture.");
        return false;
    }

    if (has_value) {
        /* capture[1] = pre-evaluated return value */
        memset(&retval_local, 0, sizeof(retval_local));
        retval_local.kind = MIR_LOCAL_CAPTURE;
        retval_local.name = ast_copy_text("__nlr_retval");
        retval_local.type = value_type;
        retval_local.index = unit.local_count;
        if (!retval_local.name || !mr_append_local(&unit, retval_local)) {
            free(retval_local.name);
            mr_unit_free(&unit);
            mr_set_error(context, source_span, NULL,
                         "Out of memory while lowering NLR return unit value capture.");
            return false;
        }
    }

    unit_ctx.build = context;
    unit_ctx.unit = &unit;
    unit_ctx.in_checked_manual = context->global_bounds_check;
    if (!mr_create_block(&unit_ctx, &unit_ctx.current_block_index)) {
        goto cleanup;
    }

    memset(&slot_value, 0, sizeof(slot_value));
    slot_value.kind = MIR_VALUE_LOCAL;
    slot_value.type = mr_nlr_external_type();
    slot_value.as.local_index = 0;

    if (has_value) {
        memset(&retval_value, 0, sizeof(retval_value));
        retval_value.kind = MIR_VALUE_LOCAL;
        retval_value.type = value_type;
        retval_value.as.local_index = 1;
    } else {
        retval_value = mr_invalid_value();
    }

    if (!mr_emit_nlr_invoke_call(&unit_ctx, slot_value, retval_value, source_span)) {
        goto cleanup;
    }

    block = mr_current_block(&unit_ctx);
    if (!block) {
        mr_set_error(context, source_span, NULL,
                     "Internal error: missing block in NLR return unit.");
        goto cleanup;
    }
    block->terminator.kind = MIR_TERM_RETURN;
    block->terminator.as.return_term.has_value = false;

    if (!mr_append_unit(context->program, unit)) {
        mr_set_error(context, source_span, NULL,
                     "Out of memory while assembling NLR return unit.");
        goto cleanup;
    }

    ok = true;

#include "mir_lambda_p3.inc"
