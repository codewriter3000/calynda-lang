            target_type = tc_type_check_source_type(target_info);
            if (strcmp(expression->as.member.member, "length") == 0 &&
                tc_checked_type_has_length_member(target_type)) {
                info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_INT64, 0));
                break;
            }
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
                if (tc_program_has_boot_entry(checker)) {
                    if (tc_is_supported_freestanding_import_member(target_symbol,
                                                                   expression->as.member.member)) {
                        info = tc_type_check_info_make_external_callable();
                        break;
                    }

                    tc_report_freestanding_import_member_error(checker,
                                                               expression,
                                                               target_symbol);
                    return NULL;
                }

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

#include "type_checker_expr_more_p5.inc"
