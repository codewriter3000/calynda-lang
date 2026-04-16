#include "type_checker_internal.h"

/* type_checker_expr_array.c */
bool tc_check_array_literal(TypeChecker *checker,
                            const AstExpression *expression,
                            TypeCheckInfo *info);

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

    default:
        break;
    }

    return tc_store_expression_info(checker, expression, info);
}
