    if (binding_decl->is_inferred_type) {
        return true;
    }

    return tr_resolve_declared_type(resolver,
                                    &binding_decl->declared_type,
                                    binding_decl->name_span,
                                    "Top-level binding",
                                    binding_decl->name,
                                    true);
}

bool tr_resolve_start_decl(TypeResolver *resolver, const AstStartDecl *start_decl) {
    size_t i;

    if (!start_decl) {
        return true;
    }

    for (i = 0; i < start_decl->parameters.count; i++) {
        if (!tr_resolve_parameter(resolver, &start_decl->parameters.items[i])) {
            return false;
        }
    }

    if (start_decl->body.kind == AST_LAMBDA_BODY_BLOCK) {
        return tr_resolve_block(resolver, start_decl->body.as.block);
    }

    return tr_resolve_expression(resolver, start_decl->body.as.expression);
}

bool tr_resolve_block(TypeResolver *resolver, const AstBlock *block) {
    size_t i;

    if (!block) {
        return true;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (!resolve_statement(resolver, block->statements[i])) {
            return false;
        }
    }

    return true;
}

static bool resolve_statement(TypeResolver *resolver, const AstStatement *statement) {
    if (!statement) {
        return true;
    }

    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        if (!statement->as.local_binding.is_inferred_type &&
            !tr_resolve_declared_type(resolver,
                                      &statement->as.local_binding.declared_type,
                                      statement->as.local_binding.name_span,
                                      "Local",
                                      statement->as.local_binding.name,
                                      true)) {
            return false;
        }
        return tr_resolve_expression(resolver, statement->as.local_binding.initializer);

    case AST_STMT_RETURN:
        return tr_resolve_expression(resolver, statement->as.return_expression);

    case AST_STMT_EXIT:
        return true;

    case AST_STMT_THROW:
        return tr_resolve_expression(resolver, statement->as.throw_expression);

    case AST_STMT_EXPRESSION:
        return tr_resolve_expression(resolver, statement->as.expression);

    case AST_STMT_MANUAL:
        if (statement->as.manual.body) {
            return tr_resolve_block(resolver, statement->as.manual.body);
        }
        return true;
    case AST_STMT_SWAP:
        return tr_resolve_expression(resolver, statement->as.swap.left) &&
               tr_resolve_expression(resolver, statement->as.swap.right);
    }

    return false;
}
