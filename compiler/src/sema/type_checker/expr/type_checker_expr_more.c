#include "type_checker_internal.h"

/* type_checker_expr_array.c */
bool tc_check_array_literal(TypeChecker *checker,
                            const AstExpression *expression,
                            TypeCheckInfo *info);
static const AstParameterList tc_empty_parameter_list = { NULL, 0, 0 };

static const Symbol *tc_find_named_type_symbol(const TypeChecker *checker,
                                               CheckedType type) {
    const Scope *root_scope;
    const Symbol *symbol;

    if (!checker || type.kind != CHECKED_TYPE_NAMED || !type.name) {
        return NULL;
    }

    root_scope = symbol_table_root_scope(checker->symbols);
    symbol = root_scope ? scope_lookup_local(root_scope, type.name) : NULL;
    if (!symbol) {
        symbol = symbol_table_find_import(checker->symbols, type.name);
    }
    return symbol;
}

static bool tc_checked_type_is_named_type(CheckedType type, const char *name) {
    return type.kind == CHECKED_TYPE_NAMED &&
           type.array_depth == 0 &&
           type.name != NULL &&
           strcmp(type.name, name) == 0;
}

typedef struct {
    const Symbol **items;
    size_t         count;
    size_t         capacity;
} TcVisitedSymbolSet;

static bool tc_scope_is_within(const Scope *scope, const Scope *ancestor) {
    const Scope *walk = scope;

    while (walk) {
        if (walk == ancestor) {
            return true;
        }
        walk = walk->parent;
    }

    return false;
}

static bool tc_visited_contains(const TcVisitedSymbolSet *set,
                                const Symbol *symbol) {
    size_t i;

    if (!set || !symbol) {
        return false;
    }
    for (i = 0; i < set->count; i++) {
        if (set->items[i] == symbol) {
            return true;
        }
    }
    return false;
}

static void tc_visited_free(TcVisitedSymbolSet *set) {
    if (!set) {
        return;
    }
    free(set->items);
    memset(set, 0, sizeof(*set));
}

static bool tc_visited_push(TypeChecker *checker,
                            TcVisitedSymbolSet *set,
                            const Symbol *symbol,
                            AstSourceSpan source_span) {
    if (!set || !symbol || tc_visited_contains(set, symbol)) {
        return true;
    }

    if (!tc_reserve_items((void **)&set->items,
                          &set->capacity,
                          set->count + 1,
                          sizeof(*set->items))) {
        tc_set_error_at(checker,
                        source_span,
                        NULL,
                        "Out of memory while analyzing spawn callables.");
        return false;
    }

    set->items[set->count++] = symbol;
    return true;
}

static bool tc_symbol_is_atomic(TypeChecker *checker, const Symbol *symbol) {
    const TypeCheckInfo *symbol_info;

    if (!checker || !symbol) {
        return false;
    }

    symbol_info = type_checker_get_symbol_info(checker, symbol);
    if (!symbol_info) {
        symbol_info = tc_resolve_symbol_info(checker, symbol);
    }
    if (!symbol_info) {
        return false;
    }

    return tc_checked_type_is_named_type(symbol_info->type, "Atomic");
}

static bool tc_symbol_is_spawn_safe(TypeChecker *checker, const Symbol *symbol) {
    return symbol &&
           (symbol->is_final ||
            symbol->is_thread_local ||
            tc_symbol_is_atomic(checker, symbol));
}

static bool tc_emit_spawn_race_diagnostic(TypeChecker *checker,
                                          const AstExpression *expression,
                                          const Symbol *symbol) {
    if (!checker || !expression || !symbol || tc_symbol_is_spawn_safe(checker, symbol)) {
        return !checker || !checker->has_error;
    }

    if (type_checker_get_global_strict_race_check()) {
        tc_set_error_at(checker,
                        expression->source_span,
                        &symbol->declaration_span,
                        "Possible data race: spawned callable captures mutable symbol '%s'. Use final, thread_local, or Atomic<T> to share it safely.",
                        symbol->name ? symbol->name : "<anonymous>");
        return false;
    }

    tc_set_warning_at(checker,
                      expression->source_span,
                      &symbol->declaration_span,
                      "Possible data race: spawned callable captures mutable symbol '%s'. Use final, thread_local, or Atomic<T> to share it safely.",
                      symbol->name ? symbol->name : "<anonymous>");
    return true;
}

static bool tc_scan_spawn_expression_for_races(TypeChecker *checker,
                                               const AstExpression *expression,
                                               const Scope *spawn_scope,
                                               TcVisitedSymbolSet *visited);

static bool tc_scan_spawn_statement_for_races(TypeChecker *checker,
                                              const AstStatement *statement,
                                              const Scope *spawn_scope,
                                              TcVisitedSymbolSet *visited) {
    if (!statement) {
        return true;
    }

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
            char target_text[64];

            if (!target_info) {
                return NULL;
            }

            target_type = tc_type_check_source_type(target_info);
            if (tc_checked_type_is_named_type(target_type, "Thread") &&
                strcmp(expression->as.member.member, "join") == 0) {
                info = tc_type_check_info_make_callable(tc_checked_type_void(),
                                                        &tc_empty_parameter_list);
                info.type = target_type;
                break;
            }
            if (tc_checked_type_is_named_type(target_type, "Thread") &&
                strcmp(expression->as.member.member, "cancel") == 0) {
                info = tc_type_check_info_make_callable(tc_checked_type_void(),
                                                        &tc_empty_parameter_list);
                info.type = target_type;
                break;
            }
            if (tc_checked_type_is_named_type(target_type, "Future")) {
                CheckedType generic_arg_type;
                bool has_generic_arg = tc_type_check_info_first_generic_arg(checker,
                                                                            target_info,
                                                                            NULL,
                                                                            &generic_arg_type);

                if (checker->has_error) {
                    return NULL;
                }
                if (strcmp(expression->as.member.member, "get") == 0) {
                    info = tc_type_check_info_make_callable(
                        has_generic_arg ? generic_arg_type : tc_checked_type_external(),
                        &tc_empty_parameter_list);
                    info.type = target_type;
                    break;
                }
                if (strcmp(expression->as.member.member, "cancel") == 0) {
                    info = tc_type_check_info_make_callable(tc_checked_type_void(),
                                                            &tc_empty_parameter_list);
                    info.type = target_type;
                    break;
                }
            }
            if (tc_checked_type_is_named_type(target_type, "Atomic")) {
                CheckedType generic_arg_type;
                bool has_generic_arg = tc_type_check_info_first_generic_arg(checker,
                                                                            target_info,
                                                                            NULL,
                                                                            &generic_arg_type);

                if (checker->has_error) {
                    return NULL;
                }
                if (strcmp(expression->as.member.member, "load") == 0) {
                    info = tc_type_check_info_make_callable(
                        has_generic_arg ? generic_arg_type : tc_checked_type_external(),
                        &tc_empty_parameter_list);
                    info.type = target_type;
                    break;
                }
                if (strcmp(expression->as.member.member, "store") == 0) {
                    info = tc_type_check_info_make_callable(tc_checked_type_void(), NULL);
                    info.type = target_type;
                    break;
                }
                if (strcmp(expression->as.member.member, "exchange") == 0) {
                    info = tc_type_check_info_make_callable(
                        has_generic_arg ? generic_arg_type : tc_checked_type_external(),
                        NULL);
                    info.type = target_type;
                    break;
                }
            }
            if (tc_checked_type_is_named_type(target_type, "Mutex")) {
                if (expression->as.member.target->kind == AST_EXPR_IDENTIFIER &&
                    expression->as.member.target->as.identifier &&
                    strcmp(expression->as.member.target->as.identifier, "Mutex") == 0 &&
                    strcmp(expression->as.member.member, "new") == 0) {
                    info = tc_type_check_info_make_callable(tc_checked_type_named("Mutex", 0, 0),
                                                            &tc_empty_parameter_list);
                    info.type = target_type;
                    break;
                }
                if (strcmp(expression->as.member.member, "lock") == 0 ||
                    strcmp(expression->as.member.member, "unlock") == 0) {
                    info = tc_type_check_info_make_callable(tc_checked_type_void(),
                                                            &tc_empty_parameter_list);
                    info.type = target_type;
                    break;
                }
            }
            if (expression->as.member.target->kind == AST_EXPR_IDENTIFIER &&
                expression->as.member.target->as.identifier &&
                strcmp(expression->as.member.target->as.identifier, "Atomic") == 0 &&
                strcmp(expression->as.member.member, "new") == 0) {
                info = tc_type_check_info_make_callable(tc_checked_type_named("Atomic", 1, 0),
                                                        NULL);
                info.type = tc_checked_type_named("Atomic", 1, 0);
                break;
            }
            if (target_type.kind == CHECKED_TYPE_EXTERNAL) {
                info = tc_type_check_info_make_external_callable();
                break;
            }

            /* Union constructor access: Option.Some */
            if (target_type.kind == CHECKED_TYPE_NAMED &&
                expression->as.member.target->kind == AST_EXPR_IDENTIFIER) {
                union_sym = symbol_table_resolve_identifier(
                    checker->symbols, expression->as.member.target);
                if (union_sym && union_sym->kind == SYMBOL_KIND_UNION) {
                    const Scope *union_scope = symbol_table_find_scope(
                        checker->symbols, union_sym->declaration, SCOPE_KIND_UNION);
                    if (union_scope) {
                        const Symbol *variant_sym = scope_lookup_local(
                            union_scope, expression->as.member.member);
                        if (variant_sym && variant_sym->kind == SYMBOL_KIND_VARIANT) {
                            if (variant_sym->variant_has_payload) {
                                info = tc_type_check_info_make(target_type);
                                info.is_callable = true;
                                info.callable_return_type = target_type;
                            } else {
                                info = tc_type_check_info_make(target_type);
                            }
                            break;
                        }
                    }
                    tc_set_error_at(checker,
                                    expression->source_span,
                                    NULL,
                                    "Union '%s' has no variant named '%s'.",
                                    union_sym->name ? union_sym->name : "?",
                                    expression->as.member.member ? expression->as.member.member : "?");
                    return NULL;
                }
            }

            union_sym = tc_find_named_type_symbol(checker, target_type);
            if (union_sym && union_sym->kind == SYMBOL_KIND_UNION) {
                if (strcmp(expression->as.member.member, "tag") == 0) {
                    info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_INT32, 0));
                    break;
                }
                if (strcmp(expression->as.member.member, "payload") == 0) {
                    info = tc_type_check_info_make_external_value();
                    break;
                }

                tc_set_error_at(checker,
                                expression->source_span,
                                NULL,
                                "Union values support only '.tag' and '.payload' access, but got '%s'.",
                                expression->as.member.member ? expression->as.member.member : "?");
                return NULL;
            }

            checked_type_to_string(target_type, target_text, sizeof(target_text));
            tc_set_error_at(checker,
                            expression->as.member.target->source_span,
                            NULL,
                            "Member access requires an imported or external target, but got %s.",
                            target_text);
            return NULL;
        }

    case AST_EXPR_CAST:
        {
            const TypeCheckInfo *operand_info = tc_check_expression(
                checker, expression->as.cast.expression);
            CheckedType operand_type, target_type;
            char operand_text[64], target_text[64];

            if (!operand_info) {
                return NULL;
            }

            operand_type = tc_type_check_source_type(operand_info);
            target_type = tc_checked_type_from_cast_target(checker, expression);
            if (checker->has_error) {
                return NULL;
            }
            if (operand_type.kind == CHECKED_TYPE_EXTERNAL || tc_checked_type_is_numeric(operand_type)) {
                info = tc_type_check_info_make(target_type);
                break;
            }

            checked_type_to_string(operand_type, operand_text, sizeof(operand_text));
            checked_type_to_string(target_type, target_text, sizeof(target_text));
            tc_set_error_at(checker,
                            expression->as.cast.expression->source_span,
                            NULL,
                            "Cannot cast expression of type %s to %s.",
                            operand_text,
                            target_text);
            return NULL;
        }

    case AST_EXPR_ARRAY_LITERAL:
        if (!tc_check_array_literal(checker, expression, &info)) {
            return NULL;
        }
        break;

    case AST_EXPR_GROUPING:
        {
            const TypeCheckInfo *inner_info = tc_check_expression(checker,
                                                                   expression->as.grouping.inner);
            if (!inner_info) {
                return NULL;
            }
            info = *inner_info;
        }
        break;

    case AST_EXPR_DISCARD:
        info = tc_type_check_info_make(tc_checked_type_void());
        break;

    case AST_EXPR_POST_INCREMENT:
    case AST_EXPR_POST_DECREMENT:
        {
            const AstExpression *operand = (expression->kind == AST_EXPR_POST_INCREMENT)
                ? expression->as.post_increment.operand
                : expression->as.post_decrement.operand;
            const TypeCheckInfo *operand_info = tc_check_expression(checker, operand);
            CheckedType operand_type;
            if (!operand_info) {
                return NULL;
            }
            operand_type = tc_type_check_source_type(operand_info);
            if (!tc_checked_type_is_numeric(operand_type)) {
                char operand_text[64];
                checked_type_to_string(operand_type, operand_text, sizeof(operand_text));
                tc_set_error_at(checker,
                                expression->source_span,
                                NULL,
                                "Postfix operator requires a numeric operand but got %s.",
                                operand_text);
                return NULL;
            }
            info = *operand_info;
        }
        break;

    case AST_EXPR_MEMORY_OP:
        return tc_check_memory_operation_expression(checker, expression);

    case AST_EXPR_SPAWN:
        {
            const TypeCheckInfo *callable_info = tc_check_expression(checker,
                                                                     expression->as.spawn.callable);
            TcVisitedSymbolSet visited;
            if (!callable_info) {
                return NULL;
            }
            if (!callable_info->is_callable ||
                (callable_info->parameters && callable_info->parameters->count != 0)) {
                tc_set_error_at(checker,
                                expression->as.spawn.callable->source_span,
                                NULL,
                                "spawn requires a zero-argument callable.");
                return NULL;
            }
            memset(&visited, 0, sizeof(visited));
            if (!tc_analyze_spawn_callable_races(checker,
                                                 expression->as.spawn.callable,
                                                 &visited)) {
                tc_visited_free(&visited);
                return NULL;
            }
            tc_visited_free(&visited);
            if (callable_info->callable_return_type.kind == CHECKED_TYPE_VOID) {
                info = tc_type_check_info_make(tc_checked_type_named("Thread", 0, 0));
            } else {
                info = tc_type_check_info_make(tc_checked_type_named("Future", 1, 0));
                tc_type_check_info_set_first_generic_arg(&info,
                                                         callable_info->callable_return_type);
            }
        }
        break;

    default:
        break;
    }

    return tc_store_expression_info(checker, expression, info);
}
