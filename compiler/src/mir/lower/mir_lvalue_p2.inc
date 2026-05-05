    switch (lvalue->kind) {
    case MIR_LVALUE_LOCAL:
        if (lvalue->is_cell) {
            return mr_emit_cell_write(context, lvalue->as.local_index,
                                       value, source_span);
        }
        return mr_append_store_local_instruction(context,
                                              lvalue->as.local_index,
                                              value,
                                              source_span);
    case MIR_LVALUE_GLOBAL:
        return mr_append_store_global_instruction(context,
                                               lvalue->as.global_name,
                                               value,
                                               source_span);
    case MIR_LVALUE_INDEX:
        return mr_append_store_index_instruction(context,
                                              &lvalue->as.index.target,
                                              &lvalue->as.index.index,
                                              value,
                                              source_span);
    case MIR_LVALUE_MEMBER:
        return mr_append_store_member_instruction(context,
                                               &lvalue->as.member.target,
                                               lvalue->as.member.member,
                                               value,
                                               source_span);
    }

    mr_value_free(&value);
    return false;
}
