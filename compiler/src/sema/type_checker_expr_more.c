#include "type_checker_internal.h"

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
            char target_text[64];

            if (!target_info) {
                return NULL;
            }

            target_type = tc_type_check_source_type(target_info);
            if (target_type.kind == CHECKED_TYPE_EXTERNAL) {
                info = tc_type_check_info_make_external_callable();
                break;
            }

            /* Union member access: Option.Some */
            if (target_type.kind == CHECKED_TYPE_NAMED &&
                expression->as.member.target->kind == AST_EXPR_IDENTIFIER) {
                const Symbol *union_sym = symbol_table_resolve_identifier(
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
        {
            CheckedType element_type = tc_checked_type_invalid();
            size_t i;

            if (expression->as.array_literal.elements.count == 0) {
                tc_set_error_at(checker, expression->source_span, NULL,
                                "Cannot infer the element type of an empty array literal.");
                return NULL;
            }
            for (i = 0; i < expression->as.array_literal.elements.count; i++) {
                const TypeCheckInfo *element_info = tc_check_expression(checker,
                                                                        expression->as.array_literal.elements.items[i]);
                CheckedType candidate_type;

                if (!element_info) {
                    return NULL;
                }

                candidate_type = tc_type_check_source_type(element_info);
                if (candidate_type.kind == CHECKED_TYPE_VOID ||
                    candidate_type.kind == CHECKED_TYPE_NULL) {
                    tc_set_error_at(checker,
                                    expression->as.array_literal.elements.items[i]->source_span,
                                    NULL,
                                    "Array literal elements cannot have type %s.",
                                    candidate_type.kind == CHECKED_TYPE_VOID
                                        ? "void"
                                        : "null");
                    return NULL;
                }

                if (i == 0) {
                    element_type = candidate_type;
                } else if (!tc_merge_types_for_inference(element_type,
                                                         candidate_type,
                                                         &element_type)) {
                    char left_text[64], right_text[64];
                    checked_type_to_string(element_type, left_text, sizeof(left_text));
                    checked_type_to_string(candidate_type, right_text, sizeof(right_text));
                    tc_set_error_at(checker,
                                    expression->as.array_literal.elements.items[i]->source_span,
                                    &expression->as.array_literal.elements.items[0]->source_span,
                                    "Array literal elements must have compatible types, but got %s and %s.",
                                    left_text,
                                    right_text);
                    return NULL;
                }
            }

            if (element_type.kind != CHECKED_TYPE_VALUE) {
                tc_set_error_at(checker, expression->source_span, NULL,
                                "Array literal element type cannot be inferred from external values.");
                return NULL;
            }

            info = tc_type_check_info_make(
                tc_checked_type_value(element_type.primitive, element_type.array_depth + 1));
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
        {
            size_t i;
            for (i = 0; i < expression->as.memory_op.arguments.count; i++) {
                const TypeCheckInfo *arg_info = tc_check_expression(
                    checker, expression->as.memory_op.arguments.items[i]);
                CheckedType arg_type;
                if (!arg_info) {
                    return NULL;
                }
                arg_type = tc_type_check_source_type(arg_info);
                if (!tc_checked_type_is_integral(arg_type)) {
                    char arg_text[64];
                    checked_type_to_string(arg_type, arg_text, sizeof(arg_text));
                    tc_set_error_at(checker,
                                    expression->as.memory_op.arguments.items[i]->source_span,
                                    NULL,
                                    "Memory operation argument must have an integral type but got %s.",
                                    arg_text);
                    return NULL;
                }
            }
            if (expression->as.memory_op.kind == AST_MEMORY_FREE) {
                info = tc_type_check_info_make(tc_checked_type_void());
            } else {
                info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_INT64, 0));
            }
        }
        break;

    default:
        break;
    }

    return tc_store_expression_info(checker, expression, info);
}
