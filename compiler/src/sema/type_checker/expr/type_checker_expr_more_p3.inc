            if (symbol &&
                resolution &&
                (symbol->kind == SYMBOL_KIND_PARAMETER ||
                 symbol->kind == SYMBOL_KIND_LOCAL ||
                 symbol->kind == SYMBOL_KIND_TOP_LEVEL_BINDING) &&
                !tc_scope_is_within(symbol->scope, spawn_scope)) {
                return tc_emit_spawn_race_diagnostic(checker, expression, symbol);
            }
        }
        return true;
    case AST_EXPR_ASSIGNMENT:
        return tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.assignment.target,
                                                  spawn_scope,
                                                  visited) &&
               tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.assignment.value,
                                                  spawn_scope,
                                                  visited);
    case AST_EXPR_TERNARY:
        return tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.ternary.condition,
                                                  spawn_scope,
                                                  visited) &&
               tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.ternary.then_branch,
                                                  spawn_scope,
                                                  visited) &&
               tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.ternary.else_branch,
                                                  spawn_scope,
                                                  visited);
    case AST_EXPR_BINARY:
        return tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.binary.left,
                                                  spawn_scope,
                                                  visited) &&
               tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.binary.right,
                                                  spawn_scope,
                                                  visited);
    case AST_EXPR_UNARY:
        return tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.unary.operand,
                                                  spawn_scope,
                                                  visited);
    case AST_EXPR_CALL:
        if (!tc_scan_spawn_expression_for_races(checker,
                                                expression->as.call.callee,
                                                spawn_scope,
                                                visited)) {
            return false;
        }
        for (i = 0; i < expression->as.call.arguments.count; i++) {
            if (!tc_scan_spawn_expression_for_races(checker,
                                                    expression->as.call.arguments.items[i],
                                                    spawn_scope,
                                                    visited)) {
                return false;
            }
        }
        return true;
    case AST_EXPR_INDEX:
        return tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.index.target,
                                                  spawn_scope,
                                                  visited) &&
               tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.index.index,
                                                  spawn_scope,
                                                  visited);
    case AST_EXPR_MEMBER:
        return tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.member.target,
                                                  spawn_scope,
                                                  visited);
    case AST_EXPR_CAST:
        return tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.cast.expression,
                                                  spawn_scope,
                                                  visited);
    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            if (!tc_scan_spawn_expression_for_races(checker,
                                                    expression->as.array_literal.elements.items[i],
                                                    spawn_scope,
                                                    visited)) {
                return false;
            }
        }
        return true;
    case AST_EXPR_GROUPING:
        return tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.grouping.inner,
                                                  spawn_scope,
                                                  visited);
    case AST_EXPR_POST_INCREMENT:
        return tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.post_increment.operand,
                                                  spawn_scope,
                                                  visited);
    case AST_EXPR_POST_DECREMENT:
        return tc_scan_spawn_expression_for_races(checker,
                                                  expression->as.post_decrement.operand,
                                                  spawn_scope,
                                                  visited);
    case AST_EXPR_MEMORY_OP:
        for (i = 0; i < expression->as.memory_op.arguments.count; i++) {
            if (!tc_scan_spawn_expression_for_races(checker,
                                                    expression->as.memory_op.arguments.items[i],
                                                    spawn_scope,
                                                    visited)) {
                return false;
            }
        }
        return true;
    case AST_EXPR_LAMBDA:
        if (expression->as.lambda.body.kind != AST_LAMBDA_BODY_BLOCK ||
            !expression->as.lambda.body.as.block) {
            return tc_scan_spawn_expression_for_races(checker,
                                                      expression->as.lambda.body.as.expression,
                                                      spawn_scope,
                                                      visited);
        }
        for (i = 0; i < expression->as.lambda.body.as.block->statement_count; i++) {
            if (!tc_scan_spawn_statement_for_races(checker,
                                                   expression->as.lambda.body.as.block->statements[i],
                                                   spawn_scope,
                                                   visited)) {
                return false;
            }
        }
        return true;
    case AST_EXPR_SPAWN:
        return tc_analyze_spawn_callable_races(checker,
                                               expression->as.spawn.callable,
                                               visited);
    case AST_EXPR_DISCARD:
        return true;
    }

    return true;
}

const TypeCheckInfo *tc_check_expression_more(TypeChecker *checker,
                                              const AstExpression *expression) {
    TypeCheckInfo info = tc_type_check_info_make(tc_checked_type_invalid());

    switch (expression->kind) {
    case AST_EXPR_INDEX:
        {
            const TypeCheckInfo *target_info = tc_check_expression(
                checker, expression->as.index.target);
            const TypeCheckInfo *index_info = tc_check_expression(
                checker, expression->as.index.index);
            CheckedType target_type, index_type;
            char target_text[64], index_text[64];

            if (!target_info || !index_info) {
                return NULL;
            }

            target_type = tc_type_check_source_type(target_info);
            index_type = tc_type_check_source_type(index_info);
            if (!tc_checked_type_is_integral(index_type)) {
                checked_type_to_string(index_type, index_text, sizeof(index_text));
                tc_set_error_at(checker,
                                expression->as.index.index->source_span,
                                NULL,
                                "Index expression must have an integral type but got %s.",
                                index_text);
                return NULL;
            }

            if (target_type.kind == CHECKED_TYPE_EXTERNAL) {
                info = tc_type_check_info_make_external_value();
                break;
            }

            if (tc_checked_type_is_string(target_type)) {
                info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_CHAR, 0));
                break;
            }

            if (tc_checked_type_is_hetero_array(target_type)) {
                info = tc_type_check_info_make_external_value();
                break;
            }

            if (target_type.kind == CHECKED_TYPE_VALUE && target_type.array_depth > 0) {
                info = tc_type_check_info_make(tc_checked_type_value(target_type.primitive,
                                                                     target_type.array_depth - 1));
                break;
            }

            checked_type_to_string(target_type, target_text, sizeof(target_text));
            tc_set_error_at(checker,
                            expression->as.index.target->source_span,
                            NULL,
                            "Cannot index expression of type %s.",
                            target_text);
            return NULL;
        }

    case AST_EXPR_MEMBER:
        {
            const TypeCheckInfo *target_info = tc_check_expression(
                checker, expression->as.member.target);
            CheckedType target_type;
            const Symbol *union_sym;
            const Symbol *target_symbol = NULL;
            char target_text[64];

            if (!target_info) {
                return NULL;
            }

            if (expression->as.member.target->kind == AST_EXPR_IDENTIFIER) {
                target_symbol = symbol_table_resolve_identifier(
                    checker->symbols, expression->as.member.target);
            }

#include "type_checker_expr_more_p4.inc"
