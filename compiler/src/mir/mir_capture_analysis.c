#include "mir_internal.h"

static bool collect_block_captures(MirBuildContext *context,
                                   const HirBlock *block,
                                   const MirBoundSymbolSet *bound,
                                   MirCaptureList *captures);

static bool collect_lambda_local_symbols(MirBuildContext *context,
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

static bool collect_expression_captures(MirBuildContext *context,
                                        const HirExpression *expression,
                                        const MirBoundSymbolSet *bound,
                                        MirCaptureList *captures) {
    size_t i;

    if (!expression) {
        return true;
    }

    switch (expression->kind) {
    case HIR_EXPR_LITERAL:
        return true;
    case HIR_EXPR_TEMPLATE:
        for (i = 0; i < expression->as.template_parts.count; i++) {
            if (expression->as.template_parts.items[i].kind == AST_TEMPLATE_PART_EXPRESSION &&
                !collect_expression_captures(context,
                                             expression->as.template_parts.items[i].as.expression,
                                             bound,
                                             captures)) {
                return false;
            }
        }
        return true;
    case HIR_EXPR_SYMBOL:
        if ((expression->as.symbol.kind == SYMBOL_KIND_PARAMETER ||
             expression->as.symbol.kind == SYMBOL_KIND_LOCAL) &&
            !mr_bound_symbol_set_contains(bound, expression->as.symbol.symbol)) {
            return mr_capture_list_add(context,
                                        captures,
                                        expression->as.symbol.symbol,
                                        expression->as.symbol.type,
                                        expression->source_span);
        }
        return true;
    case HIR_EXPR_LAMBDA:
        {
            MirBoundSymbolSet nested_bound;

            if (!mr_bound_symbol_set_clone(context,
                                            bound,
                                            &nested_bound,
                                            expression->source_span)) {
                return false;
            }

            for (i = 0; i < expression->as.lambda.parameters.count; i++) {
                if (!mr_bound_symbol_set_add(context,
                                              &nested_bound,
                                              expression->as.lambda.parameters.items[i].symbol,
                                              expression->source_span)) {
                    mr_bound_symbol_set_free(&nested_bound);
                    return false;
                }
            }

            if (!collect_lambda_local_symbols(context,
                                              expression->as.lambda.body,
                                              &nested_bound,
                                              expression->source_span) ||
                !collect_block_captures(context,
                                        expression->as.lambda.body,
                                        &nested_bound,
                                        captures)) {
                mr_bound_symbol_set_free(&nested_bound);
                return false;
            }

            mr_bound_symbol_set_free(&nested_bound);
            return true;
        }
    case HIR_EXPR_ASSIGNMENT:
        return collect_expression_captures(context,
                                           expression->as.assignment.target,
                                           bound,
                                           captures) &&
               collect_expression_captures(context,
                                           expression->as.assignment.value,
                                           bound,
                                           captures);
    case HIR_EXPR_TERNARY:
        return collect_expression_captures(context,
                                           expression->as.ternary.condition,
                                           bound,
                                           captures) &&
               collect_expression_captures(context,
                                           expression->as.ternary.then_branch,
                                           bound,
                                           captures) &&
               collect_expression_captures(context,
                                           expression->as.ternary.else_branch,
                                           bound,
                                           captures);
    case HIR_EXPR_BINARY:
        return collect_expression_captures(context,
                                           expression->as.binary.left,
                                           bound,
                                           captures) &&
               collect_expression_captures(context,
                                           expression->as.binary.right,
                                           bound,
                                           captures);
    case HIR_EXPR_UNARY:
        return collect_expression_captures(context,
                                           expression->as.unary.operand,
                                           bound,
                                           captures);
    case HIR_EXPR_CALL:
        if (!collect_expression_captures(context,
                                         expression->as.call.callee,
                                         bound,
                                         captures)) {
            return false;
        }
        for (i = 0; i < expression->as.call.argument_count; i++) {
            if (!collect_expression_captures(context,
                                             expression->as.call.arguments[i],
                                             bound,
                                             captures)) {
                return false;
            }
        }
        return true;
    case HIR_EXPR_INDEX:
        return collect_expression_captures(context,
                                           expression->as.index.target,
                                           bound,
                                           captures) &&
               collect_expression_captures(context,
                                           expression->as.index.index,
                                           bound,
                                           captures);
    case HIR_EXPR_MEMBER:
        return collect_expression_captures(context,
                                           expression->as.member.target,
                                           bound,
                                           captures);
    case HIR_EXPR_CAST:
        return collect_expression_captures(context,
                                           expression->as.cast.expression,
                                           bound,
                                           captures);
    case HIR_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.element_count; i++) {
            if (!collect_expression_captures(context,
                                             expression->as.array_literal.elements[i],
                                             bound,
                                             captures)) {
                return false;
            }
        }
        return true;
    case HIR_EXPR_DISCARD:
        return true;
    case HIR_EXPR_POST_INCREMENT:
        return collect_expression_captures(context,
                                           expression->as.post_increment.operand,
                                           bound,
                                           captures);
    case HIR_EXPR_POST_DECREMENT:
        return collect_expression_captures(context,
                                           expression->as.post_decrement.operand,
                                           bound,
                                           captures);
    case HIR_EXPR_MEMORY_OP:
        {
            size_t mem_i;
            for (mem_i = 0; mem_i < expression->as.memory_op.argument_count; mem_i++) {
                if (!collect_expression_captures(context,
                                                 expression->as.memory_op.arguments[mem_i],
                                                 bound,
                                                 captures)) {
                    return false;
                }
            }
            return true;
        }
    }

    return true;
}

static bool collect_statement_captures(MirBuildContext *context,
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

static bool collect_block_captures(MirBuildContext *context,
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
