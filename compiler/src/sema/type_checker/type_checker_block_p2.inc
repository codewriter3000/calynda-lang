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
