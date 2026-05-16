                    if (!mr_emit_nlr_invoke_call(context, outer_slot_val, get_val_temp,
                                                  expression->source_span)) {
                        free(nlr_retval_locals);
                        return false;
                    }

                    nlr_block = mr_current_block(context);
                    memset(&nlr_block->terminator, 0, sizeof(nlr_block->terminator));
                    nlr_block->terminator.kind = MIR_TERM_RETURN;
                    nlr_block->terminator.as.return_term.has_value = false;
                } else if (outer_ret.kind != CHECKED_TYPE_VOID) {
                    MirInstruction get_val_call;
                    memset(&get_val_call, 0, sizeof(get_val_call));
                    get_val_call.kind = MIR_INSTR_CALL;
                    if (!mr_value_from_global(context->build, "__calynda_rt_nlr_get_value",
                                              ext_type, &get_val_call.as.call.callee)) {
                        free(nlr_retval_locals);
                        return false;
                    }
                    get_val_call.as.call.has_result = true;
                    get_val_call.as.call.dest_temp = context->unit->next_temp_index++;
                    get_val_call.as.call.argument_count = 1;
                    get_val_call.as.call.arguments = calloc(1,
                                                             sizeof(*get_val_call.as.call.arguments));
                    if (!get_val_call.as.call.arguments) {
                        mr_instruction_free(&get_val_call);
                        free(nlr_retval_locals);
                        mr_set_error(context->build, expression->source_span, NULL,
                                     "Out of memory while lowering NLR get_value call.");
                        return false;
                    }
                    if (!mr_value_clone(context->build, &nlr_slot_value,
                                        &get_val_call.as.call.arguments[0])) {
                        mr_instruction_free(&get_val_call);
                        free(nlr_retval_locals);
                        return false;
                    }
                    nlr_block = mr_current_block(context);
                    if (!nlr_block || !mr_append_instruction(nlr_block, get_val_call)) {
                        mr_instruction_free(&get_val_call);
                        free(nlr_retval_locals);
                        mr_set_error(context->build, expression->source_span, NULL,
                                     "Out of memory while lowering NLR get_value.");
                        return false;
                    }
                    nlr_block = mr_current_block(context);
                    memset(&nlr_block->terminator, 0, sizeof(nlr_block->terminator));
                    nlr_block->terminator.kind = MIR_TERM_RETURN;
                    nlr_block->terminator.as.return_term.has_value = true;
                    nlr_block->terminator.as.return_term.value.kind = MIR_VALUE_TEMP;
                    nlr_block->terminator.as.return_term.value.type = outer_ret;
                    nlr_block->terminator.as.return_term.value.as.temp_index =
                        get_val_call.as.call.dest_temp;
                } else {
                    nlr_block = mr_current_block(context);
                    memset(&nlr_block->terminator, 0, sizeof(nlr_block->terminator));
                    nlr_block->terminator.kind = MIR_TERM_RETURN;
                    nlr_block->terminator.as.return_term.has_value = false;
                }
            }

            /* Switch to continue block for normal flow */
            context->current_block_index = continue_block_idx;
            free(nlr_retval_locals);
        } else {
            free(nlr_retval_locals);
        }

        if (instruction.as.call.has_result) {
            value->kind = MIR_VALUE_TEMP;
            value->type = expression->type;
            value->as.temp_index = instruction.as.call.dest_temp;
        } else {
            value->kind = MIR_VALUE_INVALID;
            value->type = expression->type;
        }
        return true;
    }
}
