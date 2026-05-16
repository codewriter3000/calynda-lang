/* Check whether 'sym' escapes into any direct lambda in 'expr'.
   "Escapes" = sym is referenced anywhere inside a HIR_EXPR_LAMBDA's body.
   We recurse into non-lambda expression nodes to find nested lambdas. */
static bool sym_escapes_in_expr(const Symbol *sym, const HirExpression *expr) {
    size_t i;
    if (!expr) return false;
    switch (expr->kind) {
    case HIR_EXPR_LAMBDA:
        /* sym escapes if referenced inside this lambda's body */
        return sym_ref_in_block(sym, expr->as.lambda.body);
    case HIR_EXPR_CALL:
        if (sym_escapes_in_expr(sym, expr->as.call.callee)) return true;
        for (i = 0; i < expr->as.call.argument_count; i++)
            if (sym_escapes_in_expr(sym, expr->as.call.arguments[i])) return true;
        return false;
    case HIR_EXPR_BINARY:
        return sym_escapes_in_expr(sym, expr->as.binary.left) ||
               sym_escapes_in_expr(sym, expr->as.binary.right);
    case HIR_EXPR_UNARY:
        return sym_escapes_in_expr(sym, expr->as.unary.operand);
    case HIR_EXPR_POST_INCREMENT:
        return sym_escapes_in_expr(sym, expr->as.post_increment.operand);
    case HIR_EXPR_TERNARY:
        return sym_escapes_in_expr(sym, expr->as.ternary.condition) ||
               sym_escapes_in_expr(sym, expr->as.ternary.then_branch) ||
               sym_escapes_in_expr(sym, expr->as.ternary.else_branch);
    case HIR_EXPR_ASSIGNMENT:
        return sym_escapes_in_expr(sym, expr->as.assignment.target) ||
               sym_escapes_in_expr(sym, expr->as.assignment.value);
    case HIR_EXPR_MEMBER:
        return sym_escapes_in_expr(sym, expr->as.member.target);
    case HIR_EXPR_INDEX:
        return sym_escapes_in_expr(sym, expr->as.index.target) ||
               sym_escapes_in_expr(sym, expr->as.index.index);
    case HIR_EXPR_CAST:
        return sym_escapes_in_expr(sym, expr->as.cast.expression);
    case HIR_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expr->as.array_literal.element_count; i++)
            if (sym_escapes_in_expr(sym, expr->as.array_literal.elements[i])) return true;
        return false;
    case HIR_EXPR_NONLOCAL_RETURN:
        return sym_escapes_in_expr(sym, expr->as.nonlocal_return_value);
    default:
        return false;
    }
}

static bool sym_escapes_in_block(const Symbol *sym, const HirBlock *block) {
    size_t i;
    if (!block) return false;
    for (i = 0; i < block->statement_count; i++) {
        const HirStatement *stmt = block->statements[i];
        const HirExpression *expr = NULL;
        if (stmt->kind == HIR_STMT_LOCAL_BINDING) expr = stmt->as.local_binding.initializer;
        else if (stmt->kind == HIR_STMT_RETURN) expr = stmt->as.return_expression;
        else if (stmt->kind == HIR_STMT_THROW) expr = stmt->as.throw_expression;
        else if (stmt->kind == HIR_STMT_EXPRESSION) expr = stmt->as.expression;
        else if (stmt->kind == HIR_STMT_MANUAL) {
            if (sym_escapes_in_block(sym, stmt->as.manual.body)) return true;
            continue;
        }
        if (expr && sym_escapes_in_expr(sym, expr)) return true;
    }
    return false;
}

bool mr_compute_block_escape_set(MirBuildContext *context,
                                 const HirBlock *block,
                                 const Symbol ***escape_out,
                                 size_t *escape_count_out) {
    size_t i;
    const Symbol **result = NULL;
    size_t result_count = 0;
    size_t result_cap = 0;

    (void)context;
    *escape_out = NULL;
    *escape_count_out = 0;

    if (!block) return true;

    for (i = 0; i < block->statement_count; i++) {
        const HirStatement *stmt = block->statements[i];
        const Symbol *sym;
        if (stmt->kind != HIR_STMT_LOCAL_BINDING) continue;
        if (stmt->as.local_binding.is_final) continue;
        sym = stmt->as.local_binding.symbol;
        if (!sym) continue;
        if (!sym_escapes_in_block(sym, block)) continue;
        if (result_count >= result_cap) {
            size_t new_cap = result_cap ? result_cap * 2 : 4;
            const Symbol **tmp = realloc(result, new_cap * sizeof(*tmp));
            if (!tmp) { free(result); return false; }
            result = tmp;
            result_cap = new_cap;
        }
        result[result_count++] = sym;
    }

    *escape_out = result;
    *escape_count_out = result_count;
    return true;
}

