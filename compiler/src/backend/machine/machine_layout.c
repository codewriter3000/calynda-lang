#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

bool mc_unit_uses_register(const CodegenUnit *unit, CodegenRegister reg) {
    size_t i;

    if (!unit) {
        return false;
    }

    for (i = 0; i < unit->vreg_count; i++) {
        if (unit->vreg_allocations[i].location.kind == CODEGEN_VREG_REGISTER &&
            unit->vreg_allocations[i].location.as.reg == reg) {
            return true;
        }
    }

    return false;
}

size_t mc_compute_helper_slot_count(const LirUnit *lir_unit,
                                    const CodegenUnit *codegen_unit) {
    size_t block_index;
    size_t max_slots = 0;

    if (!lir_unit || !codegen_unit) {
        return 0;
    }

    for (block_index = 0; block_index < lir_unit->block_count; block_index++) {
        size_t instruction_index;

        for (instruction_index = 0;
             instruction_index < lir_unit->blocks[block_index].instruction_count;
             instruction_index++) {
            const LirInstruction *instruction = &lir_unit->blocks[block_index].instructions[instruction_index];
            const CodegenSelectedInstruction *selected = &codegen_unit->blocks[block_index].instructions[instruction_index];
            size_t needed = 0;

            if (selected->selection.kind == CODEGEN_SELECTION_RUNTIME) {
                switch (selected->selection.as.runtime_helper) {
                case CODEGEN_RUNTIME_CLOSURE_NEW:
                    needed = instruction->as.closure.capture_count;
                    break;
                case CODEGEN_RUNTIME_CALL_CALLABLE:
                    needed = instruction->as.call.argument_count;
                    break;
                case CODEGEN_RUNTIME_ARRAY_LITERAL:
                    needed = instruction->as.array_literal.element_count;
                    break;
                case CODEGEN_RUNTIME_HETERO_ARRAY_NEW:
                    needed = instruction->as.hetero_array_new.element_count;
                    break;
                case CODEGEN_RUNTIME_TEMPLATE_BUILD:
                    needed = instruction->as.template_literal.part_count * 2;
                    break;
                case CODEGEN_RUNTIME_MEMBER_LOAD:
                case CODEGEN_RUNTIME_INDEX_LOAD:
                case CODEGEN_RUNTIME_STORE_INDEX:
                case CODEGEN_RUNTIME_STORE_MEMBER:
                case CODEGEN_RUNTIME_THROW:
                case CODEGEN_RUNTIME_CAST_VALUE:
                case CODEGEN_RUNTIME_UNION_NEW:
                case CODEGEN_RUNTIME_UNION_GET_TAG:
                case CODEGEN_RUNTIME_UNION_GET_PAYLOAD:
                case CODEGEN_RUNTIME_HETERO_ARRAY_GET_TAG:
                case CODEGEN_RUNTIME_THREAD_SPAWN:
                case CODEGEN_RUNTIME_THREAD_JOIN:
                case CODEGEN_RUNTIME_THREAD_CANCEL:
                case CODEGEN_RUNTIME_MUTEX_NEW:
                case CODEGEN_RUNTIME_MUTEX_LOCK:
                case CODEGEN_RUNTIME_MUTEX_UNLOCK:
                case CODEGEN_RUNTIME_FUTURE_SPAWN:
                case CODEGEN_RUNTIME_FUTURE_GET:
                case CODEGEN_RUNTIME_FUTURE_CANCEL:
                case CODEGEN_RUNTIME_ATOMIC_NEW:
                case CODEGEN_RUNTIME_ATOMIC_LOAD:
                case CODEGEN_RUNTIME_ATOMIC_STORE:
                case CODEGEN_RUNTIME_ATOMIC_EXCHANGE:
                case CODEGEN_RUNTIME_TYPEOF:
                case CODEGEN_RUNTIME_ISINT:
                case CODEGEN_RUNTIME_ISFLOAT:
                case CODEGEN_RUNTIME_ISBOOL:
                case CODEGEN_RUNTIME_ISSTRING:
                case CODEGEN_RUNTIME_ISARRAY:
                case CODEGEN_RUNTIME_ISSAMETYPE:
                    needed = 0;
                    break;
                }
            }
            if (needed > max_slots) {
                max_slots = needed;
            }
        }
    }

    return max_slots;
}

size_t mc_compute_outgoing_stack_slot_count(const LirUnit *lir_unit,
                                            const CodegenUnit *codegen_unit,
                                            const TargetDescriptor *target) {
    size_t block_index;
    size_t max_slots = 0;
    size_t max_reg_args = target ? target->arg_register_count : 6;

    if (!lir_unit || !codegen_unit) {
        return 0;
    }

    for (block_index = 0; block_index < lir_unit->block_count; block_index++) {
        size_t instruction_index;

        for (instruction_index = 0;
             instruction_index < lir_unit->blocks[block_index].instruction_count;
             instruction_index++) {
            const LirInstruction *instruction = &lir_unit->blocks[block_index].instructions[instruction_index];
            const CodegenSelectedInstruction *selected = &codegen_unit->blocks[block_index].instructions[instruction_index];

            if (instruction->kind == LIR_INSTR_CALL &&
                selected->selection.kind == CODEGEN_SELECTION_DIRECT &&
                selected->selection.as.direct_pattern == CODEGEN_DIRECT_CALL_GLOBAL &&
                instruction->as.call.argument_count > max_reg_args) {
                size_t needed = instruction->as.call.argument_count - max_reg_args;
                if (needed > max_slots) {
                    max_slots = needed;
                }
            }
        }
    }

    return max_slots;
}

size_t mc_unit_stack_word_count(const MachineUnit *unit) {
    return unit->frame_slot_count + unit->spill_slot_count + unit->helper_slot_count +
           unit->outgoing_stack_slot_count;
}

bool mc_format_slot_operand(const CodegenUnit *codegen_unit,
                            size_t slot_index,
                            char **text) {
    if (!codegen_unit || !text || slot_index >= codegen_unit->frame_slot_count) {
        return false;
    }

    *text = mc_copy_format("frame(%s)", codegen_unit->frame_slots[slot_index].name);
    return *text != NULL;
}

bool mc_format_vreg_operand(const TargetDescriptor *target,
                            const CodegenUnit *codegen_unit,
                            size_t vreg_index,
                            char **text) {
    const CodegenVRegAllocation *allocation;

    if (!codegen_unit || !text || vreg_index >= codegen_unit->vreg_count) {
        return false;
    }

    allocation = &codegen_unit->vreg_allocations[vreg_index];
    if (allocation->location.kind == CODEGEN_VREG_REGISTER) {
        *text = ast_copy_text(target_register_name(target, allocation->location.as.reg));
    } else {
        *text = mc_copy_format("spill(%zu)", allocation->location.as.spill_slot_index);
    }
    return *text != NULL;
}

/* Normalize alias primitive types (int, long, etc.) to their canonical form
 * (int32, int64, etc.) so the asm emit backend recognises the literal tag. */
static AstPrimitiveType mc_canonical_primitive(AstPrimitiveType p) {
    switch (p) {
    case AST_PRIMITIVE_BYTE:   return AST_PRIMITIVE_UINT8;
    case AST_PRIMITIVE_SBYTE:  return AST_PRIMITIVE_INT8;
    case AST_PRIMITIVE_SHORT:  return AST_PRIMITIVE_INT16;
    case AST_PRIMITIVE_INT:    return AST_PRIMITIVE_INT32;
    case AST_PRIMITIVE_UINT:   return AST_PRIMITIVE_UINT32;
    case AST_PRIMITIVE_LONG:   return AST_PRIMITIVE_INT64;
    case AST_PRIMITIVE_ULONG:  return AST_PRIMITIVE_UINT64;
    case AST_PRIMITIVE_FLOAT:  return AST_PRIMITIVE_FLOAT32;
    case AST_PRIMITIVE_DOUBLE: return AST_PRIMITIVE_FLOAT64;
    default:                   return p;
    }
}

bool mc_format_literal_operand(LirOperand operand, char **text) {
    char type_name[64];
    CheckedType canonical_type;

    if (!text || operand.kind != LIR_OPERAND_LITERAL) {
        return false;
    }

    switch (operand.as.literal.kind) {
    case AST_LITERAL_BOOL:
        *text = mc_copy_format("bool(%s)", operand.as.literal.bool_value ? "true" : "false");
        return *text != NULL;
    case AST_LITERAL_NULL:
        *text = ast_copy_text("null");
        return *text != NULL;
    default:
        canonical_type = operand.type;
        if (canonical_type.kind == CHECKED_TYPE_VALUE) {
            canonical_type.primitive = mc_canonical_primitive(canonical_type.primitive);
        }
        if (!checked_type_to_string(canonical_type, type_name, sizeof(type_name))) {
            return false;
        }
        *text = mc_copy_format("%s(%s)", type_name, operand.as.literal.text ? operand.as.literal.text : "\"\"");
        if (operand.as.literal.kind == AST_LITERAL_INTEGER || operand.as.literal.kind == AST_LITERAL_FLOAT) {
            free(*text);
            *text = mc_copy_format("%s(%s)", type_name, operand.as.literal.text ? operand.as.literal.text : "0");
        }
        return *text != NULL;
    }
}

bool mc_format_operand(const TargetDescriptor *target,
                       const LirUnit *lir_unit,
                       const CodegenUnit *codegen_unit,
                       LirOperand operand,
                       char **text) {
    (void)lir_unit;

    if (!text) {
        return false;
    }

    switch (operand.kind) {
    case LIR_OPERAND_INVALID:
        *text = ast_copy_text("<invalid>");
        return *text != NULL;
    case LIR_OPERAND_VREG:
        return mc_format_vreg_operand(target, codegen_unit, operand.as.vreg_index, text);
    case LIR_OPERAND_SLOT:
        return mc_format_slot_operand(codegen_unit, operand.as.slot_index, text);
    case LIR_OPERAND_GLOBAL:
        *text = mc_copy_format("global(%s)", operand.as.global_name);
        return *text != NULL;
    case LIR_OPERAND_LITERAL:
        return mc_format_literal_operand(operand, text);
    }

    return false;
}
