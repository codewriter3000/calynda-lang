    cg_set_error(context,
                 (AstSourceSpan){0},
                 NULL,
                 "Internal error: unsupported LIR instruction kind %d during instruction selection.",
                 instruction->kind);
    return false;
}

bool cg_select_terminator(CodegenBuildContext *context,
                          const LirTerminator *terminator,
                          CodegenSelectedTerminator *selected) {
    if (!terminator || !selected) {
        return false;
    }

    memset(selected, 0, sizeof(*selected));
    selected->kind = terminator->kind;

    switch (terminator->kind) {
    case LIR_TERM_NONE:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_RETURN;
        return true;

    case LIR_TERM_RETURN:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_RETURN;
        return true;

    case LIR_TERM_JUMP:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_JUMP;
        return true;

    case LIR_TERM_BRANCH:
        selected->selection.kind = CODEGEN_SELECTION_DIRECT;
        selected->selection.as.direct_pattern = CODEGEN_DIRECT_BRANCH;
        return true;

    case LIR_TERM_THROW:
        selected->selection.kind = CODEGEN_SELECTION_RUNTIME;
        selected->selection.as.runtime_helper = CODEGEN_RUNTIME_THROW;
        return true;
    }

    cg_set_error(context,
                 (AstSourceSpan){0},
                 NULL,
                 "Internal error: unsupported LIR terminator kind %d during instruction selection.",
                 terminator->kind);
    return false;
}
