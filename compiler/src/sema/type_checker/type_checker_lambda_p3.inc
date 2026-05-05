    if (start_decl->is_boot) {
        if (start_decl->parameters.count != 0) {
            tc_set_error_at(checker,
                            start_decl->start_span,
                            NULL,
                            "boot must declare zero parameters.");
            return false;
        }
    } else {
        if (start_decl->parameters.count > 1) {
            tc_set_error_at(checker,
                            start_decl->start_span,
                            NULL,
                            "start must declare zero parameters or one parameter of type string[].");
            return false;
        }

        if (start_decl->parameters.count == 1 &&
            (start_decl->parameters.items[0].type.kind != AST_TYPE_PRIMITIVE ||
             start_decl->parameters.items[0].type.primitive != AST_PRIMITIVE_STRING ||
             start_decl->parameters.items[0].type.dimension_count != 1 ||
             start_decl->parameters.items[0].type.dimensions[0].has_size)) {
            tc_set_error_at(checker,
                            start_decl->parameters.items[0].name_span,
                            &start_decl->start_span,
                            "start parameter must have type string[].");
            return false;
        }
    }

    body_span = start_decl->start_span;
    if (start_decl->body.kind == AST_LAMBDA_BODY_BLOCK) {
        BlockContext context;

        memset(&context, 0, sizeof(context));
        context.kind = BLOCK_CONTEXT_START;
        context.owner_span = start_decl->start_span;
        context.related_span = start_decl->start_span;
        context.has_related_span = true;

        if (!tc_check_block(checker,
                            start_decl->body.as.block,
                            &context,
                            &body_type,
                            &body_span)) {
            return false;
        }
    } else {
        const TypeCheckInfo *body_info = tc_check_expression(checker, start_decl->body.as.expression);

        if (!body_info) {
            return false;
        }

        body_type = tc_type_check_source_type(body_info);
        body_span = start_decl->body.as.expression->source_span;
    }

    if (body_type.kind == CHECKED_TYPE_VOID) {
        return true;
    }

    if (!tc_checked_type_assignable(int32_type, body_type)) {
        checked_type_to_string(body_type, body_text, sizeof(body_text));
        tc_set_error_at(checker,
                        body_span,
                        &start_decl->start_span,
                        "%s body must produce int32 or void but got %s.",
                        entry_name,
                        body_text);
        return false;
    }

    return true;
}
