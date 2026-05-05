        merge_block = &unit->blocks[merge_block_index];
        if (merge_block->instruction_count == 0 &&
            merge_block->terminator.kind == MIR_TERM_RETURN &&
            merge_block->terminator.as.return_term.has_value &&
            merge_block->terminator.as.return_term.value.kind == MIR_VALUE_LOCAL &&
            merge_block->terminator.as.return_term.value.as.local_index ==
                store_instruction->as.store_local.local_index) {
            match->kind = MIR_TCO_PATTERN_STORE_GOTO;
            match->call_instruction_index = block->instruction_count - 2;
            match->removed_instruction_count = 2;
            return true;
        }
    }

    return false;
}

static bool mt_append_tco_scratch_locals(MirUnit *unit,
                                         const size_t *parameter_locals,
                                         size_t *scratch_locals) {
    size_t i;

    if (!unit || (!parameter_locals && unit->parameter_count > 0) ||
        (!scratch_locals && unit->parameter_count > 0)) {
        return false;
    }

    for (i = 0; i < unit->parameter_count; i++) {
        MirLocal local;
        char name[32];
        int written;

        memset(&local, 0, sizeof(local));
        written = snprintf(name, sizeof(name), "__mir_tco%zu", i);
        if (written < 0 || (size_t)written >= sizeof(name)) {
            return false;
        }

        local.kind = MIR_LOCAL_SYNTHETIC;
        local.name = ast_copy_text(name);
        local.type = unit->locals[parameter_locals[i]].type;
        local.index = unit->local_count;
        if (!local.name || !mr_append_local(unit, local)) {
            free(local.name);
            return false;
        }
        scratch_locals[i] = local.index;
    }

    return true;
}

static bool mt_build_rewrite_instructions(const MirUnit *unit,
                                          const MirInstruction *call_instruction,
                                          const size_t *parameter_locals,
                                          const size_t *scratch_locals,
                                          MirInstruction *instructions) {
    size_t i;

    if (!unit || !call_instruction || (!instructions && unit->parameter_count > 0)) {
        return false;
    }

    for (i = 0; i < unit->parameter_count; i++) {
        MirInstruction *snapshot = &instructions[i];
        MirInstruction *restore = &instructions[unit->parameter_count + i];

        memset(snapshot, 0, sizeof(*snapshot));
        snapshot->kind = MIR_INSTR_STORE_LOCAL;
        snapshot->as.store_local.local_index = scratch_locals[i];
        if (!mt_clone_value(&call_instruction->as.call.arguments[i],
                            &snapshot->as.store_local.value)) {
            return false;
        }

        memset(restore, 0, sizeof(*restore));
        restore->kind = MIR_INSTR_STORE_LOCAL;
        restore->as.store_local.local_index = parameter_locals[i];
        restore->as.store_local.value.kind = MIR_VALUE_LOCAL;
        restore->as.store_local.value.type = unit->locals[scratch_locals[i]].type;
        restore->as.store_local.value.as.local_index = scratch_locals[i];
    }

    return true;
}

static void mt_free_instruction_range(MirInstruction *instructions, size_t count) {
    size_t i;

    if (!instructions) {
        return;
    }

    for (i = 0; i < count; i++) {
        mr_instruction_free(&instructions[i]);
    }
    free(instructions);
}

static bool mt_rewrite_block(MirUnit *unit,
                             size_t block_index,
                             const MirTcoMatch *match,
                             size_t target_block_index,
                             const size_t *parameter_locals,
                             const size_t *scratch_locals) {
    MirBasicBlock *block;
    MirInstruction *new_instructions;
    size_t prefix_count;
    size_t new_instruction_count;
    size_t i;

    if (!unit || !match || block_index >= unit->block_count) {
        return false;
    }

    block = &unit->blocks[block_index];
    prefix_count = match->call_instruction_index;
    new_instruction_count = prefix_count + (unit->parameter_count * 2);
    new_instructions = NULL;
    if (new_instruction_count > 0) {
        new_instructions = calloc(new_instruction_count, sizeof(*new_instructions));
        if (!new_instructions) {
            return false;
        }
    }

    for (i = 0; i < prefix_count; i++) {
        new_instructions[i] = block->instructions[i];
    }

    if (!mt_build_rewrite_instructions(unit,
                                       &block->instructions[match->call_instruction_index],
                                       parameter_locals,
                                       scratch_locals,
                                       new_instructions + prefix_count)) {
        mt_free_instruction_range(new_instructions, new_instruction_count);
        return false;
    }

    for (i = 0; i < match->removed_instruction_count; i++) {
        mr_instruction_free(&block->instructions[match->call_instruction_index + i]);
    }
    free(block->instructions);

    if (match->kind == MIR_TCO_PATTERN_RETURN &&
        block->terminator.kind == MIR_TERM_RETURN &&
        block->terminator.as.return_term.has_value) {
        mr_value_free(&block->terminator.as.return_term.value);
    }

    block->instructions = new_instructions;
    block->instruction_count = new_instruction_count;
    block->instruction_capacity = new_instruction_count;
    memset(&block->terminator, 0, sizeof(block->terminator));
    block->terminator.kind = MIR_TERM_GOTO;
    block->terminator.as.goto_term.target_block = target_block_index;
    return true;
}

void mir_apply_self_tco(MirUnit *unit) {
    size_t *parameter_locals;
    size_t *scratch_locals;
    size_t safe_target_block = 0;
    bool scratch_ready = false;
    size_t block_index;

    if (!unit || !unit->name || unit->block_count == 0 ||
        unit->kind == MIR_UNIT_INIT || unit->kind == MIR_UNIT_ASM) {
        return;
    }

    parameter_locals = NULL;
    scratch_locals = NULL;
    if (unit->parameter_count > 0) {
        parameter_locals = calloc(unit->parameter_count, sizeof(*parameter_locals));
        scratch_locals = calloc(unit->parameter_count, sizeof(*scratch_locals));
        if (!parameter_locals || !scratch_locals ||
            !mt_find_parameter_locals(unit, parameter_locals)) {
            free(parameter_locals);
            free(scratch_locals);
            return;
        }
    }

    for (block_index = 0; block_index < unit->block_count; block_index++) {
        MirTcoMatch match;

        if (!mt_match_tail_self_call(unit, &unit->blocks[block_index], &match)) {
            continue;
        }

        if (unit->parameter_count !=
            unit->blocks[block_index]
                .instructions[match.call_instruction_index]
                .as.call.argument_count) {
            continue;
        }

        if (!scratch_ready && unit->parameter_count > 0) {
            if (!mt_append_tco_scratch_locals(unit, parameter_locals, scratch_locals)) {
                break;
            }
            scratch_ready = true;
        }

        if (safe_target_block == 0) {
            if (!mt_ensure_safe_entry_target(unit, &safe_target_block)) {
                break;
            }
            /* mt_ensure_safe_entry_target may move the entry block's instructions to
             * a new block (safe_target_block).  When the matched tail call was in the
             * entry block (block_index == 0), we must rewrite the NEW block, not the
             * now-empty entry block.  Without this correction mt_rewrite_block would
             * copy zeroed MirInstructions into bb0, producing spurious
             * MIR_INSTR_BINARY / AST_BINARY_OP_LOGICAL_OR (both == 0) ghost entries. */
            if (block_index == 0 && safe_target_block != 0) {
                block_index = safe_target_block;
            }
        }

        if (!mt_rewrite_block(unit,
                              block_index,
                              &match,
                              safe_target_block,
                              parameter_locals,
                              scratch_locals)) {
            break;
        }
    }

    free(parameter_locals);
    free(scratch_locals);
}
