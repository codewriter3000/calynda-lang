#include "mir_internal.h"

void mr_instruction_free(MirInstruction *instruction) {
    size_t i;

    if (!instruction) {
        return;
    }

    switch (instruction->kind) {
    case MIR_INSTR_BINARY:
        mr_value_free(&instruction->as.binary.left);
        mr_value_free(&instruction->as.binary.right);
        break;
    case MIR_INSTR_UNARY:
        mr_value_free(&instruction->as.unary.operand);
        break;
    case MIR_INSTR_CLOSURE:
        free(instruction->as.closure.unit_name);
        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            mr_value_free(&instruction->as.closure.captures[i]);
        }
        free(instruction->as.closure.captures);
        break;
    case MIR_INSTR_CALL:
        mr_value_free(&instruction->as.call.callee);
        for (i = 0; i < instruction->as.call.argument_count; i++) {
            mr_value_free(&instruction->as.call.arguments[i]);
        }
        free(instruction->as.call.arguments);
        break;
    case MIR_INSTR_CAST:
        mr_value_free(&instruction->as.cast.operand);
        break;
    case MIR_INSTR_MEMBER:
        mr_value_free(&instruction->as.member.target);
        free(instruction->as.member.member);
        break;
    case MIR_INSTR_INDEX_LOAD:
        mr_value_free(&instruction->as.index_load.target);
        mr_value_free(&instruction->as.index_load.index);
        break;
    case MIR_INSTR_ARRAY_LITERAL:
        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            mr_value_free(&instruction->as.array_literal.elements[i]);
        }
        free(instruction->as.array_literal.elements);
        break;
    case MIR_INSTR_TEMPLATE:
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            mr_template_part_free(&instruction->as.template_literal.parts[i]);
        }
        free(instruction->as.template_literal.parts);
        break;
    case MIR_INSTR_STORE_LOCAL:
        mr_value_free(&instruction->as.store_local.value);
        break;
    case MIR_INSTR_STORE_GLOBAL:
        free(instruction->as.store_global.global_name);
        mr_value_free(&instruction->as.store_global.value);
        break;
    case MIR_INSTR_STORE_INDEX:
        mr_value_free(&instruction->as.store_index.target);
        mr_value_free(&instruction->as.store_index.index);
        mr_value_free(&instruction->as.store_index.value);
        break;
    case MIR_INSTR_STORE_MEMBER:
        mr_value_free(&instruction->as.store_member.target);
        free(instruction->as.store_member.member);
        mr_value_free(&instruction->as.store_member.value);
        break;
    case MIR_INSTR_HETERO_ARRAY_NEW:
        for (i = 0; i < instruction->as.hetero_array_new.element_count; i++) {
            mr_value_free(&instruction->as.hetero_array_new.elements[i]);
        }
        free(instruction->as.hetero_array_new.elements);
        free(instruction->as.hetero_array_new.element_types);
        break;
    case MIR_INSTR_UNION_NEW:
        free(instruction->as.union_new.union_name);
        if (instruction->as.union_new.has_payload) {
            mr_value_free(&instruction->as.union_new.payload);
        }
        break;
    }

    memset(instruction, 0, sizeof(*instruction));
}

void mr_basic_block_free(MirBasicBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    free(block->label);
    for (i = 0; i < block->instruction_count; i++) {
        mr_instruction_free(&block->instructions[i]);
    }
    free(block->instructions);
    switch (block->terminator.kind) {
    case MIR_TERM_RETURN:
        if (block->terminator.as.return_term.has_value) {
            mr_value_free(&block->terminator.as.return_term.value);
        }
        break;
    case MIR_TERM_BRANCH:
        mr_value_free(&block->terminator.as.branch_term.condition);
        break;
    case MIR_TERM_THROW:
        mr_value_free(&block->terminator.as.throw_term.value);
        break;
    case MIR_TERM_NONE:
    case MIR_TERM_GOTO:
        break;
    }
    memset(block, 0, sizeof(*block));
}

void mr_unit_free(MirUnit *unit) {
    size_t i;

    if (!unit) {
        return;
    }

    free(unit->name);
    for (i = 0; i < unit->local_count; i++) {
        free(unit->locals[i].name);
    }
    free(unit->locals);
    for (i = 0; i < unit->block_count; i++) {
        mr_basic_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    free(unit->asm_body);
    memset(unit, 0, sizeof(*unit));
}
