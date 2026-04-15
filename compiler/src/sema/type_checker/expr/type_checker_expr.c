#include "type_checker_internal.h"

const TypeCheckInfo *tc_check_expression(TypeChecker *checker,
                                         const AstExpression *expression) {
    TypeCheckInfo info;
    const TypeCheckInfo *cached_info;

    if (!checker || !expression) {
        return NULL;
    }

    cached_info = type_checker_get_expression_info(checker, expression);
    if (cached_info) {
        return cached_info;
    }

    info = tc_type_check_info_make(tc_checked_type_invalid());

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        switch (expression->as.literal.kind) {
        case AST_LITERAL_INTEGER:
            info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_INT32, 0));
            break;
        case AST_LITERAL_FLOAT:
            info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_FLOAT64, 0));
            break;
        case AST_LITERAL_BOOL:
            info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_BOOL, 0));
            break;
        case AST_LITERAL_CHAR:
            info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_CHAR, 0));
            break;
        case AST_LITERAL_STRING:
        case AST_LITERAL_TEMPLATE:
            if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
                size_t i;

                for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                    const AstTemplatePart *part =
                        &expression->as.literal.as.template_parts.items[i];

                    if (part->kind == AST_TEMPLATE_PART_EXPRESSION) {
                        const TypeCheckInfo *part_info = tc_check_expression(checker,
                                                                             part->as.expression);

                        if (!part_info) {
                            return NULL;
                        }
                        if (part_info->is_callable &&
                            part_info->parameters &&
                            part_info->parameters->count == 0 &&
                            part_info->callable_return_type.kind == CHECKED_TYPE_VOID) {
                            tc_set_error_at(
                                checker,
                                part->as.expression->source_span,
                                NULL,
                                "Template interpolation cannot auto-call a zero-argument callable returning void.");
                            return NULL;
                        }
                        if (tc_type_check_source_type(part_info).kind == CHECKED_TYPE_VOID) {
                            tc_set_error_at(checker,
                                            part->as.expression->source_span,
                                            NULL,
                                            "Template interpolation cannot use a void expression.");
                            return NULL;
                        }
                    }
                }
            }
            info = tc_type_check_info_make(tc_checked_type_value(AST_PRIMITIVE_STRING, 0));
            break;
        case AST_LITERAL_NULL:
            info = tc_type_check_info_make(tc_checked_type_null());
            break;
        }
        break;

    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *symbol = symbol_table_resolve_identifier(checker->symbols, expression);

            if (!symbol) {
                tc_set_error_at(checker,
                                expression->source_span,
                                NULL,
                                "Unresolved identifier '%s'.",
                                expression->as.identifier ? expression->as.identifier : "<unknown>");
                return NULL;
            }

            if (!tc_validate_internal_access(checker, expression, symbol)) {
                return NULL;
            }

            cached_info = tc_resolve_symbol_info(checker, symbol);
            if (!cached_info) {
                return NULL;
            }

            info = *cached_info;
        }
        break;

    case AST_EXPR_LAMBDA:
        return tc_check_lambda_expression(checker, expression, NULL, NULL);

    case AST_EXPR_ASSIGNMENT:
        {
            const TypeCheckInfo *target_info;
            const TypeCheckInfo *value_info;
            const Symbol *target_symbol = NULL;
            CheckedType source_type;
            bool assignable;

            target_info = tc_check_expression(checker, expression->as.assignment.target);
            if (!target_info) {
                return NULL;
            }

            value_info = tc_check_expression(checker, expression->as.assignment.value);
            if (!value_info) {
                return NULL;
            }

            if (!tc_expression_is_assignment_target(checker,
                                                    expression->as.assignment.target,
                                                    &target_symbol)) {
                const AstSourceSpan *related_span = NULL;

                if (target_symbol && tc_source_span_is_valid(target_symbol->declaration_span)) {
                    related_span = &target_symbol->declaration_span;
                }

                tc_set_error_at(checker,
                                expression->as.assignment.target->source_span,
                                related_span,
                                "Operator '%s' requires an assignable target.",
                                tc_assignment_operator_name(expression->as.assignment.operator));
                return NULL;
            }

            if (target_symbol && target_symbol->is_final) {
                tc_set_error_at(checker,
                                expression->as.assignment.target->source_span,
                                &target_symbol->declaration_span,
                                "Cannot assign to final symbol '%s'.",
                                target_symbol->name ? target_symbol->name : "<anonymous>");
                return NULL;
            }

            /* Discard target accepts any value without type checking. */
            if (expression->as.assignment.target->kind == AST_EXPR_DISCARD) {
                info = *value_info;
                break;
            }

            source_type = tc_type_check_source_type(value_info);
            if (expression->as.assignment.operator == AST_ASSIGN_OP_ASSIGN) {
                assignable = (value_info->is_callable && source_type.kind == CHECKED_TYPE_EXTERNAL) ||
                             tc_checked_type_assignable(target_info->type, source_type);
            } else {
                AstBinaryOperator binary_operator;
                CheckedType result_type;

                if (!tc_map_compound_assignment(expression->as.assignment.operator, &binary_operator) ||
                    !tc_check_binary_operator(checker,
                                              expression,
                                              binary_operator,
                                              target_info->type,
                                              source_type,
                                              &result_type)) {
                    return NULL;
                }

                assignable = tc_checked_type_assignable(target_info->type, result_type);
            }

            if (!assignable) {
                char source_text[64];
                char target_text[64];

                checked_type_to_string(source_type, source_text, sizeof(source_text));
                checked_type_to_string(target_info->type, target_text, sizeof(target_text));
                tc_set_error_at(checker,
                                expression->as.assignment.value->source_span,
                                &expression->as.assignment.target->source_span,
                                "Operator '%s' cannot assign %s to %s.",
                                tc_assignment_operator_name(expression->as.assignment.operator),
                                source_text,
                                target_text);
                return NULL;
            }

            info = *value_info;
            info.type = target_info->type;
            if (value_info->is_callable) {
                info.callable_return_type = target_info->type;
            }
        }
        break;

    default:
        return tc_check_expression_ext(checker, expression);
    }

    return tc_store_expression_info(checker, expression, info);
}
