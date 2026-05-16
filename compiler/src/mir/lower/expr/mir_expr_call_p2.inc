        for (i = 0; i < n_args; i++) {
            const HirExpression *a = expression->as.call.arguments[i];

            if (has_nlr && a->kind == HIR_EXPR_NONLOCAL_RETURN) {
                /* Build implicit NLR return closure for this argument */
                char nlr_unit_name[128];
                int written;
                bool has_retval = (nlr_retval_locals[i] != (size_t)-1);
                CheckedType retval_type;
                MirInstruction closure_instr;
                size_t cap_count;

                memset(&retval_type, 0, sizeof(retval_type));
                if (has_retval) {
                    retval_type = context->unit->locals[nlr_retval_locals[i]].type;
                }
                written = snprintf(nlr_unit_name, sizeof(nlr_unit_name),
                                   "%s$nlrret%zu",
                                   context->unit->name ? context->unit->name : "lambda",
                                   context->build->next_lambda_unit_index++);
                if (written < 0 || (size_t)written >= sizeof(nlr_unit_name)) {
                    free(nlr_retval_locals);
                    mr_instruction_free(&instruction);
                    mr_set_error(context->build, a->source_span, NULL,
                                 "Failed to name MIR NLR return unit.");
                    return false;
                }
                if (!mr_build_nlr_return_unit(context->build, nlr_unit_name,
                                              has_retval, retval_type, a->source_span)) {
                    free(nlr_retval_locals);
                    mr_instruction_free(&instruction);
                    return false;
                }

                /* Emit closure with [nlr_slot, retval?] captures */
                cap_count = has_retval ? 2 : 1;
                memset(&closure_instr, 0, sizeof(closure_instr));
                closure_instr.kind = MIR_INSTR_CLOSURE;
                closure_instr.as.closure.dest_temp = context->unit->next_temp_index++;
                closure_instr.as.closure.unit_name = ast_copy_text(nlr_unit_name);
                if (!closure_instr.as.closure.unit_name) {
                    free(nlr_retval_locals);
                    mr_instruction_free(&instruction);
                    mr_set_error(context->build, a->source_span, NULL,
                                 "Out of memory while lowering NLR return closure.");
                    return false;
                }
                closure_instr.as.closure.capture_count = cap_count;
                closure_instr.as.closure.captures = calloc(cap_count,
                                                            sizeof(*closure_instr.as.closure.captures));
                if (!closure_instr.as.closure.captures) {
                    mr_instruction_free(&closure_instr);
                    free(nlr_retval_locals);
                    mr_instruction_free(&instruction);
                    mr_set_error(context->build, a->source_span, NULL,
                                 "Out of memory while lowering NLR return closure captures.");
                    return false;
                }
                closure_instr.as.closure.captures[0].kind = MIR_VALUE_LOCAL;
                closure_instr.as.closure.captures[0].type = mr_nlr_external_type();
                closure_instr.as.closure.captures[0].as.local_index = nlr_slot_local;
                if (has_retval) {
                    closure_instr.as.closure.captures[1].kind = MIR_VALUE_LOCAL;
                    closure_instr.as.closure.captures[1].type = retval_type;
                    closure_instr.as.closure.captures[1].as.local_index = nlr_retval_locals[i];
                }
                if (!mr_current_block(context) ||
                    !mr_append_instruction(mr_current_block(context), closure_instr)) {
                    mr_instruction_free(&closure_instr);
                    free(nlr_retval_locals);
                    mr_instruction_free(&instruction);
                    mr_set_error(context->build, a->source_span, NULL,
                                 "Out of memory while lowering NLR return closure instruction.");
                    return false;
                }
                instruction.as.call.arguments[i].kind = MIR_VALUE_TEMP;
                instruction.as.call.arguments[i].type = a->type;
                instruction.as.call.arguments[i].as.temp_index = closure_instr.as.closure.dest_temp;

            } else if (has_nlr && a->kind == HIR_EXPR_LAMBDA && a->as.lambda.is_nlr_block) {
                /* Build explicit NLR block lambda */
                if (!mr_lower_nlr_block_lambda_expression(context, a, nlr_slot_local,
                                                          &instruction.as.call.arguments[i])) {
                    free(nlr_retval_locals);
                    mr_instruction_free(&instruction);
                    return false;
                }
            } else {
                if (!mr_lower_expression(context, a, &instruction.as.call.arguments[i])) {
                    free(nlr_retval_locals);
                    mr_instruction_free(&instruction);
                    return false;
                }
            }
        }

        if (!mr_current_block(context)) {
            free(nlr_retval_locals);
            mr_instruction_free(&instruction);
            mr_set_error(context->build, expression->source_span, NULL,
                         "Internal error: missing current MIR block after lowering call arguments.");
            return false;
        }
        if (!mr_append_instruction(mr_current_block(context), instruction)) {
            free(nlr_retval_locals);
            mr_instruction_free(&instruction);
            mr_set_error(context->build, expression->source_span, NULL,
                         "Out of memory while lowering MIR calls.");
            return false;
        }

        if (has_nlr) {
            /* Emit: temp_check = call __calynda_rt_nlr_check_pop(nlr_slot) */
            MirValue nlr_slot_value;
            MirInstruction check_call;
            size_t check_temp;
            size_t nlr_block_idx, continue_block_idx;
            MirBasicBlock *cur_block;
            CheckedType ext_type = mr_nlr_external_type();

            memset(&nlr_slot_value, 0, sizeof(nlr_slot_value));
            nlr_slot_value.kind = MIR_VALUE_LOCAL;
            nlr_slot_value.type = ext_type;
            nlr_slot_value.as.local_index = nlr_slot_local;

            memset(&check_call, 0, sizeof(check_call));
            check_call.kind = MIR_INSTR_CALL;
            if (!mr_value_from_global(context->build, "__calynda_rt_nlr_check_pop",
                                      ext_type, &check_call.as.call.callee)) {
                free(nlr_retval_locals);
                return false;
            }
            check_call.as.call.has_result = true;
            check_call.as.call.dest_temp = context->unit->next_temp_index++;
            check_call.as.call.argument_count = 1;
            check_call.as.call.arguments = calloc(1, sizeof(*check_call.as.call.arguments));
            if (!check_call.as.call.arguments) {
                mr_instruction_free(&check_call);
                free(nlr_retval_locals);
                mr_set_error(context->build, expression->source_span, NULL,
                             "Out of memory while lowering NLR check call.");
                return false;
            }
            if (!mr_value_clone(context->build, &nlr_slot_value, &check_call.as.call.arguments[0])) {
                mr_instruction_free(&check_call);
                free(nlr_retval_locals);
                return false;
            }
            cur_block = mr_current_block(context);
            if (!cur_block || !mr_append_instruction(cur_block, check_call)) {
                mr_instruction_free(&check_call);
                free(nlr_retval_locals);
                mr_set_error(context->build, expression->source_span, NULL,
                             "Out of memory while lowering NLR check call.");
                return false;
            }
            check_temp = check_call.as.call.dest_temp;

            /* Create nlr_block and continue_block */
            if (!mr_create_block(context, &nlr_block_idx) ||
                !mr_create_block(context, &continue_block_idx)) {
                free(nlr_retval_locals);
                return false;
            }

            /* Branch on check_temp */
            cur_block = mr_current_block(context);
            if (!cur_block) {
                free(nlr_retval_locals);
                mr_set_error(context->build, expression->source_span, NULL,
                             "Internal error: missing block before NLR branch.");
                return false;
            }
            memset(&cur_block->terminator, 0, sizeof(cur_block->terminator));
            cur_block->terminator.kind = MIR_TERM_BRANCH;
            cur_block->terminator.as.branch_term.condition.kind = MIR_VALUE_TEMP;
            cur_block->terminator.as.branch_term.condition.type = ext_type;
            cur_block->terminator.as.branch_term.condition.as.temp_index = check_temp;
            cur_block->terminator.as.branch_term.true_block = nlr_block_idx;
            cur_block->terminator.as.branch_term.false_block = continue_block_idx;

            /* Build NLR block: get value and return */
            context->current_block_index = nlr_block_idx;
            {
                CheckedType outer_ret = context->unit->return_type;
                MirBasicBlock *nlr_block;

                if (context->is_nlr_block) {
                    /* Inside an NLR block: get the value from the inner slot and
                       propagate it to this unit's outer NLR slot (capture[0]).
                       This handles `return expr` used inside a |var lambda body,
                       e.g. forPairs(a, b, (x, y) -> { if(x != y, return false); }). */
                    MirInstruction get_val_call;
                    MirValue get_val_temp;
                    MirValue outer_slot_val;

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
                                     "Out of memory while lowering NLR propagation get_value.");
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
                                     "Out of memory while lowering NLR propagation get_value.");
                        return false;
                    }

                    memset(&get_val_temp, 0, sizeof(get_val_temp));
                    get_val_temp.kind = MIR_VALUE_TEMP;
                    get_val_temp.type = ext_type;
                    get_val_temp.as.temp_index = get_val_call.as.call.dest_temp;

                    /* capture[0] is always the outer NLR slot for NLR block units */
                    memset(&outer_slot_val, 0, sizeof(outer_slot_val));
                    outer_slot_val.kind = MIR_VALUE_LOCAL;
                    outer_slot_val.type = ext_type;
                    outer_slot_val.as.local_index = 0;

#include "mir_expr_call_p3.inc"
