    switch (statement->kind) {
    case AST_STMT_LOCAL_BINDING:
        return tc_scan_spawn_expression_for_races(checker,
                                                  statement->as.local_binding.initializer,
                                                  spawn_scope,
                                                  visited);
    case AST_STMT_RETURN:
        return tc_scan_spawn_expression_for_races(checker,
                                                  statement->as.return_expression,
                                                  spawn_scope,
                                                  visited);
    case AST_STMT_THROW:
        return tc_scan_spawn_expression_for_races(checker,
                                                  statement->as.throw_expression,
                                                  spawn_scope,
                                                  visited);
    case AST_STMT_EXPRESSION:
        return tc_scan_spawn_expression_for_races(checker,
                                                  statement->as.expression,
                                                  spawn_scope,
                                                  visited);
    case AST_STMT_MANUAL:
        if (!statement->as.manual.body) {
            return true;
        }
        {
            size_t i;
            for (i = 0; i < statement->as.manual.body->statement_count; i++) {
                if (!tc_scan_spawn_statement_for_races(checker,
                                                       statement->as.manual.body->statements[i],
                                                       spawn_scope,
                                                       visited)) {
                    return false;
                }
            }
        }
        return true;
    case AST_STMT_SWAP:
        return tc_scan_spawn_expression_for_races(checker,
                                                  statement->as.swap.left,
                                                  spawn_scope,
                                                  visited) &&
               tc_scan_spawn_expression_for_races(checker,
                                                  statement->as.swap.right,
                                                  spawn_scope,
                                                  visited);
    case AST_STMT_EXIT:
        return true;
    }

    return true;
}

static bool tc_analyze_spawn_callable_races(TypeChecker *checker,
                                            const AstExpression *callable,
                                            TcVisitedSymbolSet *visited) {
    const Symbol *symbol;
    const AstExpression *initializer = NULL;
    const Scope *lambda_scope;
    size_t i;

    if (!callable) {
        return true;
    }

    switch (callable->kind) {
    case AST_EXPR_GROUPING:
        return tc_analyze_spawn_callable_races(checker,
                                               callable->as.grouping.inner,
                                               visited);
    case AST_EXPR_LAMBDA:
        lambda_scope = symbol_table_find_scope(checker->symbols,
                                               callable,
                                               SCOPE_KIND_LAMBDA);
        if (!lambda_scope) {
            return true;
        }
        if (callable->as.lambda.body.kind != AST_LAMBDA_BODY_BLOCK ||
            !callable->as.lambda.body.as.block) {
            return tc_scan_spawn_expression_for_races(checker,
                                                      callable->as.lambda.body.as.expression,
                                                      lambda_scope,
                                                      visited);
        }
        for (i = 0; i < callable->as.lambda.body.as.block->statement_count; i++) {
            if (!tc_scan_spawn_statement_for_races(checker,
                                                   callable->as.lambda.body.as.block->statements[i],
                                                   lambda_scope,
                                                   visited)) {
                return false;
            }
        }
        return true;
    case AST_EXPR_IDENTIFIER:
        symbol = symbol_table_resolve_identifier(checker->symbols, callable);
        if (!symbol || tc_visited_contains(visited, symbol)) {
            return true;
        }
        if (!tc_visited_push(checker, visited, symbol, callable->source_span)) {
            return false;
        }
        if (symbol->kind == SYMBOL_KIND_LOCAL) {
            initializer = ((const AstLocalBindingStatement *)symbol->declaration)->initializer;
        } else if (symbol->kind == SYMBOL_KIND_TOP_LEVEL_BINDING) {
            initializer = ((const AstBindingDecl *)symbol->declaration)->initializer;
        }
        return initializer
            ? tc_analyze_spawn_callable_races(checker, initializer, visited)
            : true;
    default:
        return true;
    }
}

static bool tc_scan_spawn_expression_for_races(TypeChecker *checker,
                                               const AstExpression *expression,
                                               const Scope *spawn_scope,
                                               TcVisitedSymbolSet *visited) {
    size_t i;

    if (!expression) {
        return true;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                const AstTemplatePart *part = &expression->as.literal.as.template_parts.items[i];
                if (part->kind == AST_TEMPLATE_PART_EXPRESSION &&
                    !tc_scan_spawn_expression_for_races(checker,
                                                        part->as.expression,
                                                        spawn_scope,
                                                        visited)) {
                    return false;
                }
            }
        }
        return true;
    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *symbol = symbol_table_resolve_identifier(checker->symbols, expression);
            const SymbolResolution *resolution = symbol_table_find_resolution(checker->symbols,
                                                                              expression);

#include "type_checker_expr_more_p3.inc"
