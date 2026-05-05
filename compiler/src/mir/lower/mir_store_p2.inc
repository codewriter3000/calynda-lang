    block = mr_current_block(context);
    if (!block || block->terminator.kind != MIR_TERM_NONE) {
        return;
    }

    block->terminator.kind = MIR_TERM_GOTO;
    block->terminator.as.goto_term.target_block = target_block_index;
}

/* ---- Heap-cell helpers for capture-by-reference ---- */

bool mr_emit_cell_alloc(MirUnitBuildContext *context,
                        MirValue initial_value,
                        AstSourceSpan source_span,
                        size_t cell_local_index) {
    /* emit: tmp = __calynda_rt_cell_alloc(initial_value); store_local cell_local, tmp */
    MirInstruction call;
    CheckedType cell_type = mr_nlr_external_type();
    MirValue tmp;
    MirBasicBlock *block;

    memset(&call, 0, sizeof(call));
    call.kind = MIR_INSTR_CALL;
    call.as.call.has_result = true;
    call.as.call.dest_temp = context->unit->next_temp_index++;
    if (!mr_value_from_global(context->build, "__calynda_rt_cell_alloc",
                               cell_type, &call.as.call.callee)) {
        mr_value_free(&initial_value);
        return false;
    }
    call.as.call.argument_count = 1;
    call.as.call.arguments = calloc(1, sizeof(*call.as.call.arguments));
    if (!call.as.call.arguments) {
        mr_instruction_free(&call);
        mr_value_free(&initial_value);
        mr_set_error(context->build, source_span, NULL,
                     "Out of memory while lowering cell alloc.");
        return false;
    }
    call.as.call.arguments[0] = initial_value;
    block = mr_current_block(context);
    if (!block || !mr_append_instruction(block, call)) {
        mr_instruction_free(&call);
        mr_set_error(context->build, source_span, NULL,
                     "Out of memory while lowering cell alloc instruction.");
        return false;
    }
    tmp.kind = MIR_VALUE_TEMP;
    tmp.type = cell_type;
    tmp.as.temp_index = call.as.call.dest_temp;
    return mr_append_store_local_instruction(context, cell_local_index, tmp, source_span);
}

bool mr_emit_cell_read(MirUnitBuildContext *context,
                       size_t cell_local_index,
                       CheckedType value_type,
                       AstSourceSpan source_span,
                       MirValue *out) {
    /* emit: temp = __calynda_rt_cell_read(cell_local) */
    MirInstruction call;
    MirBasicBlock *block;
    size_t temp_index;
    MirValue cell_ref;

    memset(&call, 0, sizeof(call));
    call.kind = MIR_INSTR_CALL;
    call.as.call.has_result = true;
    temp_index = context->unit->next_temp_index++;
    call.as.call.dest_temp = temp_index;
    if (!mr_value_from_global(context->build, "__calynda_rt_cell_read",
                               value_type, &call.as.call.callee)) {
        return false;
    }
    call.as.call.argument_count = 1;
    call.as.call.arguments = calloc(1, sizeof(*call.as.call.arguments));
    if (!call.as.call.arguments) {
        mr_instruction_free(&call);
        mr_set_error(context->build, source_span, NULL,
                     "Out of memory while lowering cell read.");
        return false;
    }
    cell_ref.kind = MIR_VALUE_LOCAL;
    cell_ref.type = mr_nlr_external_type();
    cell_ref.as.local_index = cell_local_index;
    call.as.call.arguments[0] = cell_ref;
    block = mr_current_block(context);
    if (!block || !mr_append_instruction(block, call)) {
        mr_instruction_free(&call);
        mr_set_error(context->build, source_span, NULL,
                     "Out of memory while lowering cell read instruction.");
        return false;
    }
    out->kind = MIR_VALUE_TEMP;
    out->type = value_type;
    out->as.temp_index = temp_index;
    return true;
}

bool mr_emit_cell_write(MirUnitBuildContext *context,
                        size_t cell_local_index,
                        MirValue value,
                        AstSourceSpan source_span) {
    /* emit: __calynda_rt_cell_write(cell_local, value) */
    MirInstruction call;
    MirBasicBlock *block;
    CheckedType void_type = mr_checked_type_void_value();
    MirValue cell_ref;

    memset(&call, 0, sizeof(call));
    call.kind = MIR_INSTR_CALL;
    call.as.call.has_result = false;
    if (!mr_value_from_global(context->build, "__calynda_rt_cell_write",
                               void_type, &call.as.call.callee)) {
        mr_value_free(&value);
        return false;
    }
    call.as.call.argument_count = 2;
    call.as.call.arguments = calloc(2, sizeof(*call.as.call.arguments));
    if (!call.as.call.arguments) {
        mr_instruction_free(&call);
        mr_value_free(&value);
        mr_set_error(context->build, source_span, NULL,
                     "Out of memory while lowering cell write.");
        return false;
    }
    cell_ref.kind = MIR_VALUE_LOCAL;
    cell_ref.type = mr_nlr_external_type();
    cell_ref.as.local_index = cell_local_index;
    call.as.call.arguments[0] = cell_ref;
    call.as.call.arguments[1] = value;
    block = mr_current_block(context);
    if (!block || !mr_append_instruction(block, call)) {
        mr_instruction_free(&call);
        mr_set_error(context->build, source_span, NULL,
                     "Out of memory while lowering cell write instruction.");
        return false;
    }
    return true;
}

/* ------------------------------------------------------ */

bool mr_emit_nlr_invoke_call(MirUnitBuildContext *context,
                             MirValue slot_value,
                             MirValue return_value,
                             AstSourceSpan source_span) {
    MirInstruction call;
    CheckedType void_type = mr_checked_type_void_value();

    memset(&call, 0, sizeof(call));
    call.kind = MIR_INSTR_CALL;
    if (!mr_value_from_global(context->build,
                               "__calynda_rt_nlr_invoke",
                               void_type,
                               &call.as.call.callee)) {
        return false;
    }
    call.as.call.has_result = false;
    call.as.call.argument_count = 2;
    call.as.call.arguments = calloc(2, sizeof(*call.as.call.arguments));
    if (!call.as.call.arguments) {
        mr_instruction_free(&call);
        mr_set_error(context->build, source_span, NULL,
                     "Out of memory while lowering NLR invoke call.");
        return false;
    }

    if (!mr_value_clone(context->build, &slot_value, &call.as.call.arguments[0])) {
        mr_instruction_free(&call);
        return false;
    }

    if (return_value.kind != MIR_VALUE_INVALID) {
        if (!mr_value_clone(context->build, &return_value, &call.as.call.arguments[1])) {
            mr_instruction_free(&call);
            return false;
        }
    } else {
        /* void return: pass literal 0 as placeholder */
        call.as.call.arguments[1].kind = MIR_VALUE_LITERAL;
        call.as.call.arguments[1].type = mr_nlr_external_type();
        call.as.call.arguments[1].as.literal.kind = AST_LITERAL_INTEGER;
        call.as.call.arguments[1].as.literal.text = ast_copy_text("0");
        if (!call.as.call.arguments[1].as.literal.text) {
            mr_instruction_free(&call);
            mr_set_error(context->build, source_span, NULL,
                         "Out of memory while lowering NLR invoke argument.");
            return false;
        }
    }

    if (!mr_current_block(context) ||
        !mr_append_instruction(mr_current_block(context), call)) {
        mr_instruction_free(&call);
        mr_set_error(context->build, source_span, NULL,
                     "Out of memory while lowering NLR invoke call.");
        return false;
    }
    return true;
}
