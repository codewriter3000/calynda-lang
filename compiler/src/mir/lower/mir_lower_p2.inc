    current_block = mr_current_block(context);
    if (!current_block) {
        mr_set_error(context->build,
                      (AstSourceSpan){0},
                      NULL,
                      "Internal error: missing current MIR block at block exit.");
        return false;
    }

    if (current_block->terminator.kind == MIR_TERM_NONE) {
        current_block->terminator.kind = MIR_TERM_RETURN;
        current_block->terminator.as.return_term.has_value = false;
    }
    return true;
}

bool mr_lower_parameters(MirUnitBuildContext *context,
                         const HirParameterList *parameters) {
    size_t i;

    if (!context || !parameters) {
        return false;
    }

    for (i = 0; i < parameters->count; i++) {
        MirLocal local;

        memset(&local, 0, sizeof(local));
        local.kind = MIR_LOCAL_PARAMETER;
        local.name = ast_copy_text(parameters->items[i].name);
        local.symbol = parameters->items[i].symbol;
        local.type = parameters->items[i].type;
        local.index = context->unit->local_count;
        if (!local.name || !mr_append_local(context->unit, local)) {
            free(local.name);
            mr_set_error(context->build,
                          parameters->items[i].source_span,
                          NULL,
                          "Out of memory while lowering MIR parameters.");
            return false;
        }
        context->unit->parameter_count++;
    }

    return true;
}


