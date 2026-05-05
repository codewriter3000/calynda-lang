#include "mir_internal.h"


bool collect_expression_captures(MirBuildContext *context,
                                const HirExpression *expression,
                                const MirBoundSymbolSet *bound,
                                MirCaptureList *captures);
bool collect_block_captures(MirBuildContext *context,
                                   const HirBlock *block,
                                   const MirBoundSymbolSet *bound,
                                   MirCaptureList *captures);

bool collect_lambda_local_symbols(MirBuildContext *context,
                                         const HirBlock *block,
                                         MirBoundSymbolSet *bound,
                                         AstSourceSpan source_span) {
    size_t i;

    if (!block || !bound) {
        return true;
    }

    for (i = 0; i < block->statement_count; i++) {
        const HirStatement *statement = block->statements[i];

        if (statement->kind == HIR_STMT_LOCAL_BINDING &&
            statement->as.local_binding.symbol &&
            !mr_bound_symbol_set_add(context,
                                      bound,
                                      statement->as.local_binding.symbol,
                                      source_span)) {
            return false;
        }
        if (statement->kind == HIR_STMT_MANUAL &&
            statement->as.manual.body &&
            !collect_lambda_local_symbols(context,
                                          statement->as.manual.body,
                                          bound,
                                          source_span)) {
            return false;
        }
    }

    return true;
}

bool collect_statement_captures(MirBuildContext *context,
                                       const HirStatement *statement,
                                       const MirBoundSymbolSet *bound,
                                       MirCaptureList *captures) {
    if (!statement) {
        return true;
    }

    switch (statement->kind) {
    case HIR_STMT_LOCAL_BINDING:
        return collect_expression_captures(context,
                                           statement->as.local_binding.initializer,
                                           bound,
                                           captures);
    case HIR_STMT_RETURN:
        return collect_expression_captures(context,
                                           statement->as.return_expression,
                                           bound,
                                           captures);
    case HIR_STMT_THROW:
        return collect_expression_captures(context,
                                           statement->as.throw_expression,
                                           bound,
                                           captures);
    case HIR_STMT_EXPRESSION:
        return collect_expression_captures(context,
                                           statement->as.expression,
                                           bound,
                                           captures);
    case HIR_STMT_EXIT:
        return true;
    case HIR_STMT_MANUAL:
        if (statement->as.manual.body) {
            return collect_block_captures(context,
                                          statement->as.manual.body,
                                          bound,
                                          captures);
        }
        return true;
    }

    return true;
}

bool collect_block_captures(MirBuildContext *context,
                                   const HirBlock *block,
                                   const MirBoundSymbolSet *bound,
                                   MirCaptureList *captures) {
    size_t i;

    if (!block) {
        return true;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (!collect_statement_captures(context,
                                        block->statements[i],
                                        bound,
                                        captures)) {
            return false;
        }
    }

    return true;
}

bool mr_analyze_lambda_captures(MirBuildContext *context,
                                const HirLambdaExpression *lambda,
                                AstSourceSpan source_span,
                                MirCaptureList *captures) {
    MirBoundSymbolSet bound;
    size_t i;

    memset(&bound, 0, sizeof(bound));
    memset(captures, 0, sizeof(*captures));

    for (i = 0; i < lambda->parameters.count; i++) {
        if (!mr_bound_symbol_set_add(context,
                                      &bound,
                                      lambda->parameters.items[i].symbol,
                                      source_span)) {
            mr_bound_symbol_set_free(&bound);
            mr_capture_list_free(captures);
            return false;
        }
    }

    if (!collect_lambda_local_symbols(context, lambda->body, &bound, source_span) ||
        !collect_block_captures(context, lambda->body, &bound, captures)) {
        mr_bound_symbol_set_free(&bound);
        mr_capture_list_free(captures);
        return false;
    }

    mr_bound_symbol_set_free(&bound);
    return true;
}

/* ---- Escape analysis: find non-final locals in 'block' captured by lambdas ---- */

bool mr_symbol_in_escape_set(const Symbol * const *escape_set,
                             size_t escape_count,
                             const Symbol *symbol) {
    size_t i;
    for (i = 0; i < escape_count; i++) {
        if (escape_set[i] == symbol) {
            return true;
        }
    }
    return false;
}

static bool sym_ref_in_block(const Symbol *sym, const HirBlock *block);
static bool sym_ref_in_stmt(const Symbol *sym, const HirStatement *stmt);

/* Walk expression tree: does 'sym' appear as a symbol reference in 'expr'?
   Does NOT recurse into nested HIR_EXPR_LAMBDA nodes. */
static bool sym_ref_in_expr(const Symbol *sym, const HirExpression *expr) {
    size_t i;
    if (!expr) return false;
    switch (expr->kind) {
    case HIR_EXPR_SYMBOL:
        return (expr->as.symbol.kind == SYMBOL_KIND_LOCAL ||
                expr->as.symbol.kind == SYMBOL_KIND_PARAMETER) &&
               expr->as.symbol.symbol == sym;
    case HIR_EXPR_LAMBDA:
        /* recurse into nested lambda bodies to find any reference */
        return sym_ref_in_block(sym, expr->as.lambda.body);
    case HIR_EXPR_CALL:
        if (sym_ref_in_expr(sym, expr->as.call.callee)) return true;
        for (i = 0; i < expr->as.call.argument_count; i++)
            if (sym_ref_in_expr(sym, expr->as.call.arguments[i])) return true;
        return false;
    case HIR_EXPR_BINARY:
        return sym_ref_in_expr(sym, expr->as.binary.left) ||
               sym_ref_in_expr(sym, expr->as.binary.right);
    case HIR_EXPR_UNARY:
        return sym_ref_in_expr(sym, expr->as.unary.operand);
    case HIR_EXPR_POST_INCREMENT:
        return sym_ref_in_expr(sym, expr->as.post_increment.operand);
    case HIR_EXPR_TERNARY:
        return sym_ref_in_expr(sym, expr->as.ternary.condition) ||
               sym_ref_in_expr(sym, expr->as.ternary.then_branch) ||
               sym_ref_in_expr(sym, expr->as.ternary.else_branch);
    case HIR_EXPR_ASSIGNMENT:
        return sym_ref_in_expr(sym, expr->as.assignment.target) ||
               sym_ref_in_expr(sym, expr->as.assignment.value);
    case HIR_EXPR_MEMBER:
        return sym_ref_in_expr(sym, expr->as.member.target);
    case HIR_EXPR_INDEX:
        return sym_ref_in_expr(sym, expr->as.index.target) ||
               sym_ref_in_expr(sym, expr->as.index.index);
    case HIR_EXPR_CAST:
        return sym_ref_in_expr(sym, expr->as.cast.expression);
    case HIR_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expr->as.array_literal.element_count; i++)
            if (sym_ref_in_expr(sym, expr->as.array_literal.elements[i])) return true;
        return false;
    case HIR_EXPR_NONLOCAL_RETURN:
        return sym_ref_in_expr(sym, expr->as.nonlocal_return_value);
    default:
        return false;
    }
}

static bool sym_ref_in_stmt(const Symbol *sym, const HirStatement *stmt) {
    if (!stmt) return false;
    switch (stmt->kind) {
    case HIR_STMT_LOCAL_BINDING:
        return sym_ref_in_expr(sym, stmt->as.local_binding.initializer);
    case HIR_STMT_RETURN:
        return sym_ref_in_expr(sym, stmt->as.return_expression);
    case HIR_STMT_THROW:
        return sym_ref_in_expr(sym, stmt->as.throw_expression);
    case HIR_STMT_EXPRESSION:
        return sym_ref_in_expr(sym, stmt->as.expression);
    case HIR_STMT_MANUAL:
        return sym_ref_in_block(sym, stmt->as.manual.body);
    default:
        return false;
    }
}

static bool sym_ref_in_block(const Symbol *sym, const HirBlock *block) {
    size_t i;
    if (!block) return false;
    for (i = 0; i < block->statement_count; i++)
        if (sym_ref_in_stmt(sym, block->statements[i])) return true;
    return false;
}

#include "mir_capture_analysis_p2.inc"
