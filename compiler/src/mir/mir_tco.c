#include "mir_internal.h"

typedef enum {
    MIR_TCO_PATTERN_NONE = 0,
    MIR_TCO_PATTERN_RETURN,
    MIR_TCO_PATTERN_STORE_GOTO
} MirTcoPatternKind;

typedef struct {
    MirTcoPatternKind kind;
    size_t            call_instruction_index;
    size_t            removed_instruction_count;
} MirTcoMatch;

static bool mt_clone_value(const MirValue *source, MirValue *clone) {
    if (!source || !clone) {
        return false;
    }

    *clone = mr_invalid_value();
    clone->kind = source->kind;
    clone->type = source->type;
    switch (source->kind) {
    case MIR_VALUE_INVALID:
        return true;
    case MIR_VALUE_TEMP:
        clone->as.temp_index = source->as.temp_index;
        return true;
    case MIR_VALUE_LOCAL:
        clone->as.local_index = source->as.local_index;
        return true;
    case MIR_VALUE_GLOBAL:
        clone->as.global_name = ast_copy_text(source->as.global_name);
        return clone->as.global_name != NULL;
    case MIR_VALUE_LITERAL:
        clone->as.literal.kind = source->as.literal.kind;
        clone->as.literal.bool_value = source->as.literal.bool_value;
        if (source->as.literal.kind == AST_LITERAL_BOOL ||
            source->as.literal.kind == AST_LITERAL_NULL) {
            return true;
        }
        clone->as.literal.text = ast_copy_text(source->as.literal.text);
        return clone->as.literal.text != NULL;
    }

    return false;
}

static bool mt_is_self_call(const MirUnit *unit, const MirInstruction *instruction) {
    return unit &&
           unit->name &&
           instruction &&
           instruction->kind == MIR_INSTR_CALL &&
           instruction->as.call.callee.kind == MIR_VALUE_GLOBAL &&
           instruction->as.call.callee.as.global_name &&
           strcmp(instruction->as.call.callee.as.global_name, unit->name) == 0;
}

static bool mt_is_module_init_call(const MirInstruction *instruction) {
    return instruction &&
           instruction->kind == MIR_INSTR_CALL &&
           instruction->as.call.callee.kind == MIR_VALUE_GLOBAL &&
           instruction->as.call.callee.as.global_name &&
           strcmp(instruction->as.call.callee.as.global_name, MIR_MODULE_INIT_NAME) == 0;
}

static bool mt_find_parameter_locals(const MirUnit *unit, size_t *parameter_locals) {
    size_t parameter_index = 0;
    size_t i;

    if (!unit || (!parameter_locals && unit->parameter_count > 0)) {
        return false;
    }

    for (i = 0; i < unit->local_count; i++) {
        if (unit->locals[i].kind != MIR_LOCAL_PARAMETER) {
            continue;
        }
        if (parameter_index >= unit->parameter_count) {
            return false;
        }
        parameter_locals[parameter_index++] = i;
    }

    return parameter_index == unit->parameter_count;
}

static bool mt_append_block(MirUnit *unit, MirBasicBlock *block, size_t *block_index) {
    char label[32];
    size_t index;
    int written;

    if (!unit || !block || !block_index) {
        return false;
    }

    index = unit->block_count;
    written = snprintf(label, sizeof(label), "bb%zu", index);
    if (written < 0 || (size_t)written >= sizeof(label)) {
        return false;
    }

    memset(block, 0, sizeof(*block));
    block->label = ast_copy_text(label);
    if (!block->label) {
        return false;
    }

    if (!mr_append_block(unit, *block)) {
        free(block->label);
        memset(block, 0, sizeof(*block));
        return false;
    }

    *block_index = index;
    return true;
}

static bool mt_ensure_safe_entry_target(MirUnit *unit, size_t *safe_target_block) {
    MirBasicBlock body_block;
    MirBasicBlock *entry_block;
    size_t split_after = 0;
    size_t remaining_count;
    size_t body_block_index;
    size_t i;

    if (!unit || !safe_target_block || unit->block_count == 0) {
        return false;
    }

    *safe_target_block = 0;
    entry_block = &unit->blocks[0];
    if (unit->kind != MIR_UNIT_START) {
        if (entry_block->instruction_count == 0 &&
            entry_block->terminator.kind == MIR_TERM_GOTO &&
            entry_block->terminator.as.goto_term.target_block != 0) {
            *safe_target_block = entry_block->terminator.as.goto_term.target_block;
            return true;
        }
    } else if (entry_block->instruction_count > 0 &&
               mt_is_module_init_call(&entry_block->instructions[0])) {
        split_after = 1;
        if (entry_block->terminator.kind == MIR_TERM_GOTO &&
            entry_block->instruction_count == 1 &&
            entry_block->terminator.as.goto_term.target_block != 0) {
            *safe_target_block = entry_block->terminator.as.goto_term.target_block;
            return true;
        }
    } else if (entry_block->instruction_count == 0 &&
               entry_block->terminator.kind == MIR_TERM_GOTO &&
               entry_block->terminator.as.goto_term.target_block != 0) {
        *safe_target_block = entry_block->terminator.as.goto_term.target_block;
        return true;
    }

    if (!mt_append_block(unit, &body_block, &body_block_index)) {
        return false;
    }

    entry_block = &unit->blocks[0];

    remaining_count = entry_block->instruction_count - split_after;
    if (remaining_count > 0) {
        body_block.instructions = calloc(remaining_count, sizeof(*body_block.instructions));
        if (!body_block.instructions) {
            mr_basic_block_free(&unit->blocks[body_block_index]);
            unit->block_count--;
            return false;
        }
    }

    for (i = 0; i < remaining_count; i++) {
        body_block.instructions[i] = entry_block->instructions[i + split_after];
        memset(&entry_block->instructions[i + split_after], 0, sizeof(*entry_block->instructions));
    }
    body_block.instruction_count = remaining_count;
    body_block.instruction_capacity = remaining_count;
    body_block.terminator = entry_block->terminator;

    unit->blocks[body_block_index] = body_block;

    entry_block->instruction_count = split_after;
    entry_block->terminator.kind = MIR_TERM_GOTO;
    entry_block->terminator.as.goto_term.target_block = body_block_index;
    *safe_target_block = body_block_index;
    return true;
}

static bool mt_match_tail_self_call(const MirUnit *unit,
                                    const MirBasicBlock *block,
                                    MirTcoMatch *match) {
    const MirInstruction *call_instruction;

    if (!unit || !block || !match || block->instruction_count == 0) {
        return false;
    }

    memset(match, 0, sizeof(*match));

    call_instruction = &block->instructions[block->instruction_count - 1];
    if (mt_is_self_call(unit, call_instruction) &&
        block->terminator.kind == MIR_TERM_RETURN) {
        if (!call_instruction->as.call.has_result &&
            !block->terminator.as.return_term.has_value) {
            match->kind = MIR_TCO_PATTERN_RETURN;
            match->call_instruction_index = block->instruction_count - 1;
            match->removed_instruction_count = 1;
            return true;
        }
        if (call_instruction->as.call.has_result &&
            block->terminator.as.return_term.has_value &&
            block->terminator.as.return_term.value.kind == MIR_VALUE_TEMP &&
            block->terminator.as.return_term.value.as.temp_index ==
                call_instruction->as.call.dest_temp) {
            match->kind = MIR_TCO_PATTERN_RETURN;
            match->call_instruction_index = block->instruction_count - 1;
            match->removed_instruction_count = 1;
            return true;
        }
    }

    if (block->instruction_count >= 2 &&
        block->terminator.kind == MIR_TERM_GOTO) {
        const MirInstruction *store_instruction =
            &block->instructions[block->instruction_count - 1];
        size_t merge_block_index = block->terminator.as.goto_term.target_block;
        const MirBasicBlock *merge_block;

        call_instruction = &block->instructions[block->instruction_count - 2];
        if (!mt_is_self_call(unit, call_instruction) ||
            !call_instruction->as.call.has_result ||
            store_instruction->kind != MIR_INSTR_STORE_LOCAL ||
            store_instruction->as.store_local.value.kind != MIR_VALUE_TEMP ||
            store_instruction->as.store_local.value.as.temp_index !=
                call_instruction->as.call.dest_temp ||
            merge_block_index >= unit->block_count) {
            return false;
        }

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
