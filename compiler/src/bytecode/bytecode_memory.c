#include "bytecode.h"
#include "bytecode_internal.h"

#include <stdlib.h>
#include <string.h>

bool bc_reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size) {
    void *resized;
    size_t new_capacity;

    if (*capacity >= needed) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 8 : *capacity;
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

void bc_constant_free(BytecodeConstant *constant) {
    size_t i;

    if (!constant) {
        return;
    }

    if (constant->kind == BYTECODE_CONSTANT_LITERAL) {
        free(constant->as.literal.text);
    } else if (constant->kind == BYTECODE_CONSTANT_TYPE_DESCRIPTOR) {
        free(constant->as.type_descriptor.name);
        free(constant->as.type_descriptor.generic_param_tags);
        for (i = 0; i < constant->as.type_descriptor.variant_count; i++) {
            free(constant->as.type_descriptor.variant_names[i]);
        }
        free(constant->as.type_descriptor.variant_names);
        free(constant->as.type_descriptor.variant_payload_tags);
    } else {
        free(constant->as.text);
    }
    memset(constant, 0, sizeof(*constant));
}

static size_t bc_intern_mir_type_descriptor(BytecodeBuildContext *context,
                                            const CalyndaRtTypeDescriptor *type_desc) {
    return bc_intern_union_type_descriptor(context,
                                           type_desc->name,
                                           type_desc->generic_param_count,
                                           type_desc->generic_param_tags,
                                           type_desc->variant_names,
                                           type_desc->variant_payload_tags,
                                           type_desc->variant_count);
}

void bc_template_part_free(BytecodeTemplatePart *part) {
    if (!part) {
        return;
    }

    memset(part, 0, sizeof(*part));
}

void bc_instruction_free(BytecodeInstruction *instruction) {
    size_t i;

    if (!instruction) {
        return;
    }

    switch (instruction->kind) {
    case BYTECODE_INSTR_CLOSURE:
        free(instruction->as.closure.captures);
        break;
    case BYTECODE_INSTR_CALL:
        free(instruction->as.call.arguments);
        break;
    case BYTECODE_INSTR_ARRAY_LITERAL:
        free(instruction->as.array_literal.elements);
        break;
    case BYTECODE_INSTR_TEMPLATE:
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            bc_template_part_free(&instruction->as.template_literal.parts[i]);
        }
        free(instruction->as.template_literal.parts);
        break;
    case BYTECODE_INSTR_HETERO_ARRAY_NEW:
        free(instruction->as.hetero_array_new.elements);
        break;
    case BYTECODE_INSTR_BINARY:
    case BYTECODE_INSTR_UNARY:
    case BYTECODE_INSTR_CAST:
    case BYTECODE_INSTR_MEMBER:
    case BYTECODE_INSTR_INDEX_LOAD:
    case BYTECODE_INSTR_STORE_LOCAL:
    case BYTECODE_INSTR_STORE_GLOBAL:
    case BYTECODE_INSTR_STORE_INDEX:
    case BYTECODE_INSTR_STORE_MEMBER:
    case BYTECODE_INSTR_UNION_NEW:
    case BYTECODE_INSTR_UNION_GET_TAG:
    case BYTECODE_INSTR_UNION_GET_PAYLOAD:
    case BYTECODE_INSTR_HETERO_ARRAY_GET_TAG:
    case BYTECODE_INSTR_TYPEOF:
    case BYTECODE_INSTR_ISINT:
    case BYTECODE_INSTR_ISFLOAT:
    case BYTECODE_INSTR_ISBOOL:
    case BYTECODE_INSTR_ISSTRING:
    case BYTECODE_INSTR_ISARRAY:
    case BYTECODE_INSTR_ISSAMETYPE:
        break;
    }

    memset(instruction, 0, sizeof(*instruction));
}

bool bc_lower_hetero_array_instruction(BytecodeBuildContext *context,
                                       const MirInstruction *instruction,
                                       BytecodeInstruction *lowered) {
    size_t i;

    lowered->kind = BYTECODE_INSTR_HETERO_ARRAY_NEW;
    lowered->as.hetero_array_new.dest_temp = instruction->as.hetero_array_new.dest_temp;
    lowered->as.hetero_array_new.element_count = instruction->as.hetero_array_new.element_count;
    if (lowered->as.hetero_array_new.element_count > 0) {
        lowered->as.hetero_array_new.elements = calloc(
            lowered->as.hetero_array_new.element_count,
            sizeof(*lowered->as.hetero_array_new.elements));
        if (!lowered->as.hetero_array_new.elements) {
            return false;
        }
    }

    for (i = 0; i < lowered->as.hetero_array_new.element_count; i++) {
        if (!bc_value_from_mir_value(context,
                                     instruction->as.hetero_array_new.elements[i],
                                     &lowered->as.hetero_array_new.elements[i])) {
            return false;
        }
    }

    lowered->as.hetero_array_new.type_desc_index =
        bc_intern_mir_type_descriptor(context,
                                      &instruction->as.hetero_array_new.type_desc);
    return lowered->as.hetero_array_new.type_desc_index != (size_t)-1;
}

bool bc_lower_union_new_instruction(BytecodeBuildContext *context,
                                    const MirInstruction *instruction,
                                    BytecodeInstruction *lowered) {
    size_t zero_index;

    lowered->kind = BYTECODE_INSTR_UNION_NEW;
    lowered->as.union_new.dest_temp = instruction->as.union_new.dest_temp;
    lowered->as.union_new.type_desc_index =
        bc_intern_mir_type_descriptor(context, &instruction->as.union_new.type_desc);
    if (lowered->as.union_new.type_desc_index == (size_t)-1) {
        return false;
    }

    lowered->as.union_new.variant_tag = (uint32_t)instruction->as.union_new.variant_index;
    if (instruction->as.union_new.has_payload) {
        return bc_value_from_mir_value(context,
                                       instruction->as.union_new.payload,
                                       &lowered->as.union_new.payload);
    }

    zero_index = bc_intern_literal_constant(context, AST_LITERAL_INTEGER, "0", false);
    if (zero_index == (size_t)-1) {
        return false;
    }
    lowered->as.union_new.payload.kind = BYTECODE_VALUE_CONSTANT;
    lowered->as.union_new.payload.as.constant_index = zero_index;
    return true;
}

void bc_block_free(BytecodeBasicBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    free(block->label);
    for (i = 0; i < block->instruction_count; i++) {
        bc_instruction_free(&block->instructions[i]);
    }
    free(block->instructions);
    memset(block, 0, sizeof(*block));
}

void bc_unit_free(BytecodeUnit *unit) {
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
        bc_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    memset(unit, 0, sizeof(*unit));
}
