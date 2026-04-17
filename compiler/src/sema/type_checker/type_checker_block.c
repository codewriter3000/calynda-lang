#include "type_checker_internal.h"

static bool tc_check_swap_target(TypeChecker *checker,
                                 const AstExpression *expression,
                                 const char *side_name,
                                 const TypeCheckInfo **info_out,
                                 const Symbol **root_symbol_out) {
    const TypeCheckInfo *info;
    const Symbol *root_symbol = NULL;
    const AstSourceSpan *related_span = NULL;

    if (info_out) {
        *info_out = NULL;
    }
    if (root_symbol_out) {
        *root_symbol_out = NULL;
    }

    info = tc_check_expression(checker, expression);
    if (!info) {
        return false;
    }

    if (expression->kind == AST_EXPR_DISCARD ||
        !tc_expression_is_assignment_target(checker, expression, &root_symbol)) {
        if (root_symbol && tc_source_span_is_valid(root_symbol->declaration_span)) {
            related_span = &root_symbol->declaration_span;
        }
        tc_set_error_at(checker,
                        expression->source_span,
                        related_span,
                        "Swap statement requires an assignable %s target.",
                        side_name);
        return false;
    }

    if (root_symbol && root_symbol->is_final) {
        tc_set_error_at(checker,
                        expression->source_span,
                        &root_symbol->declaration_span,
                        "Cannot swap final symbol '%s'.",
                        root_symbol->name ? root_symbol->name : "<anonymous>");
        return false;
    }

    if (info_out) {
        *info_out = info;
    }
    if (root_symbol_out) {
        *root_symbol_out = root_symbol;
    }
    return true;
}

bool tc_check_block(TypeChecker *checker, const AstBlock *block,
                    const BlockContext *context,
                    CheckedType *return_type, AstSourceSpan *return_span) {
    const Scope *block_scope;
    CheckedType current_return_type = tc_checked_type_void();
    AstSourceSpan first_return_span;
    bool saw_return = false;
    size_t i;

    memset(&first_return_span, 0, sizeof(first_return_span));
    block_scope = symbol_table_find_scope(checker->symbols, block, SCOPE_KIND_BLOCK);
    if (!block_scope) {
        tc_set_error(checker, "Internal error: missing block scope.");
        return false;
    }

    for (i = 0; i < block->statement_count; i++) {
        const AstStatement *statement = block->statements[i];

        switch (statement->kind) {
        case AST_STMT_LOCAL_BINDING:
            {
                const Symbol *symbol = scope_lookup_local(block_scope,
                                                          statement->as.local_binding.name);

                if (!symbol) {
                    tc_set_error_at(checker,
                                    statement->as.local_binding.name_span,
                                    NULL,
                                    "Internal error: missing local symbol '%s'.",
                                    statement->as.local_binding.name);
                    return false;
                }

                if (!tc_resolve_symbol_info(checker, symbol)) {
                    return false;
                }
            }
            break;

        case AST_STMT_RETURN:
            {
                CheckedType statement_return_type;
                AstSourceSpan statement_span;

                if (statement->as.return_expression) {
                    const TypeCheckInfo *info = tc_check_expression(checker,
                                                                    statement->as.return_expression);
                    if (!info) {
                        return false;
                    }

                    statement_return_type = tc_type_check_source_type(info);
                    statement_span = statement->as.return_expression->source_span;
                } else {
                    statement_return_type = tc_checked_type_void();
                    statement_span = statement->source_span;
                }

                if (context && context->has_expected_return_type) {
                    char expected_text[64];

                    checked_type_to_string(context->expected_return_type,
                                           expected_text,
                                           sizeof(expected_text));

                    if (!statement->as.return_expression &&
                        context->expected_return_type.kind != CHECKED_TYPE_VOID &&
                        context->kind != BLOCK_CONTEXT_START) {
                        tc_set_error_at(checker,
                                        statement->source_span,
                                        tc_block_context_related_span(context,
                                                                      statement->source_span),
                                        "Return statement in %s must produce %s.",
                                        tc_block_context_name(context->kind),
                                        expected_text);
                        return false;
                    }

                    if (statement->as.return_expression &&
                        !tc_checked_type_assignable(context->expected_return_type,
                                                    statement_return_type)) {
                        char actual_text[64];

                        checked_type_to_string(statement_return_type,
                                               actual_text,
                                               sizeof(actual_text));
                        tc_set_error_at(checker,
                                        statement->as.return_expression->source_span,
                                        tc_block_context_related_span(context,
                                                                      statement->as.return_expression->source_span),
                                        "Return statement in %s must produce %s but got %s.",
                                        tc_block_context_name(context->kind),
                                        expected_text,
                                        actual_text);
                        return false;
                    }
                }

                if (!saw_return) {
                    current_return_type = statement_return_type;
                    first_return_span = statement_span;
                    saw_return = true;
                } else {
                    CheckedType merged_type;
                    char first_text[64];
                    char second_text[64];

                    if (!tc_merge_return_types(current_return_type,
                                              statement_return_type,
                                              &merged_type)) {
                        checked_type_to_string(current_return_type,
                                               first_text,
                                               sizeof(first_text));
                        checked_type_to_string(statement_return_type,
                                               second_text,
                                               sizeof(second_text));
                        tc_set_error_at(checker,
                                        statement_span,
                                        tc_source_span_is_valid(first_return_span)
                                            ? &first_return_span
                                            : NULL,
                                        "Inconsistent return types in block: %s and %s.",
                                        first_text,
                                        second_text);
                        return false;
                    }

                    current_return_type = merged_type;
                }
            }
            break;

        case AST_STMT_EXIT:
            if (context &&
                (context->kind != BLOCK_CONTEXT_LAMBDA ||
                 (context->has_expected_return_type &&
                  context->expected_return_type.kind != CHECKED_TYPE_VOID))) {
                tc_set_error_at(checker,
                                statement->source_span,
                                tc_block_context_related_span(context,
                                                              statement->source_span),
                                "exit is only permitted in void-typed lambda blocks.");
                return false;
            }

            if (!saw_return) {
                current_return_type = tc_checked_type_void();
                first_return_span = statement->source_span;
                saw_return = true;
            } else {
                CheckedType merged_type;

                if (!tc_merge_return_types(current_return_type,
                                           tc_checked_type_void(),
                                           &merged_type)) {
                    char first_text[64];
                    char second_text[64];

                    checked_type_to_string(current_return_type, first_text, sizeof(first_text));
                    checked_type_to_string(tc_checked_type_void(), second_text, sizeof(second_text));
                    tc_set_error_at(checker,
                                    first_return_span,
                                    NULL,
                                    "Inconsistent return types in block: %s and %s.",
                                    first_text,
                                    second_text);
                    return false;
                }

                current_return_type = merged_type;
            }
            break;

        case AST_STMT_THROW:
            if (statement->as.throw_expression &&
                !tc_check_expression(checker, statement->as.throw_expression)) {
                return false;
            }
            break;

        case AST_STMT_EXPRESSION:
            if (statement->as.expression &&
                !tc_check_expression(checker, statement->as.expression)) {
                return false;
            }
            break;

        case AST_STMT_MANUAL:
            if (statement->as.manual.body) {
                CheckedType statement_return_type;
                AstSourceSpan statement_span;
                BlockContext nested_context;
                const BlockContext *manual_context = NULL;

                statement_return_type = tc_checked_type_void();
                memset(&statement_span, 0, sizeof(statement_span));
                if (context) {
                    nested_context = *context;
                    nested_context.enforce_expected_return_type = false;
                    manual_context = &nested_context;
                }

                if (!tc_check_block(checker, statement->as.manual.body,
                                    manual_context,
                                    &statement_return_type,
                                    &statement_span)) {
                    return false;
                }

                if (tc_source_span_is_valid(statement_span)) {
                    if (!saw_return) {
                        current_return_type = statement_return_type;
                        first_return_span = statement_span;
                        saw_return = true;
                    } else {
                        CheckedType merged_type;

                        if (!tc_merge_return_types(current_return_type,
                                                   statement_return_type,
                                                   &merged_type)) {
                            char first_text[64];
                            char second_text[64];

                            checked_type_to_string(current_return_type,
                                                   first_text,
                                                   sizeof(first_text));
                            checked_type_to_string(statement_return_type,
                                                   second_text,
                                                   sizeof(second_text));
                            tc_set_error_at(checker,
                                            statement_span,
                                            tc_source_span_is_valid(first_return_span)
                                                ? &first_return_span
                                                : NULL,
                                            "Inconsistent return types in block: %s and %s.",
                                            first_text,
                                            second_text);
                            return false;
                        }

                        current_return_type = merged_type;
                    }
                }
            }
            break;

        case AST_STMT_SWAP:
            {
                const TypeCheckInfo *left_info;
                const TypeCheckInfo *right_info;
                CheckedType left_type;
                CheckedType right_type;

                if (!tc_check_swap_target(checker,
                                          statement->as.swap.left,
                                          "left-hand",
                                          &left_info,
                                          NULL) ||
                    !tc_check_swap_target(checker,
                                          statement->as.swap.right,
                                          "right-hand",
                                          &right_info,
                                          NULL)) {
                    return false;
                }

                left_type = tc_type_check_source_type(left_info);
                right_type = tc_type_check_source_type(right_info);
                if (!tc_checked_type_equals(left_type, right_type)) {
                    char left_text[64];
                    char right_text[64];

                    checked_type_to_string(left_type, left_text, sizeof(left_text));
                    checked_type_to_string(right_type, right_text, sizeof(right_text));
                    tc_set_error_at(checker,
                                    statement->as.swap.right->source_span,
                                    &statement->as.swap.left->source_span,
                                    "Swap statement requires matching target types but got %s and %s.",
                                    left_text,
                                    right_text);
                    return false;
                }
            }
            break;
        }
    }

    if (context && context->has_expected_return_type &&
        context->enforce_expected_return_type &&
        !(context->kind == BLOCK_CONTEXT_START &&
          current_return_type.kind == CHECKED_TYPE_VOID) &&
        !tc_checked_type_assignable(context->expected_return_type,
                                    current_return_type)) {
        AstSourceSpan primary_span = tc_source_span_is_valid(first_return_span)
                                         ? first_return_span
                                         : context->owner_span;
        char expected_text[64];
        char actual_text[64];

        checked_type_to_string(context->expected_return_type,
                               expected_text,
                               sizeof(expected_text));
        checked_type_to_string(current_return_type,
                               actual_text,
                               sizeof(actual_text));
        tc_set_error_at(checker,
                        primary_span,
                        tc_block_context_related_span(context, primary_span),
                        "%s body must produce %s but got %s.",
                        context->kind == BLOCK_CONTEXT_START ? "start" : "Lambda",
                        expected_text,
                        actual_text);
        return false;
    }

    if (return_type) {
        *return_type = current_return_type;
    }
    if (return_span) {
        *return_span = first_return_span;
    }

    return true;
}
