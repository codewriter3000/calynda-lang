        if (statement->kind == HIR_STMT_RETURN) {
            statement->as.return_expression = expression;
        } else {
            statement->as.expression = expression;
        }

        if (!hr_append_statement(block, statement)) {
            hir_statement_free(statement);
            hir_block_free(block);
            if (!context->program->has_error) {
                hr_set_error(context,
                             body->as.expression->source_span,
                             NULL,
                             "Out of memory while lowering HIR statements.");
            }
            return NULL;
        }
    }

    return block;
}

HirBlock *hr_lower_block(HirBuildContext *context, const AstBlock *block) {
    const Scope *scope;
    HirBlock *hir_block;
    size_t i;

    if (!block) {
        return NULL;
    }

    scope = symbol_table_find_scope(context->symbols, block, SCOPE_KIND_BLOCK);
    if (!scope) {
        hr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Internal error: missing scope for block during HIR lowering.");
        return NULL;
    }

    hir_block = hr_block_new();
    if (!hir_block) {
        hr_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while lowering HIR blocks.");
        return NULL;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (block->statements[i]->kind == AST_STMT_SWAP) {
            if (!hr_lower_swap_statement(context, hir_block, block->statements[i])) {
                hir_block_free(hir_block);
                return NULL;
            }
            continue;
        }

        {
            HirStatement *statement = hr_lower_statement(context, block->statements[i], scope);

            if (!statement) {
                hir_block_free(hir_block);
                return NULL;
            }
            if (!hr_append_statement(hir_block, statement)) {
                hir_statement_free(statement);
                hir_block_free(hir_block);
                hr_set_error(context,
                             block->statements[i]->source_span,
                             NULL,
                             "Out of memory while lowering HIR statements.");
                return NULL;
            }
        }
    }

    return hir_block;
}
 
