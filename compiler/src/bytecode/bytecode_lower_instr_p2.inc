    case MIR_INSTR_TEMPLATE:
        lowered->as.template_literal.dest_temp = instruction->as.template_literal.dest_temp;
        lowered->as.template_literal.part_count = instruction->as.template_literal.part_count;
        if (lowered->as.template_literal.part_count > 0) {
            lowered->as.template_literal.parts = calloc(lowered->as.template_literal.part_count,
                                                        sizeof(*lowered->as.template_literal.parts));
            if (!lowered->as.template_literal.parts) {
                return false;
            }
        }
        for (i = 0; i < lowered->as.template_literal.part_count; i++) {
            if (!bc_lower_template_part(context,
                                        &instruction->as.template_literal.parts[i],
                                        &lowered->as.template_literal.parts[i])) {
                return false;
            }
        }
        return true;

    case MIR_INSTR_STORE_LOCAL:
        lowered->as.store_local.local_index = instruction->as.store_local.local_index;
        return bc_value_from_mir_value(context,
                                       instruction->as.store_local.value,
                                       &lowered->as.store_local.value);

    case MIR_INSTR_STORE_GLOBAL:
        lowered->as.store_global.global_index = bc_intern_text_constant(context,
                                                                        BYTECODE_CONSTANT_SYMBOL,
                                                                        instruction->as.store_global.global_name);
        return lowered->as.store_global.global_index != (size_t)-1 &&
               bc_value_from_mir_value(context,
                                       instruction->as.store_global.value,
                                       &lowered->as.store_global.value);

    case MIR_INSTR_STORE_INDEX:
        return bc_value_from_mir_value(context,
                                       instruction->as.store_index.target,
                                       &lowered->as.store_index.target) &&
               bc_value_from_mir_value(context,
                                       instruction->as.store_index.index,
                                       &lowered->as.store_index.index) &&
               bc_value_from_mir_value(context,
                                       instruction->as.store_index.value,
                                       &lowered->as.store_index.value);

    case MIR_INSTR_STORE_MEMBER:
        lowered->as.store_member.member_index = bc_intern_text_constant(context,
                                                                        BYTECODE_CONSTANT_SYMBOL,
                                                                        instruction->as.store_member.member);
        return lowered->as.store_member.member_index != (size_t)-1 &&
               bc_value_from_mir_value(context,
                                       instruction->as.store_member.target,
                                       &lowered->as.store_member.target) &&
               bc_value_from_mir_value(context,
                                       instruction->as.store_member.value,
                                       &lowered->as.store_member.value);

    case MIR_INSTR_HETERO_ARRAY_NEW:
        return bc_lower_hetero_array_instruction(context, instruction, lowered);

    case MIR_INSTR_UNION_NEW:
        return bc_lower_union_new_instruction(context, instruction, lowered);
    }

    return false;
}
