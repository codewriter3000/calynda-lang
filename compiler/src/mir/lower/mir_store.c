#include "mir_internal.h"

bool mr_append_call_global_instruction(MirUnitBuildContext *context,
                                       const char *global_name,
                                       CheckedType return_type,
                                       AstSourceSpan source_span) {
    MirInstruction instruction;
    MirBasicBlock *block;
    block = mr_current_block(context);
    if (!block) {
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering call.");
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_CALL;
    if (!mr_value_from_global(context->build,
                               global_name,
                               return_type,
                               &instruction.as.call.callee)) {
        return false;
    }
    if (!mr_append_instruction(block, instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR calls.");
        return false;
    }

    return true;
}

bool mr_append_store_local_instruction(MirUnitBuildContext *context,
                                       size_t local_index,
                                       MirValue value,
                                       AstSourceSpan source_span) {
    MirInstruction instruction;
    MirBasicBlock *block;
    block = mr_current_block(context);
    if (!block) {
        mr_value_free(&value);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering store.");
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_STORE_LOCAL;
    instruction.as.store_local.local_index = local_index;
    instruction.as.store_local.value = value;
    if (!mr_append_instruction(block, instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR stores.");
        return false;
    }

    return true;
}

bool mr_append_store_global_instruction(MirUnitBuildContext *context,
                                        const char *global_name,
                                        MirValue value,
                                        AstSourceSpan source_span) {
    MirInstruction instruction;
    MirBasicBlock *block;
    block = mr_current_block(context);
    if (!block) {
        mr_value_free(&value);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering global store.");
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_STORE_GLOBAL;
    instruction.as.store_global.global_name = ast_copy_text(global_name);
    instruction.as.store_global.value = value;
    if (!instruction.as.store_global.global_name) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR global stores.");
        return false;
    }
    if (!mr_append_instruction(block, instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR global stores.");
        return false;
    }

    return true;
}

bool mr_append_store_index_instruction(MirUnitBuildContext *context,
                                       const MirValue *target,
                                       const MirValue *index,
                                       MirValue value,
                                       AstSourceSpan source_span) {
    MirInstruction instruction;
    MirBasicBlock *block;

    block = mr_current_block(context);
    if (!block) {
        mr_value_free(&value);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering index store.");
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_STORE_INDEX;
    if (!mr_value_clone(context->build, target, &instruction.as.store_index.target) ||
        !mr_value_clone(context->build, index, &instruction.as.store_index.index)) {
        instruction.as.store_index.value = value;
        mr_instruction_free(&instruction);
        return false;
    }
    instruction.as.store_index.value = value;
    if (!mr_append_instruction(block, instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR index stores.");
        return false;
    }

    return true;
}

bool mr_append_store_member_instruction(MirUnitBuildContext *context,
                                        const MirValue *target,
                                        const char *member,
                                        MirValue value,
                                        AstSourceSpan source_span) {
    MirInstruction instruction;
    MirBasicBlock *block;

    block = mr_current_block(context);
    if (!block) {
        mr_value_free(&value);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Internal error: missing current MIR block while lowering member store.");
        return false;
    }

    memset(&instruction, 0, sizeof(instruction));
    instruction.kind = MIR_INSTR_STORE_MEMBER;
    if (!mr_value_clone(context->build, target, &instruction.as.store_member.target)) {
        instruction.as.store_member.value = value;
        mr_instruction_free(&instruction);
        return false;
    }
    instruction.as.store_member.member = ast_copy_text(member);
    instruction.as.store_member.value = value;
    if (!instruction.as.store_member.member) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR member stores.");
        return false;
    }
    if (!mr_append_instruction(block, instruction)) {
        mr_instruction_free(&instruction);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR member stores.");
        return false;
    }

    return true;
}

bool mr_append_synthetic_local(MirUnitBuildContext *context,
                               const char *name_prefix,
                               CheckedType type,
                               AstSourceSpan source_span,
                               size_t *local_index) {
    MirLocal local;
    char name[48];
    int written;

    if (!context || !context->unit || !local_index || !name_prefix) {
        return false;
    }

    written = snprintf(name,
                       sizeof(name),
                       "__mir_%s%zu",
                       name_prefix,
                       context->next_synthetic_local_index++);
    if (written < 0 || (size_t)written >= sizeof(name)) {
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Failed to name MIR synthetic local.");
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.kind = MIR_LOCAL_SYNTHETIC;
    local.name = ast_copy_text(name);
    local.type = type;
    local.index = context->unit->local_count;
    if (!local.name || !mr_append_local(context->unit, local)) {
        free(local.name);
        mr_set_error(context->build,
                      source_span,
                      NULL,
                      "Out of memory while lowering MIR synthetic locals.");
        return false;
    }

    *local_index = local.index;
    return true;
}

void mr_set_goto_terminator(MirUnitBuildContext *context, size_t target_block_index) {
    MirBasicBlock *block;

    block = mr_current_block(context);
    if (!block || block->terminator.kind != MIR_TERM_NONE) {
        return;
    }

    block->terminator.kind = MIR_TERM_GOTO;
    block->terminator.as.goto_term.target_block = target_block_index;
}
