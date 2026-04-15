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
        if (statement->as.manual_body) {
            return collect_block_captures(context,
                                          statement->as.manual_body,
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
