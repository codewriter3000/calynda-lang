#include "lir.h"
#include "lir_internal.h"

#include <stdlib.h>
#include <string.h>

bool lr_reserve_items(void **items, size_t *capacity,
                      size_t needed, size_t item_size) {
    void *resized;
    size_t new_capacity;

    if (*capacity >= needed) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 4 : *capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }

    resized = realloc(*items, new_capacity * item_size);
    if (!resized) {
        return false;
    }

    *items = resized;
    *capacity = new_capacity;
    return true;
}

void lr_operand_free(LirOperand *operand) {
    if (!operand) {
        return;
    }

    if (operand->kind == LIR_OPERAND_GLOBAL) {
        free(operand->as.global_name);
    } else if (operand->kind == LIR_OPERAND_LITERAL &&
               operand->as.literal.kind != AST_LITERAL_BOOL &&
               operand->as.literal.kind != AST_LITERAL_NULL) {
        free(operand->as.literal.text);
    }

    memset(operand, 0, sizeof(*operand));
}

void lr_template_part_free(LirTemplatePart *part) {
    if (!part) {
        return;
    }

    if (part->kind == LIR_TEMPLATE_PART_TEXT) {
        free(part->as.text);
    } else {
        lr_operand_free(&part->as.value);
    }

    memset(part, 0, sizeof(*part));
}

void lr_instruction_free(LirInstruction *instruction) {
    size_t i;

    if (!instruction) {
        return;
    }

    switch (instruction->kind) {
    case LIR_INSTR_INCOMING_ARG:
    case LIR_INSTR_INCOMING_CAPTURE:
        break;
    case LIR_INSTR_OUTGOING_ARG:
        lr_operand_free(&instruction->as.outgoing_arg.value);
        break;
    case LIR_INSTR_BINARY:
        lr_operand_free(&instruction->as.binary.left);
        lr_operand_free(&instruction->as.binary.right);
        break;
    case LIR_INSTR_UNARY:
        lr_operand_free(&instruction->as.unary.operand);
        break;
    case LIR_INSTR_CLOSURE:
        free(instruction->as.closure.unit_name);
        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            lr_operand_free(&instruction->as.closure.captures[i]);
        }
        free(instruction->as.closure.captures);
        break;
    case LIR_INSTR_CALL:
        lr_operand_free(&instruction->as.call.callee);
        break;
    case LIR_INSTR_CAST:
        lr_operand_free(&instruction->as.cast.operand);
        break;
    case LIR_INSTR_MEMBER:
        lr_operand_free(&instruction->as.member.target);
        free(instruction->as.member.member);
        break;
    case LIR_INSTR_INDEX_LOAD:
        lr_operand_free(&instruction->as.index_load.target);
        lr_operand_free(&instruction->as.index_load.index);
        break;
    case LIR_INSTR_ARRAY_LITERAL:
        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            lr_operand_free(&instruction->as.array_literal.elements[i]);
        }
        free(instruction->as.array_literal.elements);
        break;
    case LIR_INSTR_TEMPLATE:
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            lr_template_part_free(&instruction->as.template_literal.parts[i]);
        }
        free(instruction->as.template_literal.parts);
        break;
    case LIR_INSTR_STORE_SLOT:
        lr_operand_free(&instruction->as.store_slot.value);
        break;
    case LIR_INSTR_STORE_GLOBAL:
        free(instruction->as.store_global.global_name);
        lr_operand_free(&instruction->as.store_global.value);
        break;
    case LIR_INSTR_STORE_INDEX:
        lr_operand_free(&instruction->as.store_index.target);
        lr_operand_free(&instruction->as.store_index.index);
        lr_operand_free(&instruction->as.store_index.value);
        break;
    case LIR_INSTR_STORE_MEMBER:
        lr_operand_free(&instruction->as.store_member.target);
        free(instruction->as.store_member.member);
        lr_operand_free(&instruction->as.store_member.value);
        break;
    case LIR_INSTR_HETERO_ARRAY_NEW:
        for (i = 0; i < instruction->as.hetero_array_new.element_count; i++) {
            lr_operand_free(&instruction->as.hetero_array_new.elements[i]);
        }
        free(instruction->as.hetero_array_new.elements);
        free(instruction->as.hetero_array_new.element_types);
        break;
    case LIR_INSTR_UNION_NEW:
        free(instruction->as.union_new.union_name);
        if (instruction->as.union_new.has_payload) {
            lr_operand_free(&instruction->as.union_new.payload);
        }
        break;
    }

    memset(instruction, 0, sizeof(*instruction));
}

void lr_basic_block_free(LirBasicBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    free(block->label);
    for (i = 0; i < block->instruction_count; i++) {
        lr_instruction_free(&block->instructions[i]);
    }
    free(block->instructions);
    switch (block->terminator.kind) {
    case LIR_TERM_RETURN:
        if (block->terminator.as.return_term.has_value) {
            lr_operand_free(&block->terminator.as.return_term.value);
        }
        break;
    case LIR_TERM_BRANCH:
        lr_operand_free(&block->terminator.as.branch_term.condition);
        break;
    case LIR_TERM_THROW:
        lr_operand_free(&block->terminator.as.throw_term.value);
        break;
    case LIR_TERM_NONE:
    case LIR_TERM_JUMP:
        break;
    }
    memset(block, 0, sizeof(*block));
}

void lr_unit_free(LirUnit *unit) {
    size_t i;

    if (!unit) {
        return;
    }

    free(unit->name);
    for (i = 0; i < unit->slot_count; i++) {
        free(unit->slots[i].name);
    }
    free(unit->slots);
    for (i = 0; i < unit->block_count; i++) {
        lr_basic_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    free(unit->asm_body);
    memset(unit, 0, sizeof(*unit));
}
